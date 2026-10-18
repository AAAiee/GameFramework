import { execFileSync } from "node:child_process";
import { createHash } from "node:crypto";
import { realpathSync } from "node:fs";
import { join, sep } from "node:path";
import {
  extractWorkflowReview,
  runWorkflow,
  WorkflowOperationError,
  createWorkflowPacket,
  workflowPacketPolicy,
} from "@rebuild/sandcastle/workflow";
import { withPreparedWorktree } from "@rebuild/sandcastle/workflow/integrations";
import { withWorkflowRuntime } from "@rebuild/sandcastle/workflow/runtime";
import { createWorkflowMeasurements } from "@rebuild/sandcastle/workflow/measurements";
import { createEditorContext } from "./context.mjs";

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
  {
    projectId,
    task,
    agents,
    prompts,
    validate,
    prepareValidation,
    signal,
    recovery: recoveryOptions,
    evidence,
    reviewOutput,
    context: packetInputs,
  },
  runtime,
  sandbox,
  checkpoint,
) {
  const directory = runtime.directory;
  const packets = createEditorContext({
    sandbox,
    runtime,
    task,
    context: packetInputs,
    git,
  });
  const measurements = createWorkflowMeasurements({
    identity: {
      project: projectId,
      run: runtime.runId,
      task: task.id,
      environment: runtime.compatibility,
    },
    previous: await runtime.read("measurements"),
    save: (report) => runtime.save("measurements", report),
  });
  const measured = (stage, context, action) =>
    measurements.measure(
      {
        stage,
        candidate: context.candidate,
        attempt: context.attempt?.implementation,
        retries: context.attempt ? context.attempt.transport - 1 : undefined,
      },
      action,
    );
  // Receipts bind the complete observed evidence, not only the unchanged code SHA.
  const handoffName = (context) =>
    context.validation?.identity
      ? `handoff-${createHash("sha256")
          .update(
            JSON.stringify({
              task: context.task,
              candidate: context.candidate,
              validation: context.validation,
              review: context.review,
            }),
          )
          .digest("hex")}`
      : "handoff";
  const handoffFile = (context) =>
    join(directory, `records/${handoffName(context)}.json`);
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
  const recovery = recoveryOptions ?? (checkpoint?.recovery ? {} : undefined);
  const strictReview =
    reviewOutput === true || checkpoint?.reviewOutputEnabled === true;
  const review = (context, correction) =>
    measured(
      correction ? "review-correction" : "review",
      context,
      async (observation) => {
        const { candidate, validation } = context;
        const invocation = packets.invocation(
          correction ? "review-correction" : "review",
          context,
        );
        const packet = correction
          ? createWorkflowPacket({
              stage: "correction",
              task: context.task,
              candidate,
              problem: context.problem,
              output: invocation.write(
                "previous-output.log",
                typeof context.output === "string"
                  ? context.output
                  : JSON.stringify(context.output),
              ),
            })
          : createWorkflowPacket({
              stage: "review",
              task: context.task,
              base: packetInputs.base,
              candidate,
              changedPaths: git(
                sandbox.worktreePath,
                "diff",
                "--name-only",
                packetInputs.base,
                candidate,
                "--",
              )
                .split("\n")
                .filter(Boolean),
              diffCommand: `git diff ${packetInputs.base} ${candidate} --`,
              validation: {
                ...validation,
                evidence: validation.evidence.map(packets.evidence),
              },
              references: [
                packets.requirements,
                ...packets.current().references,
              ],
            });
        invocation.write("packet.json", packet);
        const result = await sandbox.run({
          agent: agents.reviewer,
          prompt: correction
            ? packet
            : `${prompts.review}\n\n${strictReview ? "Return a final <review> tag on its own line containing strict JSON with task (the exact task.id), candidate, approved, and criteria [{id,satisfied,evidence}]. Close with </review> and no trailing text." : "Return only a JSON object with candidate, approved, and criteria [{id,satisfied,evidence}]."}\n${packet}`,
          maxIterations: 1,
          signal,
          logging: {
            type: "file",
            path: invocation.log,
          },
        });
        const output = invocation.write("output.log", result.stdout);
        await runtime.save("review-evidence", {
          candidate,
          generation: context.evidenceGeneration ?? 0,
          output,
        });
        const usage = result.iterations.at(-1)?.usage;
        if (usage) observation.usage({ ...usage, scope: "last-message" });
        if (!strictReview) {
          const value = JSON.parse(result.stdout);
          if (value.approved === false) observation.rejected();
          return value;
        }
        try {
          if (extractWorkflowReview(result.stdout, context).approved === false)
            observation.rejected();
        } catch (cause) {
          if (
            !(cause instanceof WorkflowOperationError) ||
            cause.kind !== "format"
          )
            throw cause;
          // The workflow owns extraction/correction charging. Invocation measurements retain raw output semantics.
        }
        return result.stdout;
      },
    );
  return runWorkflow({
    project,
    checkpoint,
    signal,
    evidence: evidence && {
      validation: evidence.validation,
      review: async (context) => {
        const policy = await evidence.review(context);
        if (typeof policy !== "string" || !policy.trim())
          throw new Error("Current independent review policy is unavailable.");
        return createHash("sha256")
          .update(
            JSON.stringify({
              policy,
              prompt: prompts.review,
              packetPolicy: workflowPacketPolicy,
              context: packets.current(),
            }),
          )
          .digest("hex");
      },
    },
    reviewOutput: strictReview
      ? { correct: (context) => review(context, true) }
      : undefined,
    recovery: recovery && {
      ...recovery,
      reconcile: async (context) => {
        if (context.phase !== "deliver")
          return recovery.reconcile?.(context) ?? { status: "uncertain" };
        const saved = await runtime.read(handoffName(context));
        if (saved === undefined) return { status: "not-started" };
        if (
          saved.project === project.id &&
          saved.branch === sandbox.branch &&
          saved.candidate === context.candidate &&
          (await currentCandidate()) === context.candidate &&
          JSON.stringify(saved.task) === JSON.stringify(context.task) &&
          JSON.stringify(saved.validation) ===
            JSON.stringify(context.validation) &&
          JSON.stringify(saved.review) === JSON.stringify(context.review)
        )
          return {
            status: "completed",
            value: {
              candidate: saved.candidate,
              reference: handoffFile(context),
            },
          };
        return { status: "uncertain" };
      },
    },
    operations: {
      selectTask: async () => task,
      implement: async (context) =>
        measured("implement", context, async (observation) => {
          const invocation = packets.invocation(
            context.repair ? "repair" : "implement",
            context,
          );
          const previousReview =
            context.repair?.phase === "review"
              ? await runtime.read("review-evidence")
              : undefined;
          const packet = context.repair
            ? createWorkflowPacket({
                stage: "repair",
                task: context.task,
                requirements: packets.requirements,
                repair: {
                  ...context.repair,
                  failures: context.repair.failures?.map((failure) => ({
                    ...failure,
                    evidence: failure.evidence.map(packets.evidence),
                  })),
                },
                references: previousReview
                  ? [packets.evidence(previousReview.output)]
                  : [],
              })
            : packets.initial;
          invocation.write("packet.json", packet);
          const result = await sandbox.run({
            agent: agents.implementer,
            prompt: `${prompts.implement}\n\n${packet}`,
            maxIterations: 1,
            signal,
            logging: { type: "file", path: invocation.log },
          });
          const usage = result.iterations.at(-1)?.usage;
          if (usage) observation.usage({ ...usage, scope: "last-message" });
          return currentCandidate();
        }),
      currentCandidate,
      validate: async (context) =>
        measured("validate", context, async (observation) => {
          const { candidate } = context;
          const previousMeasurements = new Set(
            measurements.report().records.map(({ id }) => id),
          );
          const measureCheck = (check, action) =>
            measurements.measure(
              {
                stage: "validate",
                check,
                candidate,
                attempt: context.attempt?.implementation,
                retries: context.attempt
                  ? context.attempt.transport - 1
                  : undefined,
              },
              async (checkObservation) => {
                const value = await action();
                if (value?.passed === false) checkObservation.rejected();
                return value;
              },
            );
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
          const result = await (validationWorktree
            ? withPreparedWorktree(
                {
                  source: validationWorktree.source,
                  prepared: validationWorktree.path,
                  candidate,
                  command: validationWorktree.command,
                  saveRecovery: (record) =>
                    runtime.save("validation-recovery", record),
                },
                (worktree) =>
                  validate({ worktree, candidate, signal, measureCheck }),
              )
            : validate({
                worktree: sandbox.worktreePath,
                candidate,
                signal,
                measureCheck,
              }));
          if (result.passed === false) observation.rejected();
          const timings = measurements
            .report()
            .records.filter(
              (record) =>
                record.stage === "validate" &&
                record.check &&
                !previousMeasurements.has(record.id) &&
                record.candidate === candidate,
            )
            .map(({ id, check, attempt, retries, durationMs, outcome }) => ({
              id,
              check,
              attempt,
              retries,
              durationMs,
              outcome,
            }));
          return {
            ...result,
            evidence: result.evidence.map(packets.evidence),
            ...(result.failures
              ? {
                  failures: result.failures.map((failure) => ({
                    ...failure,
                    evidence: failure.evidence.map(packets.evidence),
                  })),
                }
              : {}),
            timings,
          };
        }),
      review: (context) => review(context, false),
      deliver: async (context) =>
        measured("deliver", context, async () => {
          const { candidate, validation, review } = context;
          if ((await currentCandidate()) !== candidate)
            throw new Error("Candidate changed before local handoff.");
          const name = handoffName(context);
          if ((await runtime.read(name)) !== undefined)
            throw new Error(
              "A local handoff already exists; reconcile it before another delivery.",
            );
          await runtime.save(name, {
            project: project.id,
            task,
            branch: sandbox.branch,
            candidate,
            validation,
            review,
            stageContext: {
              requirements: packets.retained(packets.requirements),
              review: packets.retained(
                (await runtime.read("review-evidence")).output,
              ),
            },
          });
          return { candidate, reference: handoffFile(context) };
        }),
      save: (state) => runtime.save("checkpoint", state),
    },
  });
}
