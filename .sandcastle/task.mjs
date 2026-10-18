import { execFileSync } from "node:child_process";
import { realpathSync } from "node:fs";
import { join, sep } from "node:path";
import { runWorkflow } from "@rebuild/sandcastle/workflow";
import { withPreparedWorktree } from "@rebuild/sandcastle/workflow/integrations";
import { withWorkflowRuntime } from "@rebuild/sandcastle/workflow/runtime";

const git = (cwd, ...args) =>
  execFileSync("git", args, {
    cwd,
    encoding: "utf8",
    windowsHide: true,
  }).trim();

/** Explicit single-task adapter. No queue discovery, agent defaults or remote delivery. */
export async function runEditorTask(options) {
  return withWorkflowRuntime(
    {
      ...options.runtime,
      projectId: options.projectId,
      signal: options.signal,
    },
    async (runtime) => {
      const checkpoint = await runtime.read("checkpoint");
      if (checkpoint && checkpoint.project !== options.projectId)
        throw new Error(
          "Saved checkpoint belongs to another project; it was preserved.",
        );
      if (
        checkpoint?.task &&
        JSON.stringify(checkpoint.task) !== JSON.stringify(options.task)
      )
        throw new Error(
          "Saved task requirements changed; preserve its checkpoint and review the new scope.",
        );
      const sandbox = await runtime.resource("sdk-sandbox", {
        identity: runtime.compatibility,
        create: () =>
          options.openSandbox({ runtime, branch: `task/${runtime.key}` }),
        close: (handle) => handle.close(),
      });
      return runTask(options, runtime, sandbox, checkpoint);
    },
  );
}

async function runTask(
  { projectId, task, agents, prompts, validate, prepareValidation, signal },
  runtime,
  sandbox,
  checkpoint,
) {
  const directory = runtime.directory;
  const handoffFile = join(directory, "records/handoff.json");
  const currentCandidate = async () => {
    runtime.assertActive();
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
      validate: async ({ candidate }) => {
        const recovery = await runtime.read("validation-recovery");
        if (prepareValidation && recovery && recovery.phase !== "restored")
          throw new Error(
            "Prepared worktree recovery is unresolved; preserve its recorded changes before another validation.",
          );
        const validationWorktree = prepareValidation
          ? await runtime.resource("native-validation", {
              identity: runtime.compatibility,
              create: async (directory) => {
                const prepared = await prepareValidation({
                  directory,
                  runtime,
                });
                if (
                  !realpathSync(prepared.path).startsWith(
                    realpathSync(directory) + sep,
                  )
                )
                  throw new Error(
                    "Prepared validation must stay inside this run's resource directory.",
                  );
                return prepared;
              },
            })
          : undefined;
        return validationWorktree
          ? withPreparedWorktree(
              {
                source: validationWorktree.source,
                prepared: validationWorktree.path,
                candidate,
                command: validationWorktree.command,
                saveRecovery: (record) =>
                  runtime.save("validation-recovery", record),
              },
              (worktree) => validate({ worktree, candidate, signal }),
            )
          : validate({ worktree: sandbox.worktreePath, candidate, signal });
      },
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
        if ((await runtime.read("handoff")) !== undefined)
          throw new Error(
            "A local handoff already exists; reconcile it before another delivery.",
          );
        await runtime.save("handoff", {
          project: project.id,
          task,
          branch: sandbox.branch,
          candidate,
          validation,
          review,
        });
        return { candidate, reference: handoffFile };
      },
      save: (state) => runtime.save("checkpoint", state),
    },
  });
}
