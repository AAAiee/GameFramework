# Windows native checks

PowerShell 7, Visual Studio 2022 C++ x64 tools, initialized local dependency
sources, bundled Premake and a working graphics driver are required. Native UI
fixtures create their private desktop before application initialization and fail
closed. Do not interact with their dialogs on the input desktop.

For an already prepared clean worktree:

```powershell
pwsh -NoProfile -File Scripts/Validate-Windows.ps1 -Worktree <prepared> `
  -Phase diagnose -BuildTarget Build -PlanPath <diagnostic-plan.json> `
  -ReportPath <diagnostic-results.json>
pwsh -NoProfile -File Scripts/Validate-Windows.ps1 -Worktree <prepared> `
  -Phase validate -BuildTarget Rebuild -PlanPath <final-plan.json> `
  -ReportPath <final-results.json>
```

Generate plans with `.sandcastle/native.mjs`'s `selectNativePlan({phase, issue})`.
Known coverage includes dependent fixtures (font atlas/placeholders and GL
boundaries/buffers). Changes to scene hooks use the complete diagnostic plan.
An unmapped issue also uses the complete Debug diagnostic plan. These mappings
select diagnostic coverage only; they never select queue issues.

Diagnostics use incremental Debug Build and configured standalone/UI checks.
Final validation always uses Debug and Release Rebuild, full Regression, all 13
standalone selectors, all five Windows UI scripts, and tracked UnitTests
normal/filtered/deliberate-failure-probe acceptance. The driver itself preserves
these mandatory final checks even when a supplied plan omits them; it rejects a
diagnostic/Build-only plan in final mode. A diagnostic report cannot satisfy the
adapter's independent final report contract.

Each command has a bounded timeout, exact success marker and separate log.
`results.json` owns the current candidate, phase, configurations, check outcomes,
actual per-check `durationMs`, and evidence locations. These host timings stay in
`nativeTimings` beside the adapter's separate outer-stage measurements. Missing
tools/dependencies are permanent prerequisites; explicit command/test failure is
substantive repair; missing or inconsistent process/report outcomes stop for
inspection. No missing prerequisite triggers downloads or weaker UI isolation.

`.sandcastle/native.mjs`'s `createEditorNativeValidation` supplies explicit
`prepareValidation`, `diagnose`, `validate`, and `evidence` options to
`runEditorTask`. Pass `source`, initialized `localDependencies` keyed by gitlink
path, absolute `msbuild`/`powershell` paths, `reportsDirectory`, optional issue
coverage number, and a fresh `reviewPolicy` callback. The task adapter acquires
its runtime lock before resources, prepares before dependency/environment
discovery, and shares the same run-owned worktree between both phases.

Preparation creates a detached worktree and independent local submodule clones
inside the runtime resource directory. Git permits file transport only and
disables lazy fetch. It never copies old `.git` pointers or absolute-path build
outputs. First build in a new path may be cold; subsequent phases reuse its
outputs. Occupied/partially created resources require explicit reconciliation.
Prepared source edits and ignored caches remain protected by
`withPreparedWorktree`. Candidate pins are compared with actual prepared pins
before evidence discovery and execution. A later changed pin stops as a
prerequisite rather than claiming expected pins were observed; prepare a new
compatible run locally. Native evidence additionally observes current compiler,
MSBuild, installed SDK headers, OS, graphics driver and Premake identities using
`Inspect-NativeEnvironment.ps1`; a package-builder image cannot stand in for this
host discovery.
