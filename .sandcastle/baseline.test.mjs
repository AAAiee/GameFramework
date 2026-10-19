import test from "node:test";
import assert from "node:assert/strict";
import { existsSync, readFileSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import { createSandbox } from "@ai-hero/sandcastle";
import { checkBaseline } from "./check-baseline.mjs";
import { fixture, git, success } from "./test-fixture.mjs";

test("original package provenance and public imports match the pinned baseline", async () => {
  assert.equal(
    (await checkBaseline()).installation,
    "original registry package",
  );
  await assert.rejects(import("@ai-hero/sandcastle/workflow"), {
    code: "ERR_PACKAGE_PATH_NOT_EXPORTED",
  });
});

test("public sandbox commits on a task branch and leaves the owner checkout unchanged", async (t) => {
  const f = fixture(t, async (path) => {
    writeFileSync(join(path, "source.txt"), "completed task\n");
    git(path, "add", "source.txt");
    git(path, "commit", "-m", "fixture task");
    return success("COMPLETE");
  });
  const sandbox = await createSandbox(f.options("task/fixture"));
  try {
    const result = await sandbox.run(f.runOptions);
    assert.equal(f.calls.length, 1);
    assert.equal(result.commits.length, 1);
    assert.equal(
      git(f.root, "rev-parse", "task/fixture"),
      result.commits[0].sha,
    );
    assert.equal(git(f.root, "rev-parse", "HEAD"), f.before);
    assert.equal(
      readFileSync(join(f.root, "source.txt"), "utf8"),
      "original\n",
    );
  } finally {
    await sandbox.close();
  }
  assert.equal(f.closed(), 1);
});

test("an unfamiliar agent failure stops after one invocation and preserves unfinished work", async (t) => {
  const f = fixture(t, async (path) => {
    writeFileSync(join(path, "unfinished.txt"), "recoverable work\n");
    return {
      stdout: "",
      stderr: "Unfamiliar provider error: fixture-739",
      exitCode: 7,
    };
  });
  const sandbox = await createSandbox(f.options("task/failure"));
  const path = sandbox.worktreePath;
  try {
    await assert.rejects(
      sandbox.run(f.runOptions),
      /Unfamiliar provider error: fixture-739/,
    );
    assert.equal(f.calls.length, 1);
    assert.equal(
      readFileSync(join(path, "unfinished.txt"), "utf8"),
      "recoverable work\n",
    );
    assert.equal(git(f.root, "rev-parse", "HEAD"), f.before);
  } finally {
    await sandbox.close();
  }
  assert.ok(existsSync(join(path, "unfinished.txt")));
  assert.equal(f.closed(), 1);
});

test("pre-aborted invocation does not start an agent and the handle remains usable", async (t) => {
  const f = fixture(t, async () => success("READY"));
  const sandbox = await createSandbox(f.options("task/cancel"));
  try {
    const controller = new AbortController();
    controller.abort(new Error("Fixture cancellation"));
    await assert.rejects(
      sandbox.run({ ...f.runOptions, signal: controller.signal }),
      /Fixture cancellation/,
    );
    assert.equal(f.calls.length, 0);
    await sandbox.run(f.runOptions);
    assert.equal(f.calls.length, 1);
    assert.equal(git(f.root, "rev-parse", "HEAD"), f.before);
  } finally {
    await sandbox.close();
  }
});
