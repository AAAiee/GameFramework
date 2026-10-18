# Explicit durable task recovery

The integrated recovery package selects committed fork revision
`014f16b7dcb99b98ec9d13c70b4f4c1d525dc592`, built offline with the pinned Linux
image/toolchain. Archive SHA-256 is
`07f6068740d30c9b3613e018feb8d9ad04be3a0be2c954a10fd3bf06ff42886d`;
[package-provenance.json](package-provenance.json), the vendor archive, dependency
manifest and lockfile carry the same identity. Original registry SDK remains
exact `0.12.0`. One canonical build was performed for this increment; independent
final artifact reproduction remains a #43 gate.

`runEditorTask({ ...options, recovery: {} })` enables the fork's fixed recovery
budgets for a new task/run: three implementation attempts total and three
transport attempts for each operation/candidate. Existing fail-fast callers keep
their behavior. A saved recovery ledger remains active even when the option is
omitted on resume. Counters, recoverable phase, failures and backoff deadlines
live in the project/run-scoped checkpoint; ordinary restart cannot reset them.

The boundary that knows the failure meaning may throw `WorkflowOperationError`
from `@rebuild/sandcastle/workflow` with kind `transport`, `substantive`,
`permanent` or `format`. Transport retries only the interrupted operation;
substantive failures request implementation repair with fresh host acceptance
and independent review. Repair receives the failed stage/message in its prompt.
Unknown SDK/provider errors stop for inspection. An exit code or provider message
alone is insufficient to infer a connection failure. Existing agents, model
configuration and caller cancellation/run duration are unchanged.

The adapter reconciles only its local handoff automatically. A saved handoff
must match the project, task, branch, candidate, validation and review, and the
actual clean candidate must still match. A matching receipt completes an
interrupted delivery without writing another handoff. Missing handoff permits a
classified retry; mismatched or unavailable observation stops safely. This does
not publish, create a PR, push, merge, comment on, or close a GitHub issue.

Interrupted implementation requires an explicit consumer
`recovery.reconcile(context)` that inspects its owned SDK session/worktree. Return
`completed` with the actual candidate, `not-started` with proof of safe replay, or
`uncertain` to stop. The adapter never treats an arbitrary new commit as proof
that the requested implementation finished. Reconciliation reads have their own
persisted transport budget. Unknown-effect errors cannot authorize blind replay.

Runtime locks are retained when sandbox creation/cleanup has an unknown outcome.
The fork's explicit `reconcileWorkflowRuntimeLock` requires the exact saved owner,
matching scope and observable resource cleanup before releasing that lock. Do not
delete lock files or checkpoints to force a retry. Prepared worktree restoration
records remain separate: unresolved restoration still stops host validation.

Measurements receive implementation/transport indices from the durable ledger,
so connection retries do not become additional code attempts. The independent
packaged consumer exercises built-in budget exhaustion, review resume and
lost-delivery reconciliation, alongside project A/B isolation and the preserved
legacy consumer budget.

The default `npm run sandcastle` remains read-only queue inspection during the
rebuild. No task is automatically enqueued by this increment. Evidence identity,
one-shot review-format correction and final AFK acceptance remain later slices.
