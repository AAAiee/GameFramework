import test from "node:test";
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import {
  mkdtempSync,
  readFileSync,
  realpathSync,
  rmSync,
  writeFileSync,
} from "node:fs";
import { tmpdir } from "node:os";
import { basename, join, resolve, sep } from "node:path";
import { fileURLToPath } from "node:url";

test("the built workflow runs and resumes in an independent consumer without Editor or credentials", (t) => {
  const temporary = mkdtempSync(join(tmpdir(), "sandcastle-independent-"));
  t.after(() => {
    const path = realpathSync(temporary);
    assert.ok(path.startsWith(realpathSync(tmpdir()) + sep));
    assert.ok(basename(path).startsWith("sandcastle-independent-"));
    rmSync(path, { recursive: true, force: true });
  });
  const root = fileURLToPath(new URL("..", import.meta.url));
  const manifest = JSON.parse(readFileSync(join(root, "package.json")));
  const archive = resolve(
    root,
    manifest.devDependencies["@rebuild/sandcastle"].slice(5),
  );
  writeFileSync(
    join(temporary, "package.json"),
    JSON.stringify({
      name: "independent-consumer",
      private: true,
      type: "module",
      dependencies: {
        "@rebuild/sandcastle": `file:${archive.replaceAll("\\", "/")}`,
      },
    }),
  );
  const args = [
    "install",
    "--offline",
    "--ignore-scripts",
    "--no-audit",
    "--no-fund",
  ];
  execFileSync(
    process.platform === "win32" ? "cmd.exe" : "npm",
    process.platform === "win32"
      ? ["/d", "/s", "/c", "npm.cmd " + args.join(" ")]
      : args,
    { cwd: temporary, encoding: "utf8", windowsHide: true, timeout: 60000 },
  );
  writeFileSync(
    join(temporary, "verify.mjs"),
    `
import assert from 'node:assert/strict';
import { readFileSync, writeFileSync } from 'node:fs';
import { runWorkflow } from '@rebuild/sandcastle/workflow';
const calls = []; const task = {id:'fresh-task',criteria:[{id:'C1',description:'Requested behavior works'}]};
const project = {id:'fresh-consumer',config:{repository:'example/fresh'}};
const review = async ({candidate}) => {calls.push('review');return {candidate,approved:true,criteria:[{id:'C1',satisfied:true,evidence:'Independent review'}]};};
const operations = {
 selectTask: async () => task,
 implement: async () => {calls.push('implement');return 'candidate';},
 currentCandidate: async () => 'candidate',
 validate: async ({candidate}) => {calls.push('validate');return {candidate,passed:true,evidence:['Host assertion passed']};},
 review: async () => {calls.push('interrupted-review');throw Error('Unfamiliar interruption');},
 deliver: async ({candidate}) => {calls.push('local-handoff');return {candidate,reference:'local-receipt'};},
 save: async (state) => writeFileSync('checkpoint.json',JSON.stringify(state))
};
await assert.rejects(runWorkflow({project,operations}),/Unfamiliar interruption/);
const checkpoint = JSON.parse(readFileSync('checkpoint.json'));
assert.equal(checkpoint.phase,'review'); operations.review = review;
const result = await runWorkflow({project,operations,checkpoint});
assert.equal(result.phase,'ready');
assert.deepEqual(calls,['implement','validate','interrupted-review','review','local-handoff']);
console.log('PASS: independent packaged workflow with durable review resume');
`,
  );
  const environment = { ...process.env };
  delete environment.NODE_TEST_CONTEXT;
  const output = execFileSync(process.execPath, ["verify.mjs"], {
    cwd: temporary,
    env: environment,
    encoding: "utf8",
    windowsHide: true,
    timeout: 30000,
  });
  assert.match(output, /PASS: independent packaged workflow/);
});
