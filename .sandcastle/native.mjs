import { execFile, execFileSync } from "node:child_process";
import { promisify } from "node:util";
import { fileURLToPath } from "node:url";
import {
  existsSync,
  mkdirSync,
  readFileSync,
  realpathSync,
  writeFileSync,
} from "node:fs";
import { dirname, isAbsolute, join, resolve, sep } from "node:path";
import { WorkflowOperationError } from "@rebuild/sandcastle/workflow";
import { createCommandRunner } from "@rebuild/sandcastle/workflow/integrations";
import { createEditorEvidence } from "./evidence.mjs";

const execute = promisify(execFile);
const stable = (value) =>
  Array.isArray(value)
    ? value.map(stable)
    : value && typeof value === "object"
      ? Object.fromEntries(
          Object.keys(value)
            .sort()
            .map((key) => [key, stable(value[key])]),
        )
      : value;

const localGit = (cwd, ...args) =>
  execFileSync("git", args, {
    cwd,
    encoding: "utf8",
    windowsHide: true,
    env: { ...process.env, GIT_ALLOW_PROTOCOL: "file", GIT_NO_LAZY_FETCH: "1" },
  }).trim();
const prerequisite = (message, cause) =>
  new WorkflowOperationError(
    "permanent",
    `Native prerequisite: ${message}`,
    cause,
  );
function childPath(directory, path) {
  if (
    !path ||
    /[\x00-\x1f]/.test(path) ||
    isAbsolute(path) ||
    path.split(/[\\/]/).some((part) => ["..", ".", ""].includes(part))
  )
    throw prerequisite(`Unsafe native resource path: ${path}`);
  const target = resolve(directory, path);
  if (!target.startsWith(realpathSync(directory) + sep))
    throw prerequisite("Native resource path escaped its owned directory.");
  let parent = existsSync(target) ? target : dirname(target);
  while (!existsSync(parent)) parent = dirname(parent);
  const canonical = realpathSync(parent);
  if (
    canonical !== realpathSync(directory) &&
    !canonical.startsWith(realpathSync(directory) + sep)
  )
    throw prerequisite(
      "Native resource path redirects outside its owned directory.",
    );
  return target;
}
function gitlinks(source, candidate) {
  if (!/^[a-f0-9]{40}$/.test(candidate))
    throw prerequisite("A complete local candidate SHA is required.");
  localGit(source, "cat-file", "-e", `${candidate}^{commit}`);
  return localGit(source, "ls-tree", "-r", "-z", candidate)
    .split("\0")
    .filter((line) => line.startsWith("160000 "))
    .map((line) => {
      const [metadata, path] = line.split("\t");
      return { path, pin: metadata.split(" ")[2] };
    });
}

/** Local-only preparation inside an already locked runtime resource. Unknown partial creation is preserved. */
export async function prepareNativeWorktree({
  directory,
  source,
  candidate,
  localDependencies,
}) {
  directory = realpathSync(directory);
  source = realpathSync(source);
  const path = childPath(directory, "prepared");
  const intent = join(directory, "native-intent.json");
  const receipt = join(directory, "native-ready.json");
  const common = localGit(
    source,
    "rev-parse",
    "--path-format=absolute",
    "--git-common-dir",
  );
  if (existsSync(intent) || existsSync(path)) {
    if (!existsSync(receipt))
      throw prerequisite(
        `Interrupted native creation needs reconciliation; preserved ${intent}`,
      );
    const saved = JSON.parse(readFileSync(receipt, "utf8"));
    if (
      saved.path !== path ||
      saved.source !== source ||
      saved.common !== common ||
      realpathSync(path) !== path ||
      localGit(
        path,
        "rev-parse",
        "--path-format=absolute",
        "--git-common-dir",
      ) !== common
    )
      throw prerequisite(
        "Prepared native ownership changed; existing files were preserved.",
      );
    await nativeDependencies({ prepared: path, source, candidate });
    return { source, path, command: createCommandRunner({ cwd: source }) };
  }
  const links = gitlinks(source, candidate);
  const sources = links.map((link) => {
    childPath(directory, `prepared/${link.path}`);
    const dependency = localDependencies?.[link.path];
    if (!dependency)
      throw prerequisite(
        `No initialized local source for ${link.path}; no download attempted.`,
      );
    const actual = realpathSync(dependency);
    if (localGit(actual, "status", "--porcelain"))
      throw prerequisite(`Local dependency source has edits: ${actual}`);
    localGit(actual, "cat-file", "-e", `${link.pin}^{commit}`);
    if (
      localGit(actual, "ls-tree", "-r", link.pin)
        .split("\n")
        .some((line) => line.startsWith("160000 "))
    )
      throw prerequisite(
        `Nested dependency preparation is not configured: ${link.path}`,
      );
    return { ...link, source: actual };
  });
  writeFileSync(
    intent,
    JSON.stringify(
      { source, path, common, candidate, dependencies: sources },
      null,
      2,
    ),
    { flag: "wx" },
  );
  try {
    localGit(source, "worktree", "add", "--detach", path, candidate);
    for (const link of sources) {
      const destination = childPath(path, link.path);
      mkdirSync(dirname(destination), { recursive: true });
      localGit(
        source,
        "clone",
        "--local",
        "--no-hardlinks",
        "--no-checkout",
        link.source,
        destination,
      );
      localGit(destination, "checkout", "--detach", link.pin);
      localGit(path, "submodule", "absorbgitdirs", "--", link.path);
    }
    await nativeDependencies({ prepared: path, source, candidate });
    if (localGit(path, "status", "--porcelain"))
      throw prerequisite("New native preparation was not clean.");
    writeFileSync(
      receipt,
      JSON.stringify({ source, path, common, candidate }, null, 2),
      { flag: "wx" },
    );
    return { source, path, command: createCommandRunner({ cwd: source }) };
  } catch (cause) {
    throw prerequisite(
      `Native creation stopped; preserve ${intent} and ${path} for reconciliation.`,
      cause,
    );
  }
}

/** Verify actual prepared submodules against current candidate pins before stamping evidence. */
export async function nativeDependencies({ prepared, source, candidate }) {
  const observations = gitlinks(source, candidate).map(({ path, pin }) => {
    const dependency = childPath(prepared, path);
    if (
      !existsSync(dependency) ||
      localGit(dependency, "rev-parse", "HEAD") !== pin
    )
      throw prerequisite(
        `Prepared dependency pin differs for ${path}; preserve the checkout and prepare the current pins locally.`,
      );
    if (localGit(dependency, "status", "--porcelain"))
      throw prerequisite(`Prepared dependency has edits: ${path}`);
    return {
      path,
      pin: localGit(dependency, "rev-parse", "HEAD"),
      gitDirectory: localGit(dependency, "rev-parse", "--absolute-git-dir"),
    };
  });
  return JSON.stringify({
    prepared: realpathSync(prepared),
    dependencies: observations,
  });
}

const selectors = [
  "scene-hooks",
  "camera-inverse",
  "renderer-batches",
  "entity-tags",
  "viewport-pixels",
  "minimize-restore",
  "layer-partition",
  "typed-events",
  "event-names",
  "font-atlas",
  "atlas-placeholders",
  "gl-boundaries",
  "gl-buffers",
];
const windowsChecks = [12, 15, 16, 17, 18].map((number) => ({
  script: `Tests/Windows/issue-${number}.ps1`,
  marker: `PASS: editor-issue-${number}`,
}));
const unitTests = {
  whenTracked: "Tests/Build-UnitTests.lua",
  script: "Tests/Windows/unit-tests.ps1",
  marker: "PASS: unit-tests-{configuration}",
  msbuild: true,
  timeoutSeconds: 900,
};
// Coverage data only, never queue membership. Scene hooks underpin all fixtures;
// atlas placeholders consume font-atlas state, and buffer tests exercise GL boundaries.
const coverage = {
  12: { selectors, windows: [12, 15, 16, 17, 18], fullSuite: true },
  13: { selectors: ["camera-inverse"] },
  14: { selectors: ["renderer-batches"] },
  15: { selectors: ["scene-hooks"], windows: [15] },
  16: { selectors: ["entity-tags"], windows: [16] },
  17: { selectors: ["viewport-pixels"], windows: [17] },
  18: { selectors: ["minimize-restore"], windows: [18] },
  19: { selectors: ["layer-partition"] },
  20: { selectors: ["typed-events", "event-names"] },
  21: { selectors: ["font-atlas", "atlas-placeholders"] },
  22: { selectors: ["atlas-placeholders", "font-atlas"] },
  23: { selectors: ["gl-boundaries", "gl-buffers"] },
  24: { selectors: ["gl-buffers", "gl-boundaries"] },
  29: { selectors: ["camera-inverse"], unit: true },
};

/** Trusted project plan. Missing coverage deliberately selects the entire Debug plan. */
export function selectNativePlan({ phase, issue } = {}) {
  if (!["diagnose", "validate"].includes(phase))
    throw new Error("Native phase must be diagnose or validate.");
  const mapped = phase === "diagnose" ? coverage[issue] : undefined;
  const complete = !mapped;
  return {
    phase,
    configurations: phase === "diagnose" ? ["Debug"] : ["Debug", "Release"],
    buildTarget: phase === "diagnose" ? "Build" : "Rebuild",
    fullSuite: complete || mapped.fullSuite === true,
    selectors: [...(mapped?.selectors ?? selectors)],
    windowsChecks: complete
      ? structuredClone(windowsChecks)
      : windowsChecks.filter((check) =>
          mapped.windows?.some(
            (number) => check.script === `Tests/Windows/issue-${number}.ps1`,
          ),
        ),
    optionalWindowsChecks:
      complete || mapped?.unit ? [structuredClone(unitTests)] : [],
  };
}

/** The report is authoritative only when its actual commands cover the required phase contract. */
export function nativeValidationResult({
  report,
  plan,
  candidate,
  reportPath,
  unitTests,
}) {
  if (
    report.phase === plan.phase &&
    report.status === "failed" &&
    report.failureKind === "prerequisite"
  )
    throw prerequisite(`${report.failure}; ${reportPath}`);
  if (
    report.commit !== candidate ||
    report.phase !== plan.phase ||
    report.buildTarget !== plan.buildTarget ||
    JSON.stringify(report.configurations) !==
      JSON.stringify(plan.configurations)
  )
    throw new Error(
      `Native phase contract does not match candidate/final plan: ${reportPath}`,
    );
  if (!Array.isArray(report.checks))
    throw new Error(`Native report is incomplete: ${reportPath}`);
  const checks = report.checks;
  if (
    new Set(checks.map((check) => check.name)).size !== checks.length ||
    checks.some(
      (check) =>
        typeof check.name !== "string" ||
        typeof check.log !== "string" ||
        !Number.isFinite(check.durationMs) ||
        check.durationMs < 0 ||
        typeof check.passed !== "boolean",
    )
  )
    throw new Error(
      `Native report check evidence/timings are incomplete: ${reportPath}`,
    );
  const required = ["generate"];
  for (const config of plan.configurations) {
    required.push(`build-${config}`);
    if (plan.fullSuite) required.push(`regression-all-${config}`);
    required.push(
      ...plan.selectors.map((selector) => `regression-${selector}-${config}`),
    );
    required.push(
      ...[
        ...plan.windowsChecks,
        ...(unitTests ? plan.optionalWindowsChecks : []),
      ].map(
        (check) =>
          `windows-${check.script
            .split("/")
            .at(-1)
            .replace(/\.ps1$/, "")}-${config}`,
      ),
    );
  }
  const failures = checks
    .filter((check) => !check.passed)
    .map((check) => ({
      check: check.name,
      diagnostic: `exit=${check.exitCode}; timeout=${check.timedOut}; marker=${check.markerPresent}`,
      evidence: [check.log, reportPath],
    }));
  if (
    report.status === "passed" &&
    (failures.length ||
      required.some(
        (name) =>
          !checks.some(
            (check) =>
              check.name === name &&
              check.passed &&
              check.exitCode === 0 &&
              check.timedOut === false &&
              check.markerPresent === true,
          ),
      ))
  )
    throw new Error(
      `Native report is incomplete or missing required full/standalone/UI/doctest acceptance: ${reportPath}`,
    );
  if (report.status !== "passed" && report.failureKind === "prerequisite")
    throw prerequisite(`${report.failure}; ${reportPath}`);
  if (
    report.status !== "passed" &&
    (report.failureKind !== "substantive" || !failures.length)
  )
    throw new Error(`Native outcome is unknown; preserve ${reportPath}`);
  return {
    candidate,
    passed: report.status === "passed",
    failures,
    evidence: failures.length
      ? failures.map(
          (failure) =>
            `${failure.check}: ${failure.diagnostic}; ${failure.evidence.join("; ")}`,
        )
      : [reportPath, report.report],
    // Authoritative host per-check timing, separate from the adapter's outer measurements.
    nativeTimings: checks.map(({ name, durationMs, passed, log }) => ({
      check: name,
      durationMs,
      passed,
      log,
    })),
  };
}

/** Explicit native operations and freshly observed host identities for runEditorTask. */
export function createEditorNativeValidation({
  source,
  localDependencies,
  msbuild,
  powershell,
  issue,
  reviewPolicy,
  reportsDirectory,
}) {
  const script = new URL("../Scripts/Validate-Windows.ps1", import.meta.url);
  const inspect = new URL(
    "../Scripts/Inspect-NativeEnvironment.ps1",
    import.meta.url,
  );
  const plans = {
    diagnose: selectNativePlan({ phase: "diagnose", issue }),
    validate: selectNativePlan({ phase: "validate", issue }),
  };
  const run =
    (phase) =>
    async ({ worktree, candidate, signal }) => {
      await nativeDependencies({ prepared: worktree, source, candidate });
      const directory = join(
        reportsDirectory,
        `${phase}-${candidate}-${Date.now()}`,
      );
      mkdirSync(directory, { recursive: true });
      const planPath = join(directory, "plan.json");
      const reportPath = join(directory, "results.json");
      writeFileSync(planPath, JSON.stringify(plans[phase], null, 2), {
        flag: "wx",
      });
      let exit = 0;
      try {
        const { stdout, stderr } = await execute(
          powershell,
          [
            "-NoProfile",
            "-File",
            fileURLToPath(script),
            "-Worktree",
            worktree,
            "-MSBuild",
            msbuild,
            "-Phase",
            phase,
            "-BuildTarget",
            plans[phase].buildTarget,
            "-PlanPath",
            planPath,
            "-ReportPath",
            reportPath,
          ],
          {
            windowsHide: true,
            signal,
            maxBuffer: 8 * 1024 * 1024,
            timeout: 3 * 60 * 60 * 1000,
          },
        );
        writeFileSync(join(directory, "driver.log"), stdout + stderr);
      } catch (cause) {
        exit = cause.code;
        writeFileSync(
          join(directory, "driver.log"),
          `${cause.stdout ?? ""}\n${cause.stderr ?? ""}\n${cause.message}`,
        );
        if (
          signal?.aborted ||
          typeof exit !== "number" ||
          !existsSync(reportPath)
        )
          throw new Error(
            `Native process outcome is unknown; preserve ${directory}`,
            { cause },
          );
      }
      const report = JSON.parse(readFileSync(reportPath, "utf8"));
      if (exit !== 0 && report.status === "passed")
        throw new Error(
          `Native process/report exit disagree; preserve ${directory}`,
        );
      const tracked = localGit(
        source,
        "ls-tree",
        "-r",
        "--name-only",
        candidate,
      ).split("\n");
      return nativeValidationResult({
        report,
        plan: plans[phase],
        candidate,
        reportPath,
        unitTests: tracked.includes("Tests/Build-UnitTests.lua"),
      });
    };
  return {
    prepareValidation: ({ directory, candidate }) =>
      prepareNativeWorktree({
        directory,
        source,
        candidate,
        localDependencies,
      }),
    diagnose: run("diagnose"),
    validate: run("validate"),
    evidence: createEditorEvidence({
      validationPolicy: async () =>
        JSON.stringify({
          plans,
          driver: readFileSync(script, "utf8"),
          inspector: readFileSync(inspect, "utf8"),
          adapter: readFileSync(new URL(import.meta.url), "utf8"),
        }),
      dependencies: async (context) => {
        const prepared = await context.project.config.validationWorktree(
          context.candidate,
        );
        return nativeDependencies({
          prepared: prepared.path,
          source,
          candidate: context.candidate,
        });
      },
      environment: async (context) => {
        const prepared = await context.project.config.validationWorktree(
          context.candidate,
        );
        const { stdout } = await execute(
          powershell,
          [
            "-NoProfile",
            "-File",
            fileURLToPath(inspect),
            "-Worktree",
            prepared.path,
            "-MSBuild",
            msbuild,
          ],
          { windowsHide: true, signal: context.signal, timeout: 30000 },
        );
        return JSON.stringify(stable(JSON.parse(stdout)));
      },
      reviewPolicy,
    }),
  };
}
