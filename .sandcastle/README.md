# Original Sandcastle baseline

This branch is the first increment of the pipeline rebuild. It consumes the
original npm package `@ai-hero/sandcastle@0.12.0`, with an exact version and npm
integrity in the lockfile. It imports only the package's existing public APIs.
There is no customized `@ai-hero/sandcastle/workflow` dependency.

The original published source revision is
`e99f832f26dc9d245c019a9ddd19fa5dee792427`. Package provenance is recorded in
[baseline.json](baseline.json). The separate fork worktree
`../sandcastleCustomized-pipeline-rebuild` starts at that exact upstream commit
and has no custom source changes.

## Run this increment

From `C:\FinalPolishedProjects\Editor-pipeline-rebuild`:

```powershell
npm.cmd ci --offline --ignore-scripts --no-audit --no-fund
npm.cmd run sandcastle:check
npm.cmd run sandcastle:test
```

The offline command requires the already prepared npm cache. Ordinary setup can
use `npm.cmd ci --ignore-scripts --no-audit --no-fund` with network access.

`npm.cmd run sandcastle` performs the same SDK provenance check. It starts no
agents and accepts no execution flags. The inherited implementation/review prompt
files are not loaded. Automated ticket execution remains paused; the original
template's issue-closing instructions are not enabled.

## Observed baseline

Four public-API tests pass in approximately 3.5 seconds on Node 22.13.0/Windows:

| Check                               | Observed behavior                                                                                     |
| ----------------------------------- | ----------------------------------------------------------------------------------------------------- |
| Registry package and public imports | Exact original version and lock integrity match; custom workflow subpath is absent                    |
| Successful task                     | A deterministic agent fixture commits on its named task branch; the owner checkout stays unchanged    |
| Unfamiliar provider failure         | One invocation fails visibly, unfinished work remains after sandbox cleanup, and no retry is invented |
| Cancellation                        | A pre-aborted invocation starts no agent; the existing sandbox handle remains usable                  |

The Git repositories and worktrees in these tests are real temporary fixtures.
The sandbox transport and agent responses are deterministic test doubles. These
tests use no model credentials, Docker, GitHub writes, or Editor native builds.
They do not establish unattended reliability or exercise actual provider traffic.
A fresh offline `npm ci` followed by all four tests also passed.

The original SDK already owns worktree management, agent invocation, cancellation
and sandbox cleanup. Its documented policy is to fail fast on provider errors;
persistent workflow recovery belongs in a separately tested pipeline layer.

## Rebuild sequence

Each increment must have a bounded diff and evidence at its public boundary.
Keep the SDK/package used to execute work fixed while testing its successor.
Existing draft changes are reference material, not automatically imported.

| Increment                  | Ticket      | Completion evidence                                                                                                                     |
| -------------------------- | ----------- | --------------------------------------------------------------------------------------------------------------------------------------- |
| 0. Original SDK baseline   | This branch | Original package provenance, offline installation, the four checks above, unchanged existing checkouts                                  |
| 1. One complete task       | #33         | Explicit project operations; implementation, host validation, independent review, local handoff; failed gates cannot produce a handoff  |
| 2. Git/GitHub boundary     | #34         | Project-supplied repository identity, safe Git integration and read-only issue discovery; local edits survive failures                  |
| 3. Packaging and isolation | #35         | Built package consumed by unrelated projects A and B; no inherited paths, prompts, credentials or evidence                              |
| 4. Measurements            | #36         | Baseline stage duration, calls, outcomes and unavailable measurements recorded before optimization                                      |
| 5. Recovery                | #37         | Checkpoint reloads, bounded persisted budgets, unknown-error preservation, process interruption and explicit recovery; no hidden resets |
| 6. Evidence and review     | #38–#39     | Changed validation inputs invalidate evidence; format correction remains separate from code repair                                      |
| 7. Diagnostics and context | #40–#42     | Project adapters supply native checks and focused context; final acceptance remains enforced                                            |
| 8. Final verification      | #43         | Repeatable failure scenarios and independent consumers against final artifacts; report interventions, failures and limitations          |

The doctest/CI work in #30–#31 remains a separate workstream. No ticket is marked
complete merely because an old draft contains similar code. PR creation, pushes,
merges, issue comments, labels and closure remain human actions.

## Preservation

The existing `Editor` and `sandcastleCustomized` checkouts, their local branches,
completed #12–#25/#29 work, #32 handoff, logs, credentials and paused state remain
in place. Modified and untracked source files were additionally copied to
`C:\FinalPolishedProjects\SandcastleRecovery\2026-10-18T06-02-33-525Z` with
SHA-256 manifests, staged/unstaged binary patches and recorded refs. That archive
is a source backup, not a complete Git clone; ignored runtime data stays in the
original checkouts.

This Editor worktree starts at accepted commit
`317bd1b93e632b70700f52407c5b66c81e5cbce3`, which includes merged doctest work.
The rebuild has not been promoted into the existing checkout. No live queue
should be restarted until its replacement has been reviewed and explicitly
authorized for execution.
