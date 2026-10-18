import test from "node:test";
import assert from "node:assert/strict";
import { fixture, git, success } from "./test-fixture.mjs";
import { createEditorEvidence } from "./evidence.mjs";

test("Editor discovers current host/policy/dependencies for evidence instead of defaulting missing inputs", async (t) => {
  const f = fixture(t, async () => success("unused"));
  let host = "observed-host-a";
  let policy = "full-debug-release-v1";
  let dependencies = "observed-prepared-pins-a";
  const evidence = createEditorEvidence({
    validationPolicy: async () => policy,
    dependencies: async () => dependencies,
    environment: async () => host,
    reviewPolicy: async () => "independent-review-v1",
  });
  const context = {
    project: { config: { worktree: f.root } },
    candidate: git(f.root, "rev-parse", "HEAD"),
  };
  const first = await evidence.validation(context);
  assert.deepEqual(await evidence.validation(context), first);
  host = "observed-host-b";
  assert.notEqual(
    (await evidence.validation(context)).environment,
    first.environment,
  );
  policy = "full-debug-release-v2";
  assert.notEqual((await evidence.validation(context)).policy, first.policy);
  dependencies = "observed-prepared-pins-b";
  assert.notEqual(
    (await evidence.validation(context)).dependencies,
    first.dependencies,
  );
  host = "";
  await assert.rejects(
    evidence.validation(context),
    /environment.*unavailable/,
  );
});
