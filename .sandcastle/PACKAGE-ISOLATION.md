# Package and isolation increment: #35

This page records the #35 package/isolation verification. The current package
updates and measurement/recovery limits are recorded in [MEASUREMENTS.md](MEASUREMENTS.md),
[RECOVERY.md](RECOVERY.md), [EVIDENCE.md](EVIDENCE.md) and
[package-provenance.json](package-provenance.json); archive hashes below are
the historical #35 artifact.

The fork adds `@ai-hero/sandcastle/workflow/runtime`, and Editor consumes a fixed
archive through `@rebuild/sandcastle`. The original registry SDK stays independently
pinned to exact `0.12.0`. [package-provenance.json](package-provenance.json) records
the selected source revision, archive hash, immutable build image, lock hash and
Node/npm toolchain. No mutable npm link is used.

## Current task adapter

`runEditorTask(options)` accepts explicit `projectId`, `task`, `agents`, `prompts`,
host `validate`, `runtime: { repository, runId, compatibility, command }`, and an
`openSandbox({ runtime, branch })` factory. The factory supplies the original SDK
sandbox using its returned run-scoped branch. Optional `signal` supports cancellation.
The adapter acquires the repository writer lock before calling this factory and
owns sandbox close through the runtime resource lifecycle.

The current adapter also requires project-owned `context` (complete scope,
immutable base, entrypoints, commands and references). See
[CONTEXT-PACKETS.md](CONTEXT-PACKETS.md) for stage packets, sandbox-readable evidence
copies and context freshness. Keep acceptance criteria in the task rather than
duplicating them in scope or the static prompts.

The optional `prepareValidation({ directory, runtime })` factory returns
`{ source, path, command }`. It creates or reopens a prepared validation worktree
**inside its supplied resource directory**; use `runtime.key` for any Git branch
name. Compatible files/caches can then survive successive invocations of that run.
Windows/native setup and acceptance commands remain consumer operations. A
different project/run receives a different directory; changed compatibility stops
before reuse. Previously unresolved worktree recovery continues to block validation.

Checkpoint, recovery and local handoff use `runtime.read/save` records, with a
full identity envelope. The handoff payload is the record's `value` field. This
removes the adapter's duplicate atomic-JSON implementation and caller-chosen
storage path. Historical #33/#34 files remain on their frozen branches. Old
runner state is not automatically migrated or cleared.

One lock covers linked worktrees of the same Git repository. Another run fails
before opening a sandbox. Resource handles cannot be used after the runtime
callback closes. Unknown creation outcomes or failed cleanup retain the lock for
inspection; no PID-based lock theft, hidden retry loop or budget reset is added.

## Validation and limits

- Forty-seven fork workflow/integration/runtime contract cases pass on Windows.
  The complete SDK compatibility suite passes 1,494 tests across 56 files with two
  platform skips, in isolated Linux fixtures without network or credentials.
- Fourteen Editor consumer checks pass, including real Git/source preservation,
  host assertions, prepared-worktree restoration, review resume and acquiring the
  lock before sandbox creation.
- A fresh offline-installed consumer runs A then B in the same process with no
  Editor imports. It exercises success, failed validation, interrupted review
  resume, foreign-checkpoint rejection and preservation of an injected existing
  consumer budget's exhausted state. It checks prompts, task context, resource
  mounts, fake credentials/sessions, artifacts, reports, checkpoints and evidence
  remain with their project/run. This does not introduce #37's recovery budgets.
- The test inspects the actual installed package, including source maps, for
  Editor repository paths/identity, adapters, credential files, runtime logs and
  session directories. Generic upstream scaffold templates remain intentional.

Build/install logs and reproduction results are under `logs/workflow-35/`.
Two independent offline builds of fork revision
`0ba0c0f783d14b1fac74cd182d688020cdc0d8b6` produce the identical archive SHA-256
`e24cfb53c1be5149b0990297c6cbacac8cf65d732557f916a04ebc08d18d271e`.
Editor installs `vendor/sandcastle-0.12.0-e24cfb53c1be.tgz`; its actual source maps
and packaged documentation match that committed source byte for byte. The build
image and dependency lock hash are recorded in the tracked provenance file.

Repeated installs use the committed lockfile and prepared npm cache:

```powershell
npm.cmd ci --offline --ignore-scripts --no-audit --no-fund
npm.cmd run sandcastle:check
npm.cmd run sandcastle:test
```

The fork's packaged `docs/reproducible-build.md` describes deliberate updates from
an exact source revision. Reproduction uses canonical Git source, a fixed local
Docker image and matching prepared Linux dependencies; it performs no downloads.
Runtime compatibility strings must change deliberately when resource inputs
change. Evidence identity for policy/toolchain/task changes belongs to #38.

No live model task or Editor native C++ acceptance was run for this slice. These
checks do not establish complete AFK reliability. `npm.cmd run sandcastle` remains
read-only queue inspection. Metrics, persisted recovery, evidence and review
correction are subsequent increments. Original checkouts, origin, paused runner
and prior handoffs remain preserved. PRs and other remote writes remain human actions.
