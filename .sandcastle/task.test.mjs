import test from "node:test";
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { createHash } from "node:crypto";
import { existsSync, mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import { createSandbox } from "@ai-hero/sandcastle";
import { WorkflowOperationError } from "@rebuild/sandcastle/workflow";
import { createCommandRunner } from "@rebuild/sandcastle/workflow/integrations";
import { fixture, git, success } from "./test-fixture.mjs";
import { runEditorTask } from "./task.mjs";

const task = {
  id: "fixture-task",
  criteria: [{ id: "behavior", description: "The saved value is doubled" }],
};
const read = (path) => JSON.parse(readFileSync(path, "utf8")).value;
const snapshot = (prepared) => ({
  head: git(prepared, "rev-parse", "HEAD"),
  branch: git(prepared, "branch", "--show-current"),
  staged: git(prepared, "diff", "--cached", "--binary"),
  unstaged: git(prepared, "diff", "--binary"),
  status: git(prepared, "status", "--porcelain"),
});

async function editorFixture(
  t,
  {
    rejectValidation = false,
    interruptReview = false,
    preparedValidation = false,
    measuredCheck = false,
    reportedUsage = false,
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
      const result = success("Implementation committed");
      if (reportedUsage)
        result.stdout +=
          "\n" +
          JSON.stringify({
            type: "turn.completed",
            usage: {
              input_tokens: 100,
              cached_input_tokens: 80,
              output_tokens: 7,
            },
          });
      return result;
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
  let validations = 0;
  const options = {
    projectId: "fixture-editor-project",
    task,
    runtime: {
      repository: f.root,
      runId: "fixture-task-run",
      compatibility: "fixture-dependencies-v1",
      command: createCommandRunner({ cwd: f.root }),
    },
    openSandbox: async ({ runtime, branch }) => {
      options.storage = join(runtime.directory, "records");
      options.branch = branch;
      return createSandbox(f.options(branch));
    },
    agents: { implementer: f.runOptions.agent, reviewer: f.runOptions.agent },
    prompts: {
      implement: "Implement the supplied task and commit locally.",
      review: "Independently inspect the candidate and evidence.",
    },
    validate: async ({ worktree, candidate, measureCheck }) => {
      validations++;
      if (measuredCheck) {
        return measureCheck("host-node-value", async () => {
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
        });
      }
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
    options.prepareValidation = async ({ directory }) => {
      const prepared = join(directory, "worktree");
      if (!existsSync(prepared)) {
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
        writeFileSync(
          join(prepared, "notes.txt"),
          "untracked validation notes\n",
        );
        mkdirSync(join(prepared, ".sandcastle/cache"), { recursive: true });
        writeFileSync(
          join(prepared, ".sandcastle/cache/native.bin"),
          "prepared dependency\n",
        );
      }
      options.validationWorktree = {
        source: f.root,
        path: prepared,
        command: createCommandRunner({ cwd: f.root }),
      };
      options.validationBefore = snapshot(prepared);
      return options.validationWorktree;
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
  assert.equal(handoff.candidate, git(f.root, "rev-parse", options.branch));
  assert.equal(handoff.review.criteria[0].satisfied, true);
  assert.equal(git(f.root, "rev-parse", "HEAD"), f.before);
  assert.equal(git(f.root, "remote"), "");
  assert.equal(readFileSync(join(f.root, "source.txt"), "utf8"), "original\n");
  await runEditorTask(options);
  assert.deepEqual(counts(), { invocations: 2, validations: 1 });
});

test("Editor enables durable transport recovery explicitly and reports actual code/transport attempts", async (t) => {
  const { options, counts } = await editorFixture(t);
  const validate = options.validate;
  let interrupted = false;
  options.recovery = { wait: async () => {} };
  options.validate = async (context) => {
    if (!interrupted) {
      interrupted = true;
      throw new WorkflowOperationError(
        "transport",
        "Fixture host service disconnected before validation",
      );
    }
    return validate(context);
  };
  const result = await runEditorTask(options);
  assert.equal(result.phase, "ready");
  assert.deepEqual(counts(), { invocations: 2, validations: 1 });
  assert.equal(result.recovery.implementationAttempts, 1);
  assert.equal(result.recovery.operations["validate/1"].attempts, 2);
  const report = read(join(options.storage, "measurements.json"));
  assert.deepEqual(
    report.records
      .filter((record) => record.stage === "validate")
      .map((record) => [record.attempt, record.retries]),
    [
      [1, 0],
      [1, 1],
    ],
  );
});

test("Editor reconciles a durable local handoff after interrupted receipt persistence without another delivery", async (t) => {
  const { options, counts } = await editorFixture(t);
  options.recovery = {};
  const ready = await runEditorTask(options);
  const handoffBefore = readFileSync(ready.delivery.reference, "utf8");
  const checkpointFile = join(options.storage, "checkpoint.json");
  const saved = JSON.parse(readFileSync(checkpointFile, "utf8"));
  saved.value.phase = "deliver";
  saved.value.inFlight = "deliver";
  delete saved.value.delivery;
  saved.value.recovery.operations["deliver/1"].status = "running";
  delete saved.value.recovery.operations["deliver/1"].result;
  writeFileSync(checkpointFile, JSON.stringify(saved));
  delete options.recovery;
  const resumed = await runEditorTask(options);
  assert.equal(resumed.phase, "ready");
  assert.deepEqual(counts(), { invocations: 2, validations: 1 });
  assert.equal(readFileSync(resumed.delivery.reference, "utf8"), handoffBefore);
  const report = read(join(options.storage, "measurements.json"));
  assert.equal(
    report.records.filter((record) => record.stage === "deliver").length,
    1,
  );
});

test("Editor persists stage and individual host-check measurements and preserves them on resume", async (t) => {
  const { options } = await editorFixture(t, { measuredCheck: true });
  const result = await runEditorTask(options);
  const report = read(join(options.storage, "measurements.json"));
  assert.deepEqual(
    report.records.map((record) => [
      record.stage,
      record.check,
      record.outcome,
    ]),
    [
      ["implement", undefined, "succeeded"],
      ["validate", undefined, "succeeded"],
      ["validate", "host-node-value", "succeeded"],
      ["review", undefined, "succeeded"],
      ["deliver", undefined, "succeeded"],
    ],
  );
  for (const record of report.records) {
    assert.ok(Number.isFinite(record.durationMs) && record.durationMs >= 0);
    assert.equal(record.tokens.totalInput, null);
    assert.match(record.unavailable.join(" "), /usage/);
  }
  const handoff = read(result.delivery.reference);
  assert.equal(handoff.validation.timings[0].check, "host-node-value");
  assert.ok(handoff.validation.timings[0].durationMs >= 0);
  await runEditorTask(options);
  assert.deepEqual(read(join(options.storage, "measurements.json")), report);
  if (process.env.SANDCASTLE_BASELINE_OUTPUT) {
    writeFileSync(
      process.env.SANDCASTLE_BASELINE_OUTPUT,
      JSON.stringify(
        {
          schema: 1,
          captured: new Date().toISOString(),
          kind: "extracted-workflow-scripted-agent-host-check-baseline",
          candidate: result.candidate,
          environment: {
            node: process.version,
            platform: process.platform,
            arch: process.arch,
            measurementModuleSha256: createHash("sha256")
              .update(
                readFileSync(
                  new URL(
                    "../node_modules/@rebuild/sandcastle/dist/workflow-measurements.js",
                    import.meta.url,
                  ),
                ),
              )
              .digest("hex"),
            originalSdk: "0.12.0",
          },
          limitations: [
            "Scripted agent transport with real Git and host Node assertions; no live model or Editor native acceptance.",
            "Cumulative model usage is unavailable; null is not zero.",
            "Stage and nested check timings overlap; they are not summed as end-to-end duration.",
            "Single fixture task and machine observation; no optimization savings claim.",
          ],
          report,
        },
        null,
        2,
      ) + "\n",
    );
  }
});

test("Editor preserves original SDK usage scope and does not double count Codex cached input", async (t) => {
  const { options } = await editorFixture(t, { reportedUsage: true });
  await runEditorTask(options);
  const record = read(join(options.storage, "measurements.json")).records[0];
  assert.deepEqual(record.observedUsage, {
    scope: "last-message",
    tokens: { totalInput: 100, cachedInput: 80, freshInput: 20, output: 7 },
  });
  assert.equal(record.tokens.totalInput, null);
  assert.match(record.unavailable.join(" "), /Stage totals.*last-message/);
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
  assert.deepEqual(
    read(join(options.storage, "measurements.json")).records.map(
      (record) => record.outcome,
    ),
    ["succeeded", "rejected"],
  );
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
  assert.deepEqual(
    read(join(options.storage, "measurements.json"))
      .records.filter((record) => record.stage === "review")
      .map((record) => [record.attempt, record.outcome]),
    [
      [1, "failed"],
      [2, "succeeded"],
    ],
  );
});

test("Editor validates in a separate prepared worktree and restores its edits and cache", async (t) => {
  const { f, options, counts } = await editorFixture(t, {
    preparedValidation: true,
  });
  const result = await runEditorTask(options);
  const prepared = options.validationWorktree.path;
  assert.equal(result.phase, "ready");
  assert.deepEqual(counts(), { invocations: 2, validations: 1 });
  assert.deepEqual(snapshot(prepared), options.validationBefore);
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
  writeFileSync(
    path,
    JSON.stringify({
      ...JSON.parse(readFileSync(path, "utf8")),
      value: unresolved,
    }),
  );
  await assert.rejects(runEditorTask(options), /recovery is unresolved/);
  assert.deepEqual(read(path), unresolved);
  assert.deepEqual(counts(), { invocations: 1, validations: 1 });
});

test("Editor acquires the repository lock before opening a sandbox", async (t) => {
  const { options, counts } = await editorFixture(t);
  const original = options.openSandbox;
  let entered;
  const opened = new Promise((resolve) => {
    entered = resolve;
  });
  let release;
  const gate = new Promise((resolve) => {
    release = resolve;
  });
  options.openSandbox = async (context) => {
    entered();
    await gate;
    return original(context);
  };
  const first = runEditorTask(options);
  try {
    await opened;
    await assert.rejects(runEditorTask(options), { code: "locked" });
    assert.deepEqual(counts(), { invocations: 0, validations: 0 });
  } finally {
    release();
  }
  assert.equal((await first).phase, "ready");
});
