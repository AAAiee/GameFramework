import test from "node:test";
import assert from "node:assert/strict";
import { execFile, execFileSync } from "node:child_process";
import { promisify } from "node:util";
import {
  copyFileSync,
  existsSync,
  mkdirSync,
  mkdtempSync,
  readFileSync,
  realpathSync,
  rmSync,
  writeFileSync,
} from "node:fs";
import { tmpdir } from "node:os";
import { basename, delimiter, dirname, join, sep } from "node:path";
import {
  createSandbox,
  createBindMountSandboxProvider,
  codex,
} from "@ai-hero/sandcastle";
import { checkBaseline } from "./check-baseline.mjs";

const shell =
  process.platform === "win32"
    ? "C:/Program Files/Git/bin/bash.exe"
    : "/bin/sh";
if (process.platform === "win32")
  process.env.PATH = dirname(shell) + delimiter + process.env.PATH;
const exec = promisify(execFile);
const git = (cwd, ...args) =>
  execFileSync("git", args, {
    cwd,
    encoding: "utf8",
    windowsHide: true,
  }).trim();

function fixture(t, behavior) {
  const root = mkdtempSync(join(tmpdir(), "sandcastle-original-baseline-"));
  t.after(() => {
    const path = realpathSync(root);
    assert.ok(path.startsWith(realpathSync(tmpdir()) + sep));
    assert.ok(basename(path).startsWith("sandcastle-original-baseline-"));
    rmSync(path, { recursive: true, force: true });
  });
  writeFileSync(join(root, ".gitignore"), ".sandcastle/\n");
  writeFileSync(join(root, "source.txt"), "original\n");
  git(root, "init", "-b", "main");
  git(root, "config", "user.email", "baseline@example.invalid");
  git(root, "config", "user.name", "Baseline fixture");
  git(root, "config", "core.autocrlf", "false");
  git(root, "add", ".");
  git(root, "commit", "-m", "fixture base");
  const before = git(root, "rev-parse", "HEAD");
  const calls = [];
  let closed = 0;
  const sandbox = createBindMountSandboxProvider({
    name: "deterministic-fixture",
    create: async ({ worktreePath }) => ({
      worktreePath,
      exec: async (command, options = {}) => {
        if (command.startsWith("codex ")) {
          calls.push(options.stdin);
          const result = await behavior(worktreePath);
          for (const line of result.stdout.split("\n").filter(Boolean))
            options.onLine?.(line);
          return result;
        }
        try {
          const result = await exec(shell, ["-c", command], {
            cwd: options.cwd ?? worktreePath,
            windowsHide: true,
            timeout: 10000,
          });
          for (const line of result.stdout.split("\n").filter(Boolean))
            options.onLine?.(line);
          return { ...result, exitCode: 0 };
        } catch (error) {
          return {
            stdout: error.stdout ?? "",
            stderr: error.stderr ?? String(error),
            exitCode: typeof error.code === "number" ? error.code : 1,
          };
        }
      },
      copyFileIn: async (source, target) => {
        mkdirSync(dirname(target), { recursive: true });
        copyFileSync(source, target);
      },
      copyFileOut: async (source, target) => {
        mkdirSync(dirname(target), { recursive: true });
        copyFileSync(source, target);
      },
      close: async () => {
        closed++;
      },
    }),
  });
  const options = (branch) => ({ cwd: root, branch, sandbox });
  const runOptions = {
    agent: codex("fixture-model", { captureSessions: false }),
    prompt: "Assigned fixture task",
    maxIterations: 1,
    logging: { type: "file", path: join(root, ".sandcastle/logs/fixture.log") },
  };
  return { root, before, calls, options, runOptions, closed: () => closed };
}

const success = (text) => ({
  stdout: JSON.stringify({
    type: "item.completed",
    item: { type: "agent_message", text },
  }),
  stderr: "",
  exitCode: 0,
});

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
