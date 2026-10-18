# Git/GitHub increment: #34

This records the frozen #34 contract. The subsequent [#35 increment](PACKAGE-ISOLATION.md)
uses runtime-owned records and `prepareValidation` factories to scope prepared
worktrees to a compatible project/run.

This increment is local on `rebuild/34-git-github` in the separate Editor and
customized-fork rebuild worktrees. It has not been promoted into the original
checkouts, and no GitHub issue has been closed.

## Behavior

The fork's `@ai-hero/sandcastle/workflow/integrations` entrypoint supplies a
shell-free Git/GitHub command runner, optional tool/proxy discovery, bounded
read-only GitHub collections, explicitly injected publication reconciliation,
and prepared-worktree preservation/restoration. The module contains no Editor
repository names, labels, task numbers, prompts or native acceptance commands.
See the packaged `docs/workflow-integrations.md` for its public contract.

Editor supplies repository/labels and optional discovery/timeouts in
[project.mjs](project.mjs). [queue.mjs](queue.mjs) reports GitHub eligibility from
live open issues and native dependency metadata. It excludes PR records,
parents, assigned tasks, missing metadata, unresolved dependencies and tasks
without checkable acceptance criteria. Criteria retain checked requirements and
indented continuation lines. There is no issue-number allowlist.

`npm.cmd run sandcastle` and `sandcastle:queue` now perform real **read-only queue
inspection**. `sandcastle:check` separately checks the original registry SDK.
None of these commands executes tickets. The observed live queue has #30, #31
and #32 without tracker blockers; #33–#43 still have open dependencies. A local
handoff does not silently satisfy or close GitHub dependencies.

[task.mjs](task.mjs) accepts optional `validationWorktree: { source, path,
command }`. The caller still supplies native host validation through `validate`.
The reusable helper validates in that separate prepared worktree at the exact
candidate, reuses ignored caches and restores staged/unstaged/untracked edits.
The adapter durably records `validation-recovery.json` and refuses another
validation while that record has unresolved recovery. Backup stashes are retained;
Git filters and line-ending rules apply. Callers serialize repository operations.

GitHub reads have bounded transient retries, pagination and invocation deadlines.
Unknown failures stop visibly. The optional publication helper is tested with
fake capabilities only; Editor supplies no remote write operation. PR creation,
push, merge, comments, labels and closure remain human actions.

## Evidence

- Seventeen integration contract cases pass on Windows. Real Git repositories
  cover source/prepared edit preservation, untracked collision refusal, separate
  worktree enforcement, ignored-cache reuse, validation failure restoration,
  new-work preservation and an actual rejected Git stash application.
- Simulated GitHub tests cover GET pagination, bounded transient reads, permanent
  failures, page-limit refusal, lost-write response reconciliation, unavailable
  reconciliation, unknown write outcomes, exact publication markers and
  cancellation before a write. Environment tests verify optional discovery,
  explicit proxy overrides and unchanged model/host routing.
- Thirteen Editor consumer checks pass, including the original SDK baseline,
  installed-package workflow, queue policy and actual prepared-worktree adapter.
  Host assertions execute generated code through `node --test`. An unresolved
  recovery record survives another attempted validation.
- Fork typecheck, build, public declaration check, formatting and installed
  artifact/source-map/documentation comparisons pass. The original registry SDK
  remains pinned to `0.12.0` separately from the customized alias.
- The complete SDK compatibility suite passes 1,482 tests across 55 files, with
  two platform skips, in an isolated Linux Git checkout without network or model
  credentials. This includes all seventeen integration contract cases.
- The original Editor's 99 preserved source files and fork's 15 preserved source
  files match their backup SHA-256 manifests; both original HEADs, branches and
  working-tree statuses remain unchanged.

The installed candidate archive is
`vendor/sandcastle-0.12.0-1c5bf08555b4.tgz`, SHA-256
`1c5bf08555b42f6052906f0d2528c1253633a20582633e72a556687c54d0daed`.
Detailed logs and machine-readable artifact/preservation results are retained in
`logs/workflow-34/`.

These checks establish the integration boundary, not unattended reliability. No
live model task or Editor native Debug/Release/OpenGL/dialog validation was run.
Native code and acceptance scripts are unchanged. Cross-project installation,
measurements, persisted recovery budgets, evidence identity and review-format
correction remain the subsequent #35–#39 increments before final #43 verification.
