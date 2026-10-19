import test from "node:test";
import assert from "node:assert/strict";
import { inspectQueue, issueCriteria } from "./queue.mjs";

const issue = (number, changes = {}) => ({
  number,
  title: `Task ${number}`,
  state: "open",
  body: "## Acceptance criteria\n\n- [ ] Required behavior\n- [x] Existing contract\n",
  labels: [],
  assignees: [],
  sub_issues_summary: { total: 0 },
  issue_dependencies_summary: { blocked_by: 0 },
  ...changes,
});

test("queue inspection follows live metadata without an issue-number allowlist", async () => {
  let labels;
  const reader = {
    issues: async (input) => {
      labels = input;
      return [
        issue(1004),
        issue(2025, { issue_dependencies_summary: { blocked_by: 1 } }),
        issue(87, { sub_issues_summary: { total: 4 } }),
        issue(4100, { pull_request: {} }),
        issue(9000, { issue_dependencies_summary: undefined }),
      ];
    },
  };
  const result = await inspectQueue(
    { repository: "example/consumer", queueLabels: ["consumer-label"] },
    { reader },
  );
  assert.deepEqual(labels, ["consumer-label"]);
  assert.deepEqual(
    result.issues
      .filter((value) => value.eligibleOnGitHub)
      .map((value) => value.number),
    [1004],
  );
  assert.equal(
    result.issues.find((value) => value.number === 2025).reason,
    "1 open dependencies",
  );
  assert.equal(
    result.issues.find((value) => value.number === 9000).reason,
    "dependency metadata unavailable",
  );
  assert.equal(result.execution, "disabled while the pipeline is rebuilt");
});

test("criterion extraction retains checked requirements and excludes unrelated sections", () => {
  assert.deepEqual(
    issueCriteria(issue(1).body + "\n## Notes\n- [ ] unrelated"),
    [
      { id: "C1", description: "Required behavior" },
      { id: "C2", description: "Existing contract" },
    ],
  );
  assert.deepEqual(
    issueCriteria(
      "## Acceptance criteria\r\n- [ ] First line\r\n  continued requirement\r\n- [ ] Final requirement",
    ),
    [
      { id: "C1", description: "First line\ncontinued requirement" },
      { id: "C2", description: "Final requirement" },
    ],
  );
  assert.deepEqual(issueCriteria("## Notes\n- [ ] unrelated"), []);
});

test("a discovery failure remains visible instead of being reported as an empty queue", async () => {
  await assert.rejects(
    inspectQueue(
      { repository: "example/consumer" },
      {
        reader: {
          issues: async () => {
            throw new Error("discovery unavailable");
          },
        },
      },
    ),
    /discovery unavailable/,
  );
});
