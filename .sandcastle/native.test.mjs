import test from "node:test";
import assert from "node:assert/strict";
import {
  selectNativePlan,
  prepareNativeWorktree,
  nativeDependencies,
  nativeValidationResult,
} from "./native.mjs";
import { fixture, git } from "./test-fixture.mjs";
import { mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { join } from "node:path";

test("diagnostics include known dependent fixtures and unknown coverage uses complete Debug diagnostics", () => {
  const mapped = selectNativePlan({ phase: "diagnose", issue: 21 });
  assert.deepEqual(mapped.configurations, ["Debug"]);
  assert.equal(mapped.buildTarget, "Build");
  assert.deepEqual(mapped.selectors, ["font-atlas", "atlas-placeholders"]);
  const unknown = selectNativePlan({ phase: "diagnose", issue: 9876 });
  assert.equal(unknown.fullSuite, true);
  assert.equal(unknown.selectors.length, 13);
  assert.equal(unknown.windowsChecks.length, 5);
  assert.equal(
    unknown.optionalWindowsChecks[0].script,
    "Tests/Windows/unit-tests.ps1",
  );
});

test("a passing diagnostic report cannot satisfy final native acceptance", () => {
  const plan = selectNativePlan({ phase: "validate" });
  assert.throws(
    () =>
      nativeValidationResult({
        report: {
          status: "passed",
          phase: "diagnose",
          commit: "sha",
          checks: [],
        },
        plan,
        candidate: "sha",
        reportPath: "diagnostic.json",
        unitTests: true,
      }),
    /phase|final|contract/i,
  );
  assert.throws(
    () =>
      nativeValidationResult({
        report: {
          status: "passed",
          phase: "validate",
          buildTarget: "Rebuild",
          configurations: ["Debug", "Release"],
          commit: "sha",
          checks: [],
        },
        plan,
        candidate: "sha",
        reportPath: "partial.json",
        unitTests: true,
      }),
    /missing|incomplete/i,
  );
});

test("owned preparation creates independent local submodules and refuses changed pins without touching preserved sources", async (t) => {
  const f = fixture(t, async () => {
    throw new Error("No agent expected");
  });
  const dependency = join(f.root, ".sandcastle/local-dependency");
  mkdirSync(dependency);
  git(dependency, "init", "-b", "main");
  git(dependency, "config", "user.name", "Fixture");
  git(dependency, "config", "user.email", "fixture@example.invalid");
  writeFileSync(join(dependency, "header.h"), "// pinned\n");
  git(dependency, "add", ".");
  git(dependency, "commit", "-m", "dependency");
  git(
    f.root,
    "-c",
    "protocol.file.allow=always",
    "submodule",
    "add",
    dependency,
    "dependency",
  );
  git(f.root, "commit", "-am", "pin local dependency");
  const candidate = git(f.root, "rev-parse", "HEAD");
  const directory = join(f.root, ".sandcastle/owned-native");
  mkdirSync(directory);
  const prepared = await prepareNativeWorktree({
    directory,
    source: f.root,
    candidate,
    localDependencies: { dependency },
  });
  assert.equal(git(prepared.path, "rev-parse", "HEAD"), candidate);
  assert.equal(git(prepared.path, "status", "--porcelain"), "");
  const pins = await nativeDependencies({
    prepared: prepared.path,
    source: f.root,
    candidate,
  });
  assert.match(pins, /dependency/);
  assert.equal(git(dependency, "status", "--porcelain"), "");
  assert.notEqual(
    git(join(prepared.path, "dependency"), "rev-parse", "--absolute-git-dir"),
    git(dependency, "rev-parse", "--absolute-git-dir"),
  );
  writeFileSync(join(dependency, "header.h"), "// new pin\n");
  git(dependency, "commit", "-am", "new dependency");
  git(join(f.root, "dependency"), "fetch", dependency, "main");
  git(
    join(f.root, "dependency"),
    "checkout",
    "--detach",
    git(dependency, "rev-parse", "HEAD"),
  );
  git(f.root, "commit", "-am", "change pin");
  await assert.rejects(
    nativeDependencies({
      prepared: prepared.path,
      source: f.root,
      candidate: git(f.root, "rev-parse", "HEAD"),
    }),
    /pin|prerequisite/i,
  );
  assert.equal(git(prepared.path, "rev-parse", "HEAD"), candidate);
});
