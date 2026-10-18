import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, resolve } from "node:path";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const read = (path) => JSON.parse(readFileSync(resolve(root, path), "utf8"));

export async function checkBaseline() {
  const expected = read(".sandcastle/baseline.json");
  const manifest = read("package.json");
  const lock = read("package-lock.json");
  const installed = read("node_modules/@ai-hero/sandcastle/package.json");
  const dependency = lock.packages["node_modules/@ai-hero/sandcastle"];
  assert.equal(manifest.devDependencies[expected.package], expected.version);
  assert.equal(installed.name, expected.package);
  assert.equal(installed.version, expected.version);
  assert.equal(dependency.resolved, expected.resolved);
  assert.equal(dependency.integrity, expected.integrity);
  assert.ok(
    !Object.keys(installed.exports).some((key) => key.startsWith("./workflow")),
  );
  const sdk = await import("@ai-hero/sandcastle");
  const { docker } = await import("@ai-hero/sandcastle/sandboxes/docker");
  for (const name of [
    "run",
    "createSandbox",
    "createWorktree",
    "codex",
    "createBindMountSandboxProvider",
  ])
    assert.equal(
      typeof sdk[name],
      "function",
      `Missing original public API: ${name}`,
    );
  assert.equal(typeof docker, "function");
  return {
    package: expected.package,
    version: expected.version,
    sourceCommit: expected.sourceCommit,
    installation: "original registry package",
    execution: "paused",
    node: process.version,
  };
}
