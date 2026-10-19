import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, mkdirSync, readFileSync, writeFileSync, realpathSync, rmSync, appendFileSync } from "node:fs";
import { join, sep, basename } from "node:path";
import { tmpdir } from "node:os";
import { config } from "./workflow-config.mjs";
import { criteria, selectIssue, validationPlan, parseReview, runIssue } from "./workflow.mjs";
import { git, safeMerge, prepareValidation, proxyEnvironment, containerGitHubProxy } from "./host.mjs";

function fixture(overrides = {}) {
  const calls = [];
  const active = { number: 12, phase: "implement", attempt: 0 };
  const ports = {
    save: async () => {},
    implement: async () => calls.push("implement"),
    candidate: async () => "reviewed",
    review: async () => { calls.push("review"); return { sha: "reviewed" }; },
    validate: async () => { calls.push("validate"); return { status: "passed", commit: "reviewed" }; },
    merge: async () => calls.push("merge"),
    publish: async () => calls.push("publish"),
    close: async () => calls.push("close"),
    ...overrides,
  };
  return { active, ports, calls };
}

test("acceptance precedes merge, evidence and closure", async () => {
  const f = fixture();
  await runIssue(f.active, f.ports, 3);
  assert.deepEqual(f.calls, ["implement", "validate", "review", "merge", "publish", "close"]);
  assert.equal(f.active.phase, "closed");
});

test("runtime failure returns through implementation and review", async () => {
  const f = fixture();
  let runs = 0;
  f.ports.validate = async () => {
    f.calls.push("validate");
    if (++runs === 1) throw new Error("Release UI failure");
    return { status: "passed", commit: "reviewed" };
  };
  await runIssue(f.active, f.ports, 3);
  assert.deepEqual(f.calls, ["implement", "validate", "implement", "validate", "review", "merge", "publish", "close"]);
});

test("repair exhaustion retains feedback and cannot merge or close", async () => {
  const f = fixture({ validate: async () => { throw new Error("missing UI check"); } });
  await assert.rejects(runIssue(f.active, f.ports, 2), /missing UI check/);
  assert.equal(f.active.attempt, 2);
  assert.equal(f.active.phase, "repair");
  assert.equal(f.active.feedback, "missing UI check");
  assert.ok(!f.calls.includes("merge") && !f.calls.includes("close"));
  await assert.rejects(runIssue(f.active, f.ports, 2), /Repair limit/);
});

test("success for another commit cannot pass", async () => {
  const f = fixture({ validate: async () => ({ status: "passed", commit: "old" }) });
  await assert.rejects(runIssue(f.active, f.ports, 1), /reviewed commit/);
  assert.ok(!f.calls.includes("merge"));
});

test("review rejection after validation prevents merge", async () => {
  const f = fixture({ review: async () => { throw new Error("uncovered criterion"); } });
  await assert.rejects(runIssue(f.active, f.ports, 1), /uncovered/);
  assert.deepEqual(f.calls, ["implement", "validate"]);
});

test("review receives current Windows evidence rather than stale failed evidence", async () => {
  const f = fixture();
  f.active.validation = { status: "passed", commit: "old" };
  f.ports.review = async (active) => {
    assert.equal(active.validation.commit, "reviewed");
    assert.equal(active.validation.status, "passed");
    return { sha: active.sha };
  };
  await runIssue(f.active, f.ports, 1);
  assert.equal(f.active.phase, "closed");
});

test("review cannot approve a commit other than the validated candidate", async () => {
  const f = fixture({ review: async () => ({ sha: "changed" }) });
  await assert.rejects(runIssue(f.active, f.ports, 1), /reviewed commit/);
  assert.ok(!f.calls.includes("merge"));
});

test("a saved Windows pass resumes review without implementation or rebuilding", async () => {
  const f = fixture();
  Object.assign(f.active, { phase: "review", attempt: 3, sha: "reviewed",
    validation: { status: "passed", commit: "reviewed" } });
  await runIssue(f.active, f.ports, 3);
  assert.deepEqual(f.calls, ["review", "merge", "publish", "close"]);
});

test("resumed review rejects mismatched Windows evidence", async () => {
  const f = fixture();
  Object.assign(f.active, { phase: "review", attempt: 3, sha: "reviewed",
    validation: { status: "passed", commit: "old" } });
  await assert.rejects(runIssue(f.active, f.ports, 3), /reviewed commit/);
  assert.deepEqual(f.calls, []);
});

test("merge failure preserves validated phase without publishing", async () => {
  const f = fixture({ merge: async () => { throw new Error("local edit conflict"); } });
  await assert.rejects(runIssue(f.active, f.ports, 3), /local edit conflict/);
  assert.equal(f.active.phase, "validated");
  assert.equal(f.active.attempt, 1);
  assert.ok(!f.calls.includes("publish"));
});

test("GitHub failure resumes publication without agents or merge", async () => {
  const f = fixture({ close: async () => { throw new Error("TLS timeout"); } });
  await assert.rejects(runIssue(f.active, f.ports, 3), /TLS/);
  assert.equal(f.active.phase, "merged");
  f.calls.length = 0;
  f.ports.close = async () => f.calls.push("close");
  await runIssue(f.active, f.ports, 3);
  assert.deepEqual(f.calls, ["publish", "close"]);
});

const issueList = () => config.issues.map((item) => ({ number: item.number, state: "open", labels: [{ name: "Sandcastle" }] }));
test("container gh translates host loopback without changing model proxy variables", () => {
  assert.deepEqual(containerGitHubProxy({ HTTPS_PROXY: "http://127.0.0.1:18081" }), {
    SANDCASTLE_GITHUB_PROXY: "http://host.docker.internal:18081/",
  });
  assert.deepEqual(containerGitHubProxy({}), {});
  assert.equal(containerGitHubProxy({ https_proxy: "http://proxy.example:8080" }).SANDCASTLE_GITHUB_PROXY,
    "http://proxy.example:8080/");
});
test("host GitHub proxy follows Windows settings but preserves explicit overrides", () => {
  assert.deepEqual(proxyEnvironment("127.0.0.1:18081", {}), {
    HTTP_PROXY: "http://127.0.0.1:18081", HTTPS_PROXY: "http://127.0.0.1:18081",
  });
  const specific = proxyEnvironment("http=local:80;https=secure:81", { HTTPS_PROXY: "http://explicit:82" });
  assert.equal(specific.HTTP_PROXY, "http://local:80");
  assert.equal(specific.HTTPS_PROXY, "http://explicit:82");
});
test("dependency ordering handles the unlabelled #12 handoff and defers #25", () => {
  const issues = issueList();
  issues[0].labels = [];
  assert.equal(selectIssue(config, issues).number, 12);
  issues[0].state = "closed";
  assert.equal(selectIssue(config, issues).number, 13);
  issues.slice(1, -1).forEach((issue) => { issue.state = "closed"; });
  issues.at(-1).labels = [];
  assert.equal(selectIssue(config, issues).number, 25);
  issues.find((issue) => issue.number === 20).state = "open";
  assert.equal(selectIssue(config, issues).number, 20);
});

test("missing GitHub data cannot be interpreted as completion", () => {
  assert.throws(() => selectIssue(config, issueList().slice(1)), /did not return/);
});

test("final task reruns the combined regression and UI checks", () => {
  const plan = validationPlan(config, 25, config.issues.slice(0, -1).map((item) => item.number));
  assert.ok(plan.selectors.includes("scene-hooks") && plan.selectors.includes("gl-buffers"));
  assert.equal(plan.windowsChecks.length, 5);
  assert.ok(plan.windowsChecks.some((item) => item.script === "Tests/Windows/issue-15.ps1"));
});

test("review requires matching SHA and every criterion exactly once", () => {
  const issue = { number: 12, body: "- [ ] compile\n- [x] runtime" };
  assert.equal(criteria(issue.body).length, 2);
  const result = { issue: 12, sha: "abc", approved: true,
    criteria: [{ id: "C1", satisfied: true, evidence: "Build" }, { id: "C2", satisfied: true, evidence: "Runtime" }] };
  const encode = () => `<review>${JSON.stringify(result)}</review>`;
  assert.equal(parseReview(encode(), issue, "abc").sha, "abc");
  assert.throws(() => parseReview(encode(), issue, "def"), /final commit/);
  result.criteria[1].id = "C1";
  assert.throws(() => parseReview(encode(), issue, "abc"), /exactly once/);
  assert.throws(() => parseReview("COMPLETE", issue, "abc"), /<review>/);
});

function repository(t) {
  const root = mkdtempSync(join(tmpdir(), "editor-workflow-test-"));
  t.after(() => {
    const resolved = realpathSync(root);
    if (!resolved.startsWith(realpathSync(tmpdir()) + sep) || !basename(resolved).startsWith("editor-workflow-test-")) {
      throw new Error("Refusing to remove an unexpected test directory.");
    }
    rmSync(resolved, { recursive: true, force: true });
  });
  git(root, "init", "-b", "Editor");
  git(root, "config", "user.name", "Workflow Test");
  git(root, "config", "user.email", "workflow-test@example.invalid");
  git(root, "config", "core.autocrlf", "false");
  for (const name of ["local.txt", "staged.txt"]) writeFileSync(join(root, name), "original\n");
  git(root, "add", ".");
  git(root, "commit", "-m", "base");
  const base = git(root, "rev-parse", "HEAD");
  git(root, "switch", "-c", "reviewed");
  writeFileSync(join(root, "feature.txt"), "validated feature\n");
  git(root, "add", ".");
  git(root, "commit", "-m", "feature");
  const sha = git(root, "rev-parse", "HEAD");
  git(root, "switch", "Editor");
  return { root, active: { number: 12, base, sha } };
}

test("real merge preserves staged, unstaged and untracked edits", async (t) => {
  const { root, active } = repository(t);
  writeFileSync(join(root, "local.txt"), "unsaved local edit\n");
  writeFileSync(join(root, "staged.txt"), "staged local edit\n");
  git(root, "add", "staged.txt");
  writeFileSync(join(root, "draft.txt"), "untracked draft\n");
  const beforeIndex = git(root, "diff", "--cached");
  const state = {};
  await safeMerge(root, "Editor", active, state, async () => {});
  assert.equal(git(root, "rev-parse", "HEAD"), active.sha);
  assert.equal(readFileSync(join(root, "local.txt"), "utf8"), "unsaved local edit\n");
  assert.equal(readFileSync(join(root, "draft.txt"), "utf8"), "untracked draft\n");
  assert.equal(git(root, "diff", "--cached"), beforeIndex);
  assert.equal(state.pendingStash, null);
  assert.equal(state.preservedStashes.length, 1);
});

test("untracked collision refuses merge and restores tracked work", async (t) => {
  const { root, active } = repository(t);
  writeFileSync(join(root, "local.txt"), "user work\n");
  writeFileSync(join(root, "feature.txt"), "user draft\n");
  const state = {};
  await assert.rejects(safeMerge(root, "Editor", active, state, async () => {}), /failed/);
  assert.equal(git(root, "rev-parse", "HEAD"), active.base);
  assert.equal(readFileSync(join(root, "local.txt"), "utf8"), "user work\n");
  assert.equal(readFileSync(join(root, "feature.txt"), "utf8"), "user draft\n");
});

test("validation cannot repurpose the main checkout", (t) => {
  const { root, active } = repository(t);
  assert.throws(() => prepareValidation(root, root, active.sha), /separate/);
});

test("validation reuses the checkout and retains ignored build caches", (t) => {
  const { root, active } = repository(t);
  const worktree = join(root, "validation");
  git(root, "worktree", "add", "--detach", worktree, active.base);
  appendFileSync(join(root, ".git/info/exclude"), "\nBinaries/\n");
  mkdirSync(join(worktree, "Binaries"));
  writeFileSync(join(worktree, "Binaries/cache"), "retained");
  prepareValidation(root, worktree, active.sha);
  assert.equal(git(worktree, "rev-parse", "HEAD"), active.sha);
  assert.equal(readFileSync(join(worktree, "Binaries/cache"), "utf8"), "retained");
  assert.equal(git(root, "rev-parse", "HEAD"), active.base);
});

test("restoration conflict retains the original edits and recovery checkpoint", async (t) => {
  const { root, active } = repository(t);
  git(root, "switch", "reviewed");
  writeFileSync(join(root, "local.txt"), "incoming change\n");
  git(root, "commit", "-am", "conflicting incoming change");
  active.sha = git(root, "rev-parse", "HEAD");
  git(root, "switch", "Editor");
  writeFileSync(join(root, "local.txt"), "user original work\n");
  const state = {};
  await assert.rejects(safeMerge(root, "Editor", active, state, async () => {}), /failed/);
  assert.match(state.pendingStash, /^[a-f0-9]{40}$/);
  assert.equal(git(root, "show", `${state.pendingStash}:local.txt`), "user original work");
  assert.ok(git(root, "diff", "--name-only", "--diff-filter=U").includes("local.txt"));
});
