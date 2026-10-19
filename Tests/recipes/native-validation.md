# Native diagnostics and final acceptance

Run from the repository root. Use Windows, PowerShell 7, VS2022 C++ x64 tools
and installed Windows SDK headers, initialized local pinned dependency sources,
bundled Premake and a real OpenGL 4.5 compatible desktop/driver. Discover
`pwsh.exe` (for example `(Get-Command pwsh.exe).Source`) and the absolute MSBuild
path on the host. Containers used to build an npm artifact do not identify the
native acceptance environment.

Prefer `runEditorTask` with `createEditorNativeValidation` for supervised runs
and prepared-resource restoration. The direct PowerShell commands below are the
driver API for a completed foreground run; they alone do not provide the
adapter's whole-tree ownership, cancellation or safe restoration guarantee.

## Prepare and reuse resources

The existing adapter export
[`createEditorNativeValidation`](../../.sandcastle/native.mjs) returns
`prepareValidation`, `diagnose`, `validate` and `evidence` options for
[`runEditorTask`](../../.sandcastle/task.mjs). Its configuration is explicit:

```js
import { createEditorNativeValidation } from "./.sandcastle/native.mjs";

// source, localDependencies, msbuild, powershell and reportsDirectory are
// absolute host paths/configuration supplied by the caller.
// localDependencies maps each candidate gitlink path to a clean, initialized
// local repository containing that pin. reviewPolicy freshly observes policy.
const native = createEditorNativeValidation({
  source, localDependencies, msbuild, powershell, reportsDirectory,
  issue: 21, reviewPolicy,
});
// Supply ...native alongside the other required runEditorTask options.
// This sketch does not launch a task or discover/enqueue queue work.
```

The task acquires its runtime lock before resource preparation and environment/
dependency discovery. Preparation creates a detached parent and independent
local-only dependency clones inside the run resource directory. It disables lazy
fetch and non-file transports; a missing local pin fails as a prerequisite
without downloads. Do not reset or reuse a preserved external checkout directly.

Premake regenerates for the new path. Preparation does not copy old `.git`
pointers, solution/PCH/tlogs or absolute-path outputs. The first build may be
cold. Both phases and compatible resume reuse the same run-owned worktree and
outputs. `withPreparedWorktree` protects staged, unstaged and untracked edits
and ignored caches. Actual prepared dependency pins are checked against the
candidate before discovery and execution; changed pins need a new compatible
resource. Preserve occupied or partially created/restored resources for
reconciliation; do not delete receipts or unlock to force reuse.

## Diagnose first

For an already prepared clean worktree, set absolute paths in `$prepared`,
`$msbuild` and `$diagnosticReport`, then:

```powershell
pwsh.exe -NoProfile -File .\Scripts\Validate-Windows.ps1 -Worktree $prepared `
  -MSBuild $msbuild -Phase diagnose -BuildTarget Build `
  -ReportPath $diagnosticReport
if ($LASTEXITCODE -ne 0) { throw 'Diagnostics failed; inspect the host report' }
```

Without `-PlanPath` this runs the complete Debug diagnostic plan: incremental
Build, full Regression, standalone selectors and configured Windows scripts.
A known coverage mapping can select focused diagnostics. Generate JSON through
the existing API from the repository root, setting an absolute ignored host-log
path in `EDITOR_NATIVE_PLAN`, then supply it with `-PlanPath`:

```powershell
$env:EDITOR_NATIVE_PLAN = $diagnosticPlan
node --input-type=module -e 'import { writeFileSync } from "node:fs"; import { selectNativePlan } from "./.sandcastle/native.mjs"; writeFileSync(process.env.EDITOR_NATIVE_PLAN, JSON.stringify(selectNativePlan({phase:"diagnose",issue:21}),null,2));'
if ($LASTEXITCODE -ne 0) { throw 'Plan generation failed' }
# Add -PlanPath $diagnosticPlan to the diagnostic driver command above.
```

Issue numbers configure coverage; they never select queue membership. #21/#22
include both `font-atlas` and `atlas-placeholders`; #23/#24 include both
`gl-boundaries` and `gl-buffers`. #12 scene hooks and unmapped numbers use
complete Debug diagnostics. Inspect [the mapping](../../.sandcastle/native.mjs)
for other supported issues. Do not use `-Selector` as a narrowing switch; the
complete defaults or supplied plan own selector coverage.

## Independent final gate

```powershell
pwsh.exe -NoProfile -File .\Scripts\Validate-Windows.ps1 -Worktree $prepared `
  -MSBuild $msbuild -Phase validate -BuildTarget Rebuild `
  -ReportPath $finalReport
if ($LASTEXITCODE -ne 0) { throw 'Final native acceptance failed; inspect the host report' }
```

Final acceptance regenerates projects, performs Debug **and** Release Rebuilds,
each full Regression run, all 13 standalone selectors, all five UI wrappers and
the tracked UnitTests acceptance script in each configuration. The existing plan
has 43 checks including generation and the two UnitTests invocations. Final
plans generated with `selectNativePlan({phase:"validate",issue})` cannot subtract
this mandatory baseline. Mismatched phase/build/configuration plans are rejected.
A successful diagnostic report cannot satisfy the independent final report gate.

## Private desktop and process ownership

Regression creates and attaches its private Windows desktop before GLFW or
Application initialization, never switches the input desktop, and fails closed.
Real production dialogs are automated there. See [Editor UI](editor-ui.md);
do not click fixture dialogs manually or rerun on the input desktop.

The native adapter uses the fork's explicit owned-process boundary on Windows
10+ with PowerShell 7: suspended launch in a non-breakaway JobObject, durable
ownership receipts before execution, and `completion: "primary"`. The driver
awaits meaningful checks; after its exit remaining owned tool services terminate
and zero active owned processes is required before restoration. MSBuild node
reuse is disabled; each driver/focused UnitTests invocation has a distinct
PDB-server endpoint, preventing delegation to a preexisting shared tool process.
Reference contracts: [MSBuild node reuse](https://learn.microsoft.com/en-us/visualstudio/msbuild/msbuild-command-line-reference)
and [Microsoft BuildXL MSVC endpoint isolation](https://github.com/microsoft/BuildXL/blob/main/Public/Sdk/Experimental/Msvc/Native/Tools/Link/Link.dsc).

Confirmed cancellation stops owned processes before restoration. Unknown process
ownership preserves the lock, receipts and prepared state. Use the boundary's
read-only `reconcile()` and owner-matched runtime recovery; see the fork's
`docs/workflow-process.md` and [Editor recovery](../../.sandcastle/RECOVERY.md).
Generic supervision/recovery policy remains fork-owned.

## Read outcomes and retain evidence

The helper writes
`<reportsDirectory>/<phase>-<candidate>-<timestamp>/` containing `plan.json`,
`results.json`, `results.md`, `driver.log`, per-command logs and process
receipts. The driver without `-ReportPath` defaults to
`<prepared>/Binaries/Validation/<timestamp>/`. Choose a unique ignored host-log
directory for each explicit report path.

Host reports bind candidate, phase, build target/configurations, outcome,
exit/timeout/marker and actual per-check `durationMs`. The adapter retains
`nativeTimings` separately from outer-stage timing. UnitTests retains nested
logs/JUnit under each configuration's `UnitTests/issue-29/`. Preserve all raw
evidence and its paths. [Evidence identity](../../.sandcastle/EVIDENCE.md)
owns reuse and delivery gates; [context packets](../../.sandcastle/CONTEXT-PACKETS.md)
carry concise summaries and exact raw pointers.

Missing tools/dependencies are prerequisites. Explicit command/assertion/marker
failures provide substantive repair evidence. Missing, inconsistent or uncertain
process/report outcomes stop for inspection; do not invent a pass or downgrade
desktop isolation. `Inspect-NativeEnvironment.ps1` observes actual compiler,
MSBuild, SDK headers, OS, graphics driver and Premake identities.

For driver parameters see [Scripts/Validate-Windows.md](../../Scripts/Validate-Windows.md).
Original attempts remain in [HISTORY.md](../HISTORY.md); historical passes do not
authorize the current candidate.
