# Sandcastle checks and read-only discovery

From the repository root, use Node 22+, npm and Git. Keep the original invocation
SDK `@ai-hero/sandcastle@0.12.0` distinct from the separately pinned customized
candidate `@rebuild/sandcastle`. Package provenance is in
[baseline.json](../../.sandcastle/baseline.json) and the
[package/isolation record](../../.sandcastle/PACKAGE-ISOLATION.md).

```powershell
npm.cmd ci --offline --ignore-scripts --no-audit --no-fund
npm.cmd run sandcastle:check
npm.cmd run sandcastle:test
npm.cmd run test:sandcastle-baseline
```

Offline installation requires the prepared npm cache. Ordinary authorized setup
can use `npm.cmd ci --ignore-scripts --no-audit --no-fund` with network access.
The candidate dependency remains the content-addressed archive selected by
package.json and its lock; do not substitute a mutable link.

| Command | Boundary and evidence |
| --- | --- |
| `sandcastle:check` | Original installed SDK version/registry integrity and public-import provenance |
| `sandcastle:test` | Current explicit public workflow and installed-consumer/Editor adapter checks, as enumerated in [package.json](../../package.json) |
| `test:sandcastle-baseline` | Frozen #32 fixture: all 15 original checksums and original 32 orchestration tests in temporary Git repositories with simulated GitHub responses |
| `sandcastle` or `sandcastle:queue` | Read-only real GitHub queue discovery using explicit repository/labels; blocked/eligible reasons, no agents |

Keep [frozen fixture bytes](../fixtures/sandcastle-baseline/README.md) unchanged.
Its tests preserve the old orchestration baseline, not the current workflow
implementation. See [the preservation record](../../docs/sandcastle-baseline.md).

The current portable suites exercise real temporary Git/worktree behavior with
scripted system/provider boundaries. They do not establish live provider,
unattended endurance, or Windows/native acceptance. Queue discovery needs
available GitHub read credentials; unavailable discovery errors instead of
pretending the queue is empty. Local handoffs do not close GitHub blockers.
Do not restart the paused original runner or automatically enqueue tasks.

For stable adapter contracts see [single-task](../../.sandcastle/SINGLE-TASK.md),
[recovery](../../.sandcastle/RECOVERY.md),
[evidence identity](../../.sandcastle/EVIDENCE.md),
[review output](../../.sandcastle/REVIEW-OUTPUT.md),
[measurements](../../.sandcastle/MEASUREMENTS.md) and
[complete context packets](../../.sandcastle/CONTEXT-PACKETS.md).
Generic workflow policy remains in the fork's `docs/workflow*.md`.
Editor-specific [native acceptance](native-validation.md) remains independent.
Host outcomes and raw evidence belong to retained reports, not moving completion
claims in committed procedures.
