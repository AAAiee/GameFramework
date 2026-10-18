# Single-task increment: #33

The customized fork adds one opt-in, Promise-based workflow entrypoint. It owns
stage ordering, candidate/acceptance/review gates, and explicit checkpoint
progress. It contains no Editor repository names, issue numbers, native paths,
provider error matching, GitHub writes or automatic retry loop.

The Editor adapter in [task.mjs](task.mjs) supplies the actual Git candidate,
original SDK agent invocations, host validation, JSON checkpoint persistence and
a local handoff. Its public function is `runEditorTask(options)`. The caller
supplies one task and owns the sandbox lifecycle; no task is selected from a
live issue queue in this increment.

Required options are `projectId`, `task`, `sandbox`, `agents: { implementer,
reviewer }`, `prompts: { implement, review }`, `validate` and task-scoped `storage`.
`signal` is optional. The validation callback receives `{ worktree, candidate,
signal }` and returns `{ candidate, passed, evidence }`; Editor native acceptance
remains a host operation supplied by the caller. The adapter never interprets a
task number to choose validation. Reviewer output must be one JSON object;
format extraction/correction belongs to the later #39 increment.

The caller must serialize a task's storage/worktree. The adapter rejects an
unexpected branch, unfinished changes and changed saved task requirements.
Checkpoint files are flushed and atomically replaced. Local handoffs bind the
project, task, branch, exact candidate, validation and criterion review. Existing
handoffs are preserved for reconciliation instead of overwritten.

## Verified behavior

- The fork's 18 public-function cases cover successful task completion, no work,
  validation and review gates, actual candidate changes, review checkpoint reload,
  unfamiliar errors, uncertain side effects, persistence failure and cancellation.
- Eight consumer checks pass: the four original SDK baseline checks, three
  Editor adapter integration checks, and an independent installed-package check.
- The adapter integration uses real Git worktrees and the original published SDK.
  Agent responses are scripted through the SDK's public sandbox-provider seam.
  The host executes a real assertion on generated code through `node --test`.
  Failed validation cannot invoke the reviewer or create a handoff. Reloaded
  review state repeats neither implementation nor host validation.
- The independent consumer installs the built archive offline into a fresh
  temporary directory. It completes interrupted-review recovery without Editor
  configuration, an Editor checkout, model credentials or remote operations.
- Fork build, typecheck and the public declaration check pass. The workflow
  documentation is included in the package. The full SDK compatibility suite
  passes 1,465 tests across 54 files, with two platform skips. Logs are under
  `logs/workflow-33/`; `sdk-suite-result.json` records the isolated test setup.

The candidate archive is `vendor/sandcastle-0.12.0-4fe71120a6e6.tgz`, SHA-256
`4fe71120a6e6f8efca4e91fdda8a97aea67ad2d106bef68bd39bff74b7b15d2e`.
Its fork implementation is saved in local commit `6611bb4`.
The installed workflow module matches the built fork module byte for byte.
The original registry SDK remains independently pinned and unchanged.

These are deterministic integration checks. No live model task or Editor native
Debug/Release/OpenGL/dialog acceptance was run for this slice; native code and
its validators were unchanged. The SDK retains upstream provider diagnostics.
This increment does not establish complete unattended reliability.

The initial consumer test run exposed two test-environment problems: concurrent
SDK fixtures shared the user's Git config, and a nested Node test inherited the
parent test-runner marker. Fixtures now use isolated Git config; the host child
uses an independent Node test environment. The full-SDK Linux setup initially
mounted dependencies at a path without the `node_modules` directory name; its
corrected mount preserves normal Node package resolution. These were verification
setup corrections, not additions to production error handling.

## Fixed execution baseline

`@ai-hero/sandcastle` is still the original exact registry package. Only the
candidate workflow import uses `@rebuild/sandcastle/workflow`. `package-lock.json`
pins both independently. In this increment `npm.cmd run sandcastle` performed
the original SDK check. The subsequent [Git/GitHub increment](GIT-GITHUB.md)
changes it to explicit read-only queue inspection; `sandcastle:check` retains the
SDK check. Neither command starts agents or uses the inherited prompts.

Recovery budgets, evidence identity beyond the candidate, review-format repair,
queue discovery, project locks and automatic reconciliation remain separate
increments. If implementation or delivery may have performed an effect before
an error, the saved marker blocks replay; reconciliation is required. Passing a
review checkpoint to the entrypoint explicitly resumes the read-only review.

All changes remain local on `rebuild/33-single-task`. The baseline branches,
original checkouts, earlier feature work and #32 handoff remain preserved. Pushes,
PR creation, merge, comments, labels and issue closure remain human actions.
