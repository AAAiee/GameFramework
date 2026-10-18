import test from "node:test";
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { existsSync, mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import { createSandbox } from "@ai-hero/sandcastle";
import { createCommandRunner } from "@rebuild/sandcastle/workflow/integrations";
import { fixture, git, success } from "./test-fixture.mjs";
import { runEditorTask } from "./task.mjs";

const task = {
  id: "fixture-task",
  criteria: [{ id: "behavior", description: "The saved value is doubled" }],
};
const read = (path) => JSON.parse(readFileSync(path, "utf8"));

async function editorFixture(
  t,
  {
    rejectValidation = false,
    interruptReview = false,
    preparedValidation = false,
  } = {},
) {
  let invocations = 0;
  const f = fixture(t, async (worktree) => {
    invocations++;
    if (invocations === 1) {
      writeFileSync(
        join(worktree, "value.cjs"),
        "module.exports = value => value * 2;\n",
      );
      writeFileSync(
        join(worktree, "value.test.cjs"),
        "const test = require('node:test'); const assert = require('node:assert/strict'); test('doubles the input', () => assert.equal(require('./value.cjs')(21), 42));\n",
      );
      git(worktree, "add", "value.cjs", "value.test.cjs");
      git(worktree, "commit", "-m", "Implement fixture behavior");
      return success("Implementation committed");
    }
    if (interruptReview && invocations === 2)
      return {
        stdout: "",
        stderr: "Unfamiliar reviewer interruption",
        exitCode: 9,
      };
    return success(
      JSON.stringify({
        candidate: git(worktree, "rev-parse", "HEAD"),
        approved: true,
        criteria: [
          {
            id: "behavior",
            satisfied: true,
            evidence: "Reviewed value.cjs and its actual host assertion",
          },
        ],
      }),
    );
  });
  const sandbox = await createSandbox(f.options("task/editor-adapter"));
  t.after(async () => {
    await sandbox.close();
  });
  let validations = 0;
  const options = {
    projectId: "fixture-editor-project",
    task,
    sandbox,
    agents: { implementer: f.runOptions.agent, reviewer: f.runOptions.agent },
    prompts: {
      implement: "Implement the supplied task and commit locally.",
      review: "Independently inspect the candidate and evidence.",
    },
    storage: join(f.root, ".sandcastle/logs/task"),
    validate: async ({ worktree, candidate }) => {
      validations++;
      const environment = { ...process.env };
      delete environment.NODE_TEST_CONTEXT;
      const output = execFileSync(
        process.execPath,
        ["--test", "value.test.cjs"],
        {
          cwd: worktree,
          encoding: "utf8",
          windowsHide: true,
          env: environment,
        },
      );
      assert.match(output, /# pass 1/);
      return {
        candidate,
        passed: !rejectValidation,
        evidence: ["Host node --test value.test.cjs passed"],
      };
    },
  };
  if (preparedValidation) {
    const prepared = join(f.root, ".sandcastle/native-validation");
    git(
      f.root,
      "worktree",
      "add",
      "-b",
      "native-validation",
      prepared,
      f.before,
    );
    writeFileSync(join(prepared, "source.txt"), "staged validation edit\n");
    git(prepared, "add", "source.txt");
    writeFileSync(
      join(prepared, "source.txt"),
      "staged plus unstaged validation edit\n",
    );
    writeFileSync(join(prepared, "notes.txt"), "untracked validation notes\n");
    mkdirSync(join(prepared, ".sandcastle/cache"), { recursive: true });
    writeFileSync(
      join(prepared, ".sandcastle/cache/native.bin"),
      "prepared dependency\n",
    );
    options.validationWorktree = {
      source: f.root,
      path: prepared,
      command: createCommandRunner({ cwd: f.root }),
    };
  }
  return { f, options, counts: () => ({ invocations, validations }) };
}

test("Editor adapter uses the packaged workflow with the original SDK and hands off only locally", async (t) => {
  const { f, options, counts } = await editorFixture(t);
  const result = await runEditorTask(options);
  assert.equal(result.phase, "ready");
  assert.deepEqual(counts(), { invocations: 2, validations: 1 });
  const handoff = read(result.delivery.reference);
  assert.equal(
    handoff.candidate,
    git(f.root, "rev-parse", "task/editor-adapter"),
  );
  assert.equal(handoff.review.criteria[0].satisfied, true);
  assert.equal(git(f.root, "rev-parse", "HEAD"), f.before);
  assert.equal(git(f.root, "remote"), "");
  assert.equal(readFileSync(join(f.root, "source.txt"), "utf8"), "original\n");
  await runEditorTask(options);
  assert.deepEqual(counts(), { invocations: 2, validations: 1 });
});

test("Editor host rejection prevents reviewer invocation and local handoff", async (t) => {
  const { options, counts } = await editorFixture(t, {
    rejectValidation: true,
  });
  await assert.rejects(runEditorTask(options), { code: "gate" });
  assert.deepEqual(counts(), { invocations: 1, validations: 1 });
  assert.equal(
    read(join(options.storage, "checkpoint.json")).phase,
    "validate",
  );
  assert.equal(existsSync(join(options.storage, "handoff.json")), false);
});

test("Editor reloads a review checkpoint without repeating implementation or host validation", async (t) => {
  const { options, counts } = await editorFixture(t, { interruptReview: true });
  await assert.rejects(
    runEditorTask(options),
    /Unfamiliar reviewer interruption/,
  );
  assert.equal(read(join(options.storage, "checkpoint.json")).phase, "review");
  assert.equal((await runEditorTask(options)).phase, "ready");
  assert.deepEqual(counts(), { invocations: 3, validations: 1 });
});

test("Editor validates in a separate prepared worktree and restores its edits and cache", async (t) => {
  const { f, options, counts } = await editorFixture(t, {
    preparedValidation: true,
  });
  const prepared = options.validationWorktree.path;
  const snapshot = () => ({
    head: git(prepared, "rev-parse", "HEAD"),
    branch: git(prepared, "branch", "--show-current"),
    staged: git(prepared, "diff", "--cached", "--binary"),
    unstaged: git(prepared, "diff", "--binary"),
    status: git(prepared, "status", "--porcelain"),
  });
  const before = snapshot();
  const result = await runEditorTask(options);
  assert.equal(result.phase, "ready");
  assert.deepEqual(counts(), { invocations: 2, validations: 1 });
  assert.deepEqual(snapshot(), before);
  assert.equal(
    readFileSync(join(prepared, "notes.txt"), "utf8"),
    "untracked validation notes\n",
  );
  assert.equal(
    readFileSync(join(prepared, ".sandcastle/cache/native.bin"), "utf8"),
    "prepared dependency\n",
  );
  const recovery = read(join(options.storage, "validation-recovery.json"));
  assert.equal(recovery.phase, "restored");
  assert.match(recovery.stash, /^[a-f0-9]{40}$/);
  assert.equal(recovery.candidate, result.candidate);
  assert.equal(git(f.root, "rev-parse", "HEAD"), f.before);
});

test("Editor preserves an unresolved worktree recovery record on validation resume", async (t) => {
  const { options, counts } = await editorFixture(t, {
    rejectValidation: true,
    preparedValidation: true,
  });
  await assert.rejects(runEditorTask(options), { code: "gate" });
  const path = join(options.storage, "validation-recovery.json");
  const unresolved = { ...read(path), phase: "restoration-required" };
  writeFileSync(path, JSON.stringify(unresolved));
  await assert.rejects(runEditorTask(options), /recovery is unresolved/);
  assert.deepEqual(read(path), unresolved);
  assert.deepEqual(counts(), { invocations: 1, validations: 1 });
});
