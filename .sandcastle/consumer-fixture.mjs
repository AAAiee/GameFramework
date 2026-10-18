// Copied into a fresh npm consumer and executed there; imports no Editor files.
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { existsSync, mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import {
  runWorkflow,
  WorkflowOperationError,
} from "@rebuild/sandcastle/workflow";
import { createCommandRunner } from "@rebuild/sandcastle/workflow/integrations";
import { withWorkflowRuntime } from "@rebuild/sandcastle/workflow/runtime";

const git = (cwd, ...args) =>
  execFileSync("git", args, {
    cwd,
    encoding: "utf8",
    windowsHide: true,
  }).trim();
const roots = [];
const snapshots = [];
for (const id of ["A", "B"]) {
  const root = join(process.cwd(), "consumer-" + id);
  roots.push(root);
  mkdirSync(root);
  git(root, "init", "-b", "main");
  git(root, "config", "user.name", "Independent consumer");
  git(root, "config", "user.email", "consumer@example.invalid");
  git(root, "commit", "--allow-empty", "-m", "base");
  const project = {
    id: "consumer-" + id,
    config: {
      prompt: `prompt-${id}`,
      credential: `fixture-token-${id}`,
      image: "fixture-image-v1",
    },
  };
  const task = {
    id: "same-task-number",
    criteria: [
      { id: "behavior", description: `Requested ${id} behavior works` },
    ],
  };
  const candidate = git(root, "rev-parse", "HEAD");
  const events = [];
  const directories = [];
  let opens = 0;
  let closes = 0;
  let interrupt = true;
  const execute = (scenario) =>
    withWorkflowRuntime(
      {
        repository: root,
        projectId: project.id,
        runId: scenario,
        compatibility: "fixture-dependencies-v1",
        command: createCommandRunner({ cwd: root }),
      },
      async (runtime) => {
        directories.push(runtime.directory);
        const sandbox = await runtime.resource("sandbox", {
          identity: project.config.image,
          create: async (directory) => {
            opens++;
            const session = join(directory, "session.json");
            if (existsSync(session))
              assert.equal(
                JSON.parse(readFileSync(session)).project,
                project.id,
              );
            writeFileSync(
              session,
              JSON.stringify({ project: project.id, session: `session-${id}` }),
            );
            return {
              mount: directory,
              credential: project.config.credential,
              prompts: [],
            };
          },
          close: async () => {
            closes++;
          },
        });
        assert.ok(sandbox.mount.startsWith(runtime.directory));
        assert.equal(sandbox.credential, `fixture-token-${id}`);
        assert.equal(
          await runtime.resource("sandbox", {
            identity: project.config.image,
            create: async () => {
              throw Error("must share the same handle");
            },
          }),
          sandbox,
        );
        if (scenario === "exhausted" && !(await runtime.read("budget")))
          await runtime.save("budget", { used: 2, maximum: 2 });
        const operations = {
          selectTask: async () => {
            const budget = await runtime.read("budget");
            if (budget && budget.used >= budget.maximum)
              throw Error("Consumer's existing budget is exhausted");
            return task;
          },
          implement: async ({ project: selected, task: selectedTask }) => {
            assert.deepEqual(selected, project);
            assert.deepEqual(selectedTask, task);
            events.push(`${scenario}:implement`);
            sandbox.prompts.push(project.config.prompt);
            await runtime.save("artifact", {
              project: id,
              prompt: project.config.prompt,
            });
            return candidate;
          },
          currentCandidate: async () => candidate,
          validate: async () => {
            events.push(`${scenario}:validate`);
            return {
              candidate,
              passed: scenario !== "failed",
              evidence: [`evidence-${id}`],
            };
          },
          review: async ({ validation }) => {
            events.push(`${scenario}:review`);
            assert.deepEqual(validation.evidence, [`evidence-${id}`]);
            if (scenario === "review" && interrupt) {
              interrupt = false;
              throw Error("Unknown reviewer interruption");
            }
            await runtime.save("report", {
              project: id,
              evidence: validation.evidence,
            });
            return {
              candidate,
              approved: true,
              criteria: [
                {
                  id: "behavior",
                  satisfied: true,
                  evidence: `independent-review-${id}`,
                },
              ],
            };
          },
          deliver: async ({ review }) => {
            events.push(`${scenario}:deliver`);
            assert.equal(
              review.criteria[0].evidence,
              `independent-review-${id}`,
            );
            await runtime.save("handoff", { candidate, project: id });
            return { candidate, reference: `local-handoff-${id}` };
          },
          save: (state) => runtime.save("checkpoint", state),
        };
        const checkpoint = await runtime.read("checkpoint");
        try {
          return await runWorkflow({ project, operations, checkpoint });
        } finally {
          const snapshot = {
            checkpoint: await runtime.read("checkpoint"),
            artifact: await runtime.read("artifact"),
            report: await runtime.read("report"),
            budget: await runtime.read("budget"),
            mount: sandbox.mount,
            credential: sandbox.credential,
            prompts: sandbox.prompts,
          };
          const other = id === "A" ? "B" : "A";
          assert.doesNotMatch(
            JSON.stringify(snapshot),
            new RegExp(
              `prompt-${other}|fixture-token-${other}|evidence-${other}|independent-review-${other}|session-${other}`,
            ),
          );
          snapshots.push(snapshot);
        }
      },
    );
  assert.equal((await execute("success")).phase, "ready");
  const successful = [...events];
  assert.equal((await execute("success")).phase, "ready");
  assert.deepEqual(events, successful);
  await assert.rejects(execute("failed"), { code: "gate" });
  assert.equal(events.includes("failed:review"), false);
  assert.equal(events.includes("failed:deliver"), false);
  await assert.rejects(execute("review"), /Unknown reviewer interruption/);
  assert.equal((await execute("review")).phase, "ready");
  assert.equal(
    events.filter((event) => event === "review:implement").length,
    1,
  );
  assert.equal(events.filter((event) => event === "review:validate").length, 1);
  for (let retry = 0; retry < 2; retry++)
    await assert.rejects(execute("exhausted"), /existing budget is exhausted/);
  assert.equal(
    events.some((event) => event.startsWith("exhausted:")),
    false,
  );
  assert.equal(opens, closes);
  assert.equal(new Set(directories).size, 4);
  const held = snapshots.filter((snapshot) => snapshot.budget);
  assert.ok(held.length >= 2);
  for (const snapshot of held)
    assert.deepEqual(snapshot.budget, { used: 2, maximum: 2 });
}
assert.notEqual(roots[0], roots[1]);
const foreign = snapshots.find(
  (snapshot) => snapshot.checkpoint.phase === "ready",
).checkpoint;
let calls = 0;
await assert.rejects(
  runWorkflow({
    project: { id: "foreign-project", config: {} },
    checkpoint: foreign,
    operations: Object.fromEntries(
      [
        "selectTask",
        "implement",
        "currentCandidate",
        "validate",
        "review",
        "deliver",
        "save",
      ].map((name) => [
        name,
        async () => {
          calls++;
        },
      ]),
    ),
  }),
  { code: "invalid-checkpoint" },
);
assert.equal(calls, 0);
// The distributable itself owns these counters, rather than the consumer's legacy budget above.
for (const boundary of ["validate", "review", "deliver"]) {
  const checkpointFile = join(process.cwd(), `recovery-${boundary}.json`);
  const events = [];
  let receipt;
  let online = false;
  const project = { id: `independent-recovery-${boundary}`, config: {} };
  const operations = {
    selectTask: async () => ({
      id: "portable-task",
      criteria: [{ id: "C1", description: "Works" }],
    }),
    implement: async () => {
      events.push("implement");
      return "portable-candidate";
    },
    currentCandidate: async () => "portable-candidate",
    validate: async () => {
      events.push("validate");
      if (boundary === "validate")
        return {
          candidate: "portable-candidate",
          passed: false,
          evidence: ["Assertion failed"],
        };
      return {
        candidate: "portable-candidate",
        passed: true,
        evidence: ["assertion.log"],
      };
    },
    review: async () => {
      events.push("review");
      if (boundary === "review" && !online)
        throw new WorkflowOperationError("transport", "Disconnected");
      return {
        candidate: "portable-candidate",
        approved: true,
        criteria: [
          { id: "C1", satisfied: true, evidence: "Inspected behavior" },
        ],
      };
    },
    deliver: async () => {
      events.push("deliver");
      receipt = {
        candidate: "portable-candidate",
        reference: "portable-receipt",
      };
      if (boundary === "deliver")
        throw new WorkflowOperationError("transport", "Lost receipt response");
      return receipt;
    },
    save: async (state) => writeFileSync(checkpointFile, JSON.stringify(state)),
  };
  const load = () => JSON.parse(readFileSync(checkpointFile));
  if (boundary === "validate") {
    await assert.rejects(runWorkflow({ project, operations, recovery: {} }), {
      code: "exhausted",
    });
    await assert.rejects(
      runWorkflow({ project, operations, checkpoint: load() }),
      { code: "exhausted" },
    );
    assert.equal(events.filter((event) => event === "implement").length, 3);
    assert.equal(events.includes("deliver"), false);
  } else if (boundary === "review") {
    await assert.rejects(
      runWorkflow({
        project,
        operations,
        recovery: {
          wait: async () => {
            throw Error("restart boundary");
          },
        },
      }),
      /restart boundary/,
    );
    online = true;
    const resumed = await runWorkflow({
      project,
      operations,
      checkpoint: load(),
      recovery: { wait: async () => {} },
    });
    assert.equal(resumed.phase, "ready");
    assert.equal(resumed.recovery.operations["review/1"].attempts, 2);
    assert.equal(events.filter((event) => event === "implement").length, 1);
    assert.equal(events.filter((event) => event === "validate").length, 1);
  } else {
    await assert.rejects(runWorkflow({ project, operations, recovery: {} }), {
      code: "uncertain",
    });
    const resumed = await runWorkflow({
      project,
      operations,
      checkpoint: load(),
      recovery: {
        reconcile: async () => ({ status: "completed", value: receipt }),
      },
    });
    assert.equal(resumed.phase, "ready");
    assert.equal(events.filter((event) => event === "deliver").length, 1);
  }
}
console.log(
  "PASS: independent packaged A then B; success, failed validation, durable review resume, preserved consumer budget and foreign-checkpoint refusal",
);
