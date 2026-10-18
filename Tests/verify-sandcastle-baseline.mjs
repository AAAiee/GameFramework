import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { spawnSync } from "node:child_process";

const root = new URL("./fixtures/sandcastle-baseline/", import.meta.url);
const manifestBytes = readFileSync(new URL("manifest.json", root));
assert.equal(createHash("sha256").update(manifestBytes).digest("hex"),
  "8bcdfed0115945e50b7892c8de8ef867f0873b45d3daee55d803aa00f9aa109e", "Original manifest changed");
const manifest = JSON.parse(manifestBytes);
assert.equal(manifest.files.length, 15, "Incomplete recovery manifest");
for (const entry of manifest.files) {
  assert.match(entry.path, /^(?:\.sandcastle\/[^/]+|Scripts\/Validate-Windows\.(?:md|ps1)|package(?:-lock)?\.json)$/);
  const bytes = readFileSync(new URL(entry.path, root));
  assert.equal(createHash("sha256").update(bytes).digest("hex"), entry.sha256, entry.path);
}
// Only run the original tests, never main.mts or any native/remote delivery operation.
const result = spawnSync(process.execPath, [
  "--test",
  fileURLToPath(new URL(".sandcastle/workflow.test.mjs", root)),
  fileURLToPath(new URL(".sandcastle/github.test.mjs", root)),
], { encoding: "utf8", timeout: 120_000 });
process.stdout.write(result.stdout ?? "");
process.stderr.write(result.stderr ?? "");
if (result.error) throw result.error;
assert.equal(result.status, 0, "Original orchestration tests failed");
assert.match(result.stdout, /# tests 32\r?\n/);
assert.match(result.stdout, /# pass 32\r?\n/);
console.log("PASS: sandcastle-baseline (15 checksums; 32 original tests)");
