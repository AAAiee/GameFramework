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

const shell =
  process.platform === "win32"
    ? "C:/Program Files/Git/bin/bash.exe"
    : "/bin/sh";
if (process.platform === "win32")
  process.env.PATH = dirname(shell) + delimiter + process.env.PATH;
const exec = promisify(execFile);
export const git = (cwd, ...args) =>
  execFileSync("git", args, {
    cwd,
    encoding: "utf8",
    windowsHide: true,
  }).trim();

export function fixture(t, behavior) {
  const root = mkdtempSync(join(tmpdir(), "sandcastle-original-baseline-"));
  mkdirSync(join(root, ".sandcastle"));
  process.env.GIT_CONFIG_GLOBAL = join(root, ".sandcastle/gitconfig");
  writeFileSync(process.env.GIT_CONFIG_GLOBAL, "");
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

export const success = (text) => ({
  stdout: JSON.stringify({
    type: "item.completed",
    item: { type: "agent_message", text },
  }),
  stderr: "",
  exitCode: 0,
});
