# Portable workflow verification

This is the repeatable verification procedure, not a completion report. Current
artifact identity comes from [package-provenance.json](package-provenance.json).
Actual outcomes belong to reports naming the tested fork revision, Editor
candidate, archive SHA-256, policy, dependency and observed host identities.
Commit source and these procedures before freezing the acceptance candidate;
keep final host reports outside tracked source to avoid a self-SHA update cycle.

## Installed public boundary

After deliberately updating the archive, provenance, manifest and lock together,
install from the prepared npm cache and run the existing consumer entrypoints:

```powershell
npm.cmd ci --offline --ignore-scripts --no-audit --no-fund
npm.cmd run sandcastle:check
npm.cmd run sandcastle:test
```

For a package-only focused check, use `node --test .sandcastle/package.test.mjs`
instead of the full suite; the full suite already includes that fixture.

[package.test.mjs](package.test.mjs) creates a fresh independent npm consumer,
installs only the pinned archive offline, and inspects the actual installed files,
including source maps, for repository paths, project context and credential or
session artifacts. It requires public workflow, integration, runtime, measurement
and process JavaScript exports, declarations, maps and packaged documentation.
The original invocation SDK stays exactly `@ai-hero/sandcastle@0.12.0`.

[consumer-fixture.mjs](consumer-fixture.mjs) imports only installed public APIs.
It runs project A then B with isolated repositories, prompts, criteria, fake
credentials, sessions, resources, artifacts, reports and checkpoints. It covers
success and ready resume, failed validation, interrupted review, transport resume,
lost-write reconciliation, changed environment evidence and strict review-output
correction. Foreign project checkpoints are refused before operation callbacks.

[consumer-budget-fixture.mjs](consumer-budget-fixture.mjs) adds four exhausted
budget cases: three implementation attempts, three review transport attempts,
one unsuccessful format correction, and three transport attempts for that same
correction. Each initial invocation and ordinary restart runs in a separate Node
process, reading the persisted checkpoint and event files. Restart callbacks
would pass validation or approve review if called; every case must still refuse
delivery and invoke no additional stage callback. The test prints both phases'
events and process identities into its TAP diagnostics.

The package test removes its temporary consumer after the test. For retained
checkpoint evidence, create a separate fresh offline npm consumer outside the
checkout, install the same absolute archive path, copy both fixture files there,
and run each with `node` from that directory. Preserve its manifest, lock,
`budget-fixtures/`, stdout, stderr and archive hash. Never reuse an earlier
consumer directory or relabel its results after changing the pin.

The fork's public evidence suite separately verifies pending correction after
changed validation inputs: new validation and review occur without another
implementation, and the old correction cannot authorize delivery. Editor's
[native.test.mjs](native.test.mjs) and [task.test.mjs](task.test.mjs) verify dependent
selector coverage, complete Debug fallback, the final-acceptance gate, prepared
edit restoration, real process quiescence before restoration and preservation
when ownership is unknown. These complement the installed portability fixture.
The fork's public diagnostics suite covers bounded diagnostic failure, transport
budget reload and failure of final acceptance after passing diagnostics.

## Artifact and final acceptance order

1. Record clean committed fork and Editor heads. Build the fork twice into
   distinct fresh output directories with its packaged
   `docs/reproducible-build.md` procedure, the fixed local Linux image and matching
   offline dependencies. Compare full archive SHA-256 values. Preserve both
   build commands, logs and provenance. A first build is reusable only if its
   exact committed fork source is unchanged.
2. Pin that artifact deliberately, commit the consumer lock and provenance,
   install fresh, and run the checks above. Keep source maps, declarations and
   packaged documentation in the inspection. Run the installed public Windows
   process smoke with the actual discovered PowerShell executable, including
   primary exit propagation and residual descendant shutdown.
3. Run fork public tests, typecheck, build and format check using its existing
   scripts. Run the complete SDK suite in the isolated Linux harness with source
   read-only, prepared dependencies and no network or credentials. Serialize
   heavy native builds and real-Git suites to avoid competing fixture timeouts.
4. Against the frozen consumer candidate and canonical installed package, run
   the [native adapter](NATIVE.md)'s complete final plan: project generation,
   Debug and Release Rebuild, both full Regression suites, 26 standalone selector
   runs, ten private-desktop wrappers, and both existing UnitTests procedures.
   This is 43 outer checks including generation. Each UnitTests procedure retains
   normal/filter passes, intentional console/JUnit failures and a rebuilt passing
   runner. Diagnostics alone never satisfy this gate.
5. Preserve raw command output, per-check durations, report identities, private
   desktop observations, original-edit manifests and final resource/lock state.
   Changed source, package, policy, dependency or observed host inputs require
   relevant reruns; a historical pass never silently approves the new identity.

Discover PowerShell's actual executable and record its version for each run.
Store application updates can change its path. Use the owned process runner and
private desktop fail-closed behavior; preserve prepared dependencies, ignored
build outputs and staged, unstaged and untracked edits. Follow
[Tests/README.md](../Tests/README.md) for current project test commands.

## Evidence and measurement record

Use an external bundle with a manifest/index containing:

| Record       | Required contents                                                                                                            |
| ------------ | ---------------------------------------------------------------------------------------------------------------------------- |
| Identities   | Fork/Editor heads, archive/provenance and lock hashes, host/tools, validation policy and dependency observations             |
| Commands     | Working directory, executable/arguments, start/end, exit status, stdout/stderr log paths                                     |
| Consumer     | Offline installation, package inspection, A/B isolation, checkpoint/event files for both restart phases                      |
| Tooling      | Public and full SDK results with counts/skips, type/build/format results, explicit unrun checks                              |
| Native       | Authoritative JSON/Markdown reports, raw check logs, per-check duration/outcome, cleanup receipts and preservation manifests |
| Measurements | Stage attempts/outcomes, transport/correction retries, available provider usage and explicit missing fields                  |
| Limitations  | Scripted versus live traffic, interventions, historical failed runs, unrun checks and human publication/review status        |

Preserve [measurement-baseline.json](measurement-baseline.json) unchanged.
Its single scripted-agent/real-Git/host-Node observation used Node 22.13.0 on
Windows x64, candidate `578b6dde28b74c752285845732ebdcd582e220ed`:

| Stage/check                 | Observed elapsed time |
| --------------------------- | --------------------: |
| Implementation              |          1259.9349 ms |
| Host validation             |           210.8507 ms |
| Nested host Node check      |           196.6185 ms |
| Independent scripted review |           957.2521 ms |
| Local handoff               |           200.7838 ms |

Provider usage and retries were unavailable. Stage and nested check durations
overlap, so do not sum them into an end-to-end duration. A later deterministic
fixture can demonstrate accounting and stage reuse; it cannot measure model
token savings or native performance.

[MEASUREMENTS.md](MEASUREMENTS.md) records the historical retrospective source:
53 sessions, 50 with usage, 25,364,703 cumulative input tokens including
23,041,536 cached and 2,323,167 fresh input, with 272,880 output tokens. Its
44 issue stages comprised 21 implementation and 23 review stages with 106.51
captured agent minutes; 26 native reports held 466 checks, 19 reports passing and
seven failing. This excludes native/build time, manual recovery and invocation
gaps. Keep historical raw records in the human-review bundle with their hashes.

The historical task mix and cumulative session counters differ from scripted
fixtures, final native checks and the SDK's last-message usage snapshots. Cached
input is included in total input, and reasoning is a subset of output. Missing
measurements remain unavailable. Compare observations with these scope and host
differences stated; infer no savings percentage, cost, peak context or unattended
provider reliability. Initial models, effort and the existing 12-hour bound stay
unchanged.

Retain all meaningful failures and interventions alongside subsequent passes;
distinguish deliberate TDD red cases, host-load timeouts, discovery drift and
actual defects. Name each historical candidate instead of combining partial
native runs into one final pass. Commit only explicitly historical summaries or
stable procedures after a run; current outcomes stay in host reports.

No remote publication is performed by this procedure. Keep local evidence and
branches reviewable for human PR/publication in their owning repositories and
state remote delivery as pending. The original runner remains paused, queue
inspection remains read-only, and no automatic enqueue or promotion into the
original checkouts is authorized.
