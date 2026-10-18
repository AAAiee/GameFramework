import { execFileSync, spawn } from "node:child_process";
import { existsSync, mkdirSync, readFileSync, writeFileSync, renameSync, realpathSync } from "node:fs";
import { dirname, join, delimiter } from "node:path";

export function configureTools() {
  const directories = [
    dirname(process.execPath),
    join(process.env.ProgramFiles ?? "C:/Program Files", "GitHub CLI"),
    join(process.env.LOCALAPPDATA ?? "", "Programs/DockerDesktop/resources/bin"),
    join(process.env.ProgramFiles ?? "C:/Program Files", "Docker/Docker/resources/bin"),
    join(process.env.ProgramFiles ?? "C:/Program Files", "PowerShell/7"),
  ].filter(existsSync);
  process.env.PATH = [...directories, process.env.PATH ?? ""].join(delimiter);
  process.env.GIT_ALLOW_PROTOCOL = "file";
  process.env.GIT_NO_LAZY_FETCH = "1";
}

export function command(executable, args, cwd, options = {}) {
  try {
    return execFileSync(executable, args, {
      cwd, encoding: "utf8", windowsHide: true, timeout: 45_000,
      maxBuffer: 16 * 1024 * 1024, stdio: ["ignore", "pipe", "pipe"], ...options,
    }).trim();
  } catch (error) {
    const failure = new Error(`${executable} ${args.join(" ")} failed: ${String(error.stderr || error.stdout || error.message).slice(-12000)}`, { cause: error });
    failure.code = error.code;
    throw failure;
  }
}

export const git = (cwd, ...args) => command("git", args, cwd);
export const readJson = (path) => JSON.parse(readFileSync(path, "utf8").replace(/^\uFEFF/, ""));
export function proxyEnvironment(server, environment = process.env) {
  const result = { ...environment };
  const entries = server.split(";").map((entry) => entry.trim()).filter(Boolean);
  for (const scheme of ["http", "https"]) {
    if (result[`${scheme.toUpperCase()}_PROXY`] || result[`${scheme}_proxy`]) continue;
    const entry = entries.find((value) => value.startsWith(`${scheme}=`));
    const address = entry ? entry.slice(scheme.length + 1) : entries.find((value) => !value.includes("="));
    if (address) result[`${scheme.toUpperCase()}_PROXY`] = address.includes("://") ? address : `http://${address}`;
  }
  return result;
}

export function githubEnvironment() {
  if (process.platform !== "win32") return process.env;
  try {
    const server = command("pwsh.exe", ["-NoProfile", "-Command",
      "$p = Get-ItemProperty 'HKCU:\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings'; if ($p.ProxyEnable -eq 1) { [Console]::Write($p.ProxyServer) }"], process.cwd());
    return proxyEnvironment(server);
  } catch { return process.env; }
}

export function containerGitHubProxy(environment = githubEnvironment()) {
  const address = environment.HTTPS_PROXY || environment.https_proxy || environment.HTTP_PROXY || environment.http_proxy;
  if (!address) return {};
  const proxy = new URL(address);
  if (["localhost", "127.0.0.1", "[::1]"].includes(proxy.hostname)) proxy.hostname = "host.docker.internal";
  return { SANDCASTLE_GITHUB_PROXY: proxy.toString() };
}

// Only gh receives proxy variables; model traffic keeps its existing route.
export const githubProxyHook = `mkdir -p /tmp/editor-github-bin && printf '%s\\n' '#!/bin/sh' 'if [ -n "$SANDCASTLE_GITHUB_PROXY" ]; then' 'export HTTPS_PROXY="$SANDCASTLE_GITHUB_PROXY" HTTP_PROXY="$SANDCASTLE_GITHUB_PROXY"' 'fi' 'exec /usr/bin/gh "$@"' > /tmp/editor-github-bin/gh && chmod +x /tmp/editor-github-bin/gh`;

export function writeJson(path, data) {
  mkdirSync(dirname(path), { recursive: true });
  writeFileSync(`${path}.tmp`, `${JSON.stringify(data, null, 2)}\n`);
  renameSync(`${path}.tmp`, path);
}

export function assertClean(root) {
  const status = git(root, "status", "--porcelain", "--untracked-files=all");
  if (status) throw new Error(`Preserving changes in ${root}; checkout must be clean:\n${status}`);
}

export function prepareValidation(root, worktree, sha) {
  if (realpathSync(root).toLowerCase() === realpathSync(worktree).toLowerCase()) {
    throw new Error("Validation must use its separate prepared worktree.");
  }
  const common = (path) => realpathSync(git(path, "rev-parse", "--path-format=absolute", "--git-common-dir")).toLowerCase();
  if (common(root) !== common(worktree)) throw new Error("Validation worktree belongs to a different repository.");
  assertClean(worktree);
  git(worktree, "-c", "submodule.recurse=false", "checkout", "--detach", sha);
}

export async function safeMerge(root, targetBranch, active, state, save) {
  if (state.pendingStash) {
    throw new Error(`Saved edits need recovery before continuing: ${state.pendingStash}. Inspect git status; the stash is retained.`);
  }
  if (git(root, "branch", "--show-current") !== targetBranch) throw new Error(`Return the main checkout to ${targetBranch}.`);
  const current = git(root, "rev-parse", "HEAD");
  if (current === active.sha) return;
  if (current !== active.base) throw new Error("Target branch changed during this task; revalidate against the new base.");
  git(root, "merge-base", "--is-ancestor", current, active.sha);
  if (git(root, "diff", "--name-only", "--diff-filter=U")) throw new Error("Resolve existing merge conflicts before continuing.");
  const dirty = git(root, "status", "--porcelain", "--untracked-files=no");
  if (dirty) {
    state.pendingStash = `Backup creation interrupted before #${active.number}; inspect the named entry in git stash list`;
    await save();
    git(root, "stash", "push", "-m", `Sandcastle preserved edits before #${active.number}`);
    state.pendingStash = git(root, "rev-parse", "refs/stash");
    await save();
  }
  let mergeError;
  try { git(root, "-c", "submodule.recurse=false", "merge", "--ff-only", active.sha); }
  catch (error) { mergeError = error; }
  if (state.pendingStash) {
    // apply retains the recovery copy; never pop/drop a user's saved changes.
    git(root, "stash", "apply", "--index", state.pendingStash);
    state.preservedStashes ??= [];
    state.preservedStashes.push(state.pendingStash);
    state.pendingStash = null;
    await save();
  }
  if (mergeError) throw mergeError;
}

export async function runWindows(root, worktree, sha, plan, directory) {
  prepareValidation(root, worktree, sha);
  mkdirSync(directory, { recursive: true });
  const planPath = join(directory, "plan.json");
  const reportPath = join(directory, "results.json");
  writeJson(planPath, plan);
  const args = ["-NoProfile", "-File", join(root, "Scripts/Validate-Windows.ps1"),
    "-Worktree", worktree, "-PlanPath", planPath, "-ReportPath", reportPath];
  const exitCode = await new Promise((done, reject) => {
    const child = spawn("pwsh.exe", args, { cwd: root, stdio: "inherit", windowsHide: true });
    child.on("error", reject);
    child.on("exit", (code) => done(code));
  });
  const report = existsSync(reportPath) ? readJson(reportPath) : null;
  if (exitCode !== 0 || report?.status !== "passed" || report.commit !== sha) {
    const details = report?.checks?.filter((check) => check.exitCode !== 0 || check.timedOut || !check.markerPresent)
      .map((check) => `${check.name}:\n${readFileSync(check.log, "utf8").slice(-16000)}`).join("\n") ?? "";
    throw new Error(`Windows validation failed for ${sha}. Logs: ${directory}\n${details}\n${existsSync(join(directory, "results.md")) ? readFileSync(join(directory, "results.md"), "utf8") : "No report produced."}`);
  }
  assertClean(worktree);
  return { ...report, reportPath };
}

export function transientGitHubError(error) {
  const code = error.code ?? error.cause?.code;
  if (["ETIMEDOUT", "ECONNRESET", "ECONNREFUSED", "EAI_AGAIN", "ENETUNREACH", "EHOSTUNREACH"].includes(code)) return true;
  return /ETIMEDOUT|TLS handshake timeout|context deadline exceeded|i\/o timeout|unexpected EOF|connection (?:reset|refused)|connection was forcibly closed|connection was aborted|temporary failure in name resolution|HTTP (?:429|500|502|503|504)\b/i.test(error.message);
}

export function github(root, repository, artifactDirectory, options = {}) {
  // Go's HTTP client does not automatically use Windows' system proxy. Keep this
  // host-only so a loopback proxy address is not injected into the container.
  const env = options.env ?? githubEnvironment();
  const execute = options.command ?? command;
  const timeout = options.timeout ?? 120_000;
  const attempts = options.attempts ?? 3;
  const warn = options.warn ?? console.warn;
  const sleep = options.sleep ?? ((milliseconds) => Atomics.wait(new Int32Array(new SharedArrayBuffer(4)), 0, 0, milliseconds));
  const backoff = (attempt) => sleep(2000 * 2 ** attempt);
  const api = (path, args = [], retry = true) => {
    for (let attempt = 0; attempt < attempts; attempt++) {
      try {
        return execute("gh.exe", ["api", `repos/${repository}/${path}`, ...args], root, { env, timeout });
      } catch (error) {
        if (!retry || !transientGitHubError(error) || attempt + 1 === attempts) throw error;
        warn(`GitHub request ${path} interrupted; retry ${attempt + 2}/${attempts} after backoff.`);
        backoff(attempt);
      }
    }
  };
  const json = (path, args, retry) => JSON.parse(api(path, args, retry));
  const write = (path, method, body, retry = true) => {
    const input = join(artifactDirectory, "github-request.json");
    writeJson(input, body);
    return json(path, ["--method", method, "--input", input], retry);
  };
  return {
    issues: () => json("issues?state=all&per_page=100", ["--paginate", "--slurp"]).flat().filter((issue) => !issue.pull_request),
    issue: (number) => json(`issues/${number}`),
    blockers: (number) => json(`issues/${number}/dependencies/blocked_by?per_page=100`, ["--paginate", "--slurp"]).flat().filter((issue) => issue.state === "open"),
    label: (number, labels) => write(`issues/${number}`, "PATCH", { labels }),
    publish: (active) => {
      const marker = `<!-- sandcastle:${active.number}:${active.sha} -->`;
      const report = readFileSync(active.validation.report, "utf8");
      const review = active.review.criteria.map((item) => `- ${item.id}: ${item.evidence}`).join("\n");
      const body = {
        body: `${marker}\nMerged locally into Editor at ${active.sha} after review and Windows acceptance.\n\n${review}\n\nValidation commands and outcomes:\n\n${report}`,
      };
      let lastError;
      for (let attempt = 0; attempt <= attempts; attempt++) {
        // A lost POST response can still mean the comment was committed. Always
        // reconcile by its marker before re-sending, including after the last try.
        const comments = json(`issues/${active.number}/comments?per_page=100`, ["--paginate", "--slurp"]).flat();
        if (comments.some((comment) => comment.body.includes(marker))) return;
        if (attempt === attempts) throw lastError;
        try {
          write(`issues/${active.number}/comments`, "POST", body, false);
          return;
        } catch (error) {
          if (!transientGitHubError(error)) throw error;
          lastError = error;
          warn(`GitHub comment #${active.number} response interrupted; checking its commit marker before another POST.`);
          backoff(attempt);
        }
      }
    },
    close: (number) => {
      const issue = json(`issues/${number}`);
      write(`issues/${number}`, "PATCH", { state: "closed", state_reason: "completed",
        labels: issue.labels.map((label) => label.name).filter((name) => !["Sandcastle", "ready-for-human"].includes(name)) });
    },
  };
}
