// Copied beside consumer-fixture.mjs into the same fresh installed consumer.
// All workflow imports resolve from that consumer, without Editor modules.
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { existsSync, mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";
import {
  runWorkflow,
  WorkflowOperationError,
} from "@rebuild/sandcastle/workflow";

const scenarios = [
  "implementation",
  "transport",
  "format",
  "correction-transport",
];
const root = join(process.cwd(), "budget-fixtures");
const scenario = process.argv[2];
const phase = process.argv[3];
if (!scenario) {
  assert.equal(
    existsSync(root),
    false,
    "Use a fresh consumer; preserve earlier evidence.",
  );
  mkdirSync(root);
  const reports = [];
  for (const name of scenarios) {
    for (const step of ["initial", "restart"]) {
      execFileSync(
        process.execPath,
        [fileURLToPath(import.meta.url), name, step],
        {
          cwd: process.cwd(),
          encoding: "utf8",
          windowsHide: true,
          timeout: 30000,
        },
      );
    }
    const initial = JSON.parse(
      readFileSync(join(root, `${name}-initial.json`), "utf8"),
    );
    const restart = JSON.parse(
      readFileSync(join(root, `${name}-restart.json`), "utf8"),
    );
    assert.notEqual(
      initial.process,
      restart.process,
      "Restart uses a fresh Node process",
    );
    assert.deepEqual(
      restart.events,
      initial.events,
      "Restart invokes no replacement callbacks",
    );
    reports.push({ scenario: name, initial, restart });
  }
  writeFileSync(
    join(root, "summary.json"),
    JSON.stringify(reports, null, 2) + "\n",
  );
  console.log(JSON.stringify(reports));
  console.log(
    "PASS: installed consumer preserves all exhausted budgets across fresh Node processes",
  );
} else {
  assert.ok(scenarios.includes(scenario));
  assert.ok(["initial", "restart"].includes(phase));
  const path = join(root, `${scenario}-checkpoint.json`);
  const eventPath = join(root, `${scenario}-events.json`);
  const events = existsSync(eventPath)
    ? JSON.parse(readFileSync(eventPath, "utf8"))
    : [];
  const record = (event) => {
    events.push(event);
    writeFileSync(eventPath, JSON.stringify(events, null, 2) + "\n");
  };
  const candidate = "independent-budget-candidate";
  const task = {
    id: `budget-${scenario}`,
    criteria: [{ id: "C1", description: "Behavior works" }],
  };
  const approval = {
    task: task.id,
    candidate,
    approved: true,
    criteria: [{ id: "C1", satisfied: true, evidence: "Independent review" }],
  };
  const options = {
    project: { id: `independent-${scenario}`, config: {} },
    recovery: { wait: async () => {} },
    operations: {
      selectTask: async () => task,
      implement: async () => {
        record("implement");
        return candidate;
      },
      currentCandidate: async () => candidate,
      validate: async () => {
        record("validate");
        return {
          candidate,
          passed: scenario !== "implementation" || phase === "restart",
          evidence: ["host.json"],
        };
      },
      review: async () => {
        record("review");
        if (phase === "restart") return approval;
        if (scenario === "transport")
          throw new WorkflowOperationError("transport", "Offline fixture");
        return scenario === "implementation"
          ? approval
          : "Malformed review fixture";
      },
      deliver: async () => {
        record("deliver");
        return { candidate, reference: "local-fixture-receipt" };
      },
      save: async (state) =>
        writeFileSync(path, JSON.stringify(state, null, 2) + "\n"),
    },
  };
  if (["format", "correction-transport"].includes(scenario)) {
    options.reviewOutput = {
      correct: async () => {
        record("correction");
        if (phase === "restart") return approval;
        if (scenario === "correction-transport")
          throw new WorkflowOperationError(
            "transport",
            "Correction offline fixture",
          );
        return "Still malformed fixture";
      },
    };
  }
  if (phase === "restart")
    options.checkpoint = JSON.parse(readFileSync(path, "utf8"));
  await assert.rejects(runWorkflow(options), {
    code: scenario === "format" ? "format" : "exhausted",
  });
  const count = (name) => events.filter((event) => event === name).length;
  assert.equal(count("implement"), scenario === "implementation" ? 3 : 1);
  assert.equal(count("validate"), scenario === "implementation" ? 3 : 1);
  assert.equal(
    count("review"),
    scenario === "implementation" ? 0 : scenario === "transport" ? 3 : 1,
  );
  assert.equal(
    count("correction"),
    scenario === "format" ? 1 : scenario === "correction-transport" ? 3 : 0,
  );
  assert.equal(count("deliver"), 0);
  writeFileSync(
    join(root, `${scenario}-${phase}.json`),
    JSON.stringify(
      {
        scenario,
        phase,
        process: process.pid,
        events,
        outcome: "exhaustion retained; no delivery",
      },
      null,
      2,
    ) + "\n",
  );
}
