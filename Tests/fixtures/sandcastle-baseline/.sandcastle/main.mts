import * as sandcastle from "@ai-hero/sandcastle";
import { docker } from "@ai-hero/sandcastle/sandboxes/docker";
import { existsSync, mkdirSync, openSync, closeSync, unlinkSync, readFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { config } from "./workflow-config.mjs";
import { hash, criteria, selectIssue, validationPlan, parseReview, runIssue } from "./workflow.mjs";
import { configureTools, command, git, readJson, writeJson, assertClean, safeMerge, runWindows, github, containerGitHubProxy, githubProxyHook } from "./host.mjs";

configureTools();
const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const stateDirectory = join(root, ".sandcastle/state");
const logs = join(root, ".sandcastle/logs/spec-11");
const stateFile = join(stateDirectory, "workflow.json");
const validationWorktree = resolve(root, config.validationWorktree);
const args = new Set(process.argv.slice(2));
for (const arg of args) {
  if (!["--check", "--offline", "--retry"].includes(arg)) throw new Error(`Unknown option: ${arg}`);
}
if (args.has("--offline") && !args.has("--check")) throw new Error("--offline is only supported with --check.");

function preflight() {
  if (process.platform !== "win32") throw new Error("Start in Windows PowerShell; acceptance requires Windows/MSVC/OpenGL.");
  if (Number(process.versions.node.split(".")[0]) < 22) throw new Error("Node 22 or later is required.");
  if (git(root, "branch", "--show-current") !== config.targetBranch) throw new Error(`Main checkout must be on ${config.targetBranch}.`);
  const origin = git(root, "remote", "get-url", "origin").replace(/\.git$/, "");
  if (!origin.endsWith(`/${config.repository}`) && !origin.endsWith(`:${config.repository}`)) throw new Error("Configured repository does not match origin.");
  for (const file of [join(validationWorktree, ".git"), join(root, "Scripts/Validate-Windows.ps1"), join(root, ".sandcastle/codex-home/auth.json")]) {
    if (!existsSync(file)) throw new Error(`Required local file is missing: ${file}`);
  }
  assertClean(validationWorktree);
  const submodules = git(validationWorktree, "submodule", "status", "--recursive");
  if (/^[-+U]/m.test(submodules)) throw new Error("Prepare pinned dependencies locally; no download will be attempted.");
  command("pwsh.exe", ["-NoProfile", "-Command", "if ($PSVersionTable.PSVersion.Major -lt 7) { exit 1 }"], root);
  command("gh.exe", ["--version"], root);
  command("docker.exe", ["image", "inspect", config.image, "--format", "{{.Id}}"], root);
}

preflight();
if (args.has("--check")) {
  if (!args.has("--offline")) {
    const issues = github(root, config.repository, logs).issues();
    const state = existsSync(stateFile) ? readJson(stateFile) : {};
    const next = selectIssue(config, issues, state.active);
    console.log(next ? `Next issue: #${next.number} ${next.title}` : "No eligible open issue.");
  }
  console.log("PASS: local workflow prerequisites. No agents started or issues modified.");
} else {
  await main();
}

async function main() {
  mkdirSync(stateDirectory, { recursive: true });
  mkdirSync(logs, { recursive: true });
  const lockPath = join(stateDirectory, "workflow.lock");
  let lock;
  try { lock = openSync(lockPath, "wx"); }
  catch { throw new Error(`Another run or an interrupted run owns ${lockPath}. Check its process before removing the lock.`); }
  let sandbox;
  try {
    writeJson(join(stateDirectory, "owner.json"), { pid: process.pid, started: new Date().toISOString() });
    const state = existsSync(stateFile) ? readJson(stateFile) : { repository: config.repository, completed: [], active: null };
    if (state.repository !== config.repository) throw new Error("State belongs to another repository.");
    if (state.pendingStash) throw new Error(`Preserved edits need recovery: ${state.pendingStash}. Inspect git status before continuing.`);
    const save = async () => writeJson(stateFile, state);
    const gh = github(root, config.repository, logs);
    const deadline = Date.now() + config.maxHours * 60 * 60 * 1000;
    const signal = AbortSignal.timeout(config.maxHours * 60 * 60 * 1000);
    if (args.has("--retry") && state.active?.phase === "repair") {
      state.active.attempt = 0;
      await save();
    }
    while (Date.now() < deadline) {
      const issues = gh.issues();
      const final = issues.find((issue) => issue.number === 25);
      const prerequisitesClosed = config.issues.find((issue) => issue.number === 25).dependencies
        .every((number) => issues.find((issue) => issue.number === number)?.state === "closed");
      if (final?.state === "open") {
        const labels = final.labels.map((label) => label.name);
        if (prerequisitesClosed !== labels.includes("Sandcastle")) {
          gh.label(25, prerequisitesClosed ? [...labels, "Sandcastle"] : labels.filter((label) => label !== "Sandcastle"));
        }
      }
      const issue = selectIssue(config, issues, state.active);
      if (!issue) {
        const remaining = config.issues.filter((item) => issues.find((value) => value.number === item.number)?.state !== "closed");
        if (remaining.length) throw new Error(`No eligible issue; remaining tasks are blocked or not labelled: ${remaining.map((item) => `#${item.number}`).join(", ")}`);
        console.log("Spec #11 child tasks #12–#25 are complete. Parent issue is unchanged.");
        return;
      }
      const blockers = gh.blockers(issue.number);
      if (blockers.length) throw new Error(`#${issue.number} has open dependencies: ${blockers.map((value) => `#${value.number}`).join(", ")}`);
      const specHash = hash(issue.body);
      if (state.active && state.active.specHash !== specHash) throw new Error(`Criteria for #${issue.number} changed; review the saved task before resuming.`);
      if (!state.active) {
        state.active = { number: issue.number, base: git(root, "rev-parse", config.targetBranch), specHash,
          phase: "implement", attempt: 0, feedback: "" };
        await save();
      }
      const active = state.active;
      const plan = validationPlan(config, issue.number, issues.filter((value) => value.state === "closed").map((value) => value.number));
      const policyHash = hash(JSON.stringify(plan) + readFileSync(join(root, "Scripts/Validate-Windows.ps1"), "utf8"));
      if (active.policyHash && active.policyHash !== policyHash) {
        throw new Error(`Validation policy changed for #${issue.number}; saved evidence must be reviewed before resuming.`);
      }
      active.policyHash = policyHash;
      await save();
      const context = JSON.stringify({ issue: { number: issue.number, title: issue.title, body: issue.body },
        criteria: criteria(issue.body), validation: plan }, null, 2);
      const ensureSandbox = async () => {
        if (sandbox) return sandbox;
        let base = active.base;
        if (issue.number === 12) {
          try {
            const seed = git(root, "rev-parse", "--verify", `${config.seedBranch}^{commit}`);
            git(root, "merge-base", "--is-ancestor", base, seed);
            base = seed;
          } catch { /* A missing legacy branch starts from the target branch. */ }
        }
        sandbox = await sandcastle.createSandbox({
          cwd: root, branch: config.workBranch, baseBranch: base,
          sandbox: docker({ imageName: config.image, env: {
            GIT_ALLOW_PROTOCOL: "file", GIT_NO_LAZY_FETCH: "1",
            ...containerGitHubProxy(),
            PATH: "/tmp/editor-github-bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
          }, mounts: [
            { hostPath: join(root, ".sandcastle/codex-home"), sandboxPath: "/home/agent/.codex" },
            { hostPath: logs, sandboxPath: "/opt/editor-validation", readonly: true },
            { hostPath: join(validationWorktree, "Engine/RBVendor"), sandboxPath: "/opt/editor-dependencies", readonly: true },
            { hostPath: join(root, "docs"), sandboxPath: "/opt/editor-local-docs", readonly: true },
          ] }),
          hooks: { sandbox: { onSandboxReady: [{ command: githubProxyHook }, { command: "git config core.autocrlf true" }] } },
        });
        return sandbox;
      };
      const agentRun = async (name, promptFile) => {
        const worker = await ensureSandbox();
        const result = await worker.run({ name, maxIterations: 1, signal,
          agent: name === "reviewer" ? sandcastle.codex("gpt-6-luna", { effort: "high" }) : sandcastle.codex("gpt-6.1-sol", { effort: "medium" }),
          promptFile: join(root, ".sandcastle", promptFile),
          promptArgs: { TASK_CONTEXT: context, BASE: active.base, BRANCH: config.workBranch,
            VALIDATION: name === "reviewer" ? JSON.stringify(active.validation, null, 2) : "Pending implementation.",
            FEEDBACK: active.feedback || "No earlier failure. Complete the assigned task and its acceptance checks." },
          logging: { type: "file", path: join(logs, `issue-${issue.number}-attempt-${active.attempt}-${name}.log`) },
        });
        return result;
      };
      console.log(`Processing #${issue.number}: ${issue.title} (${active.phase})`);
      await runIssue(active, {
        save,
        implement: () => agentRun("implementer", "implement-prompt.md"),
        candidate: async () => {
          const status = await sandbox.exec("git status --porcelain --untracked-files=all");
          if (status.exitCode !== 0 || status.stdout.trim()) throw new Error(`Implementer left uncommitted files: ${status.stdout}`);
          const sha = git(root, "rev-parse", config.workBranch);
          git(root, "merge-base", "--is-ancestor", active.base, sha);
          if (sha === active.base) throw new Error("Task produced no committed change; completion has not been demonstrated.");
          const forbidden = git(root, "diff", "--name-only", active.base, sha, "--", ".sandcastle", "Scripts/Validate-Windows.ps1", "package.json", "package-lock.json", ".gitmodules", "Vendor", "Engine/RBVendor");
          if (forbidden) throw new Error(`Task changed orchestration or dependencies outside its scope:\n${forbidden}`);
          return sha;
        },
        review: async () => {
          const report = readJson(active.validation.reportPath);
          if (report.status !== "passed" || report.commit !== active.sha || git(root, "rev-parse", config.workBranch) !== active.sha) {
            throw new Error("Saved Windows evidence or branch no longer matches the review candidate.");
          }
          const result = await agentRun("reviewer", "review-prompt.md");
          const status = await sandbox.exec("git status --porcelain --untracked-files=all");
          if (status.exitCode !== 0 || status.stdout.trim()) throw new Error(`Reviewer left uncommitted files: ${status.stdout}`);
          if (git(root, "rev-parse", config.workBranch) !== active.sha) {
            throw new Error("Reviewer changed the validated commit; implementation and Windows validation must run again.");
          }
          return parseReview(result.stdout, issue, active.sha);
        },
        validate: () => runWindows(root, validationWorktree, active.sha, plan,
          join(logs, `issue-${issue.number}-${active.sha}-attempt-${active.attempt}`)),
        merge: async () => {
          if (hash(gh.issue(issue.number).body) !== active.specHash) throw new Error("Issue criteria changed after validation.");
          const report = readJson(active.validation.reportPath);
          if (report.status !== "passed" || report.commit !== active.sha) throw new Error("Saved evidence does not match the reviewed commit.");
          await safeMerge(root, config.targetBranch, active, state, save);
        },
        publish: () => {
          git(root, "merge-base", "--is-ancestor", active.sha, config.targetBranch);
          gh.publish(active);
        },
        close: () => gh.close(issue.number),
      }, config.maxAttempts);
      if (!state.completed.includes(issue.number)) state.completed.push(issue.number);
      state.active = null;
      await save();
    }
    throw new Error("Run time limit reached; progress is saved for the next invocation.");
  } finally {
    try {
      if (sandbox) {
        const closed = await sandbox.close();
        if (closed.preservedWorktreePath) console.log(`Preserved agent worktree: ${closed.preservedWorktreePath}`);
      }
    } finally {
      closeSync(lock);
      unlinkSync(lockPath);
    }
  }
}
