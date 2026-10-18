import { execFileSync } from "node:child_process";
import { randomUUID } from "node:crypto";
import {
  closeSync,
  existsSync,
  fsyncSync,
  mkdirSync,
  openSync,
  readFileSync,
  renameSync,
  writeFileSync,
} from "node:fs";
import { join, resolve } from "node:path";
import { runWorkflow } from "@rebuild/sandcastle/workflow";

const git = (cwd, ...args) =>
  execFileSync("git", args, {
    cwd,
    encoding: "utf8",
    windowsHide: true,
  }).trim();
const read = (file) => JSON.parse(readFileSync(file, "utf8"));

// The caller serializes this task's storage and worktree. SDK lifecycle stays with the caller.
function saveJson(path, value) {
  const temporary = `${path}.${randomUUID()}.tmp`;
  const fd = openSync(temporary, "wx");
  try {
    writeFileSync(fd, JSON.stringify(value, null, 2) + "\n");
    fsyncSync(fd);
  } finally {
    closeSync(fd);
  }
  renameSync(temporary, path);
}

/** Explicit single-task adapter. No queue discovery, agent defaults or remote delivery. */
export async function runEditorTask({
  projectId,
  task,
  sandbox,
  agents,
  prompts,
  validate,
  storage,
  signal,
}) {
  const directory = resolve(storage);
  mkdirSync(directory, { recursive: true });
  const stateFile = join(directory, "checkpoint.json");
  const handoffFile = join(directory, "handoff.json");
  const checkpoint = existsSync(stateFile) ? read(stateFile) : undefined;
  if (
    checkpoint?.task &&
    JSON.stringify(checkpoint.task) !== JSON.stringify(task)
  )
    throw new Error(
      "Saved task requirements changed; preserve its checkpoint and review the new scope.",
    );
  const currentCandidate = async () => {
    if (
      git(sandbox.worktreePath, "branch", "--show-current") !== sandbox.branch
    )
      throw new Error(
        "Task checkout changed branches; preserve it for inspection.",
      );
    if (git(sandbox.worktreePath, "status", "--porcelain").trim())
      throw new Error(
        "Task worktree has unfinished changes; preserve them for inspection.",
      );
    return git(sandbox.worktreePath, "rev-parse", "HEAD");
  };
  const project = {
    id: projectId,
    config: { branch: sandbox.branch, worktree: sandbox.worktreePath },
  };
  return runWorkflow({
    project,
    checkpoint,
    signal,
    operations: {
      selectTask: async () => task,
      implement: async (context) => {
        await sandbox.run({
          agent: agents.implementer,
          prompt: `${prompts.implement}\n\n${JSON.stringify({ task: context.task, branch: sandbox.branch })}`,
          maxIterations: 1,
          signal,
          logging: { type: "file", path: join(directory, "implement.log") },
        });
        return currentCandidate();
      },
      currentCandidate,
      validate: async ({ candidate }) =>
        validate({ worktree: sandbox.worktreePath, candidate, signal }),
      review: async ({ candidate, validation }) => {
        const result = await sandbox.run({
          agent: agents.reviewer,
          prompt: `${prompts.review}\n\nReturn only a JSON object with candidate, approved, and criteria [{id,satisfied,evidence}].\n${JSON.stringify({ task, candidate, validation })}`,
          maxIterations: 1,
          signal,
          logging: { type: "file", path: join(directory, "review.log") },
        });
        return JSON.parse(result.stdout);
      },
      deliver: async ({ candidate, validation, review }) => {
        if ((await currentCandidate()) !== candidate)
          throw new Error("Candidate changed before local handoff.");
        if (existsSync(handoffFile))
          throw new Error(
            "A local handoff already exists; reconcile it before another delivery.",
          );
        saveJson(handoffFile, {
          project: project.id,
          task,
          branch: sandbox.branch,
          candidate,
          validation,
          review,
        });
        return { candidate, reference: handoffFile };
      },
      save: async (state) => saveJson(stateFile, state),
    },
  });
}
