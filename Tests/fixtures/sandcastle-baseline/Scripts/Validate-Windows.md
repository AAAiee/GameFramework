# Offline Windows validation

Run the host-side build and regression checks with PowerShell 7:

```powershell
pwsh -NoProfile -File .\Scripts\Validate-Windows.ps1 -Worktree ..\Editor-validate-12
```

The script reuses that existing checkout and its installed submodules. It does
not create a worktree, clone, fetch, initialize submodules, or install packages.
Git network transports and lazy object fetching are disabled for the script's
process. Missing or mismatched dependencies fail before building; a slow VPN
will not cause an implicit dependency download.

It checks for a clean checkout and pinned submodules, discovers Visual Studio
2022 MSBuild, generates the solution, rebuilds Debug and Release, and runs the
entire Regression suite plus `Regression.exe scene-hooks` from each executable's directory. Both focused runs must
print `PASS: scene-hooks` and exit 0. Each command has a timeout and a log.
Markdown and JSON results, the tested commit, dependency commits, and exit codes are saved under
the validation checkout's ignored `Binaries/Validation/<timestamp>/` directory.
The script stops on the first failure and returns a nonzero exit status.

Keep the validation worktree for subsequent work instead of recreating it per
issue. Git worktrees share the main repository's object database; submodule
working copies and build outputs are separate. When moving the validation
checkout to another already-local reviewed revision, check its cleanliness
first and retain its existing dependencies. If dependency pins change, prepare
those exact revisions from a local repository/cache; only obtain objects over
the network when they are genuinely unavailable locally.

For a repeat development check, `-BuildTarget Build` reuses incremental outputs.
Acceptance validation defaults to `Rebuild`. `-MSBuild <path>` overrides tool
discovery; `-Selector <name>` selects a regression check that prints `PASS: <name>`.

The continuous Sandcastle runner passes `-PlanPath <json>` containing `selectors`
and `windowsChecks` (each with a tracked `.ps1` script path and success `marker`).
It also supplies `-ReportPath <json>` to collect evidence in its persistent logs.
Every Windows script receives `-Configuration` and `-BinaryRoot` and must exit 0
and print its marker in both configurations. Missing scripts, selectors, or
markers fail acceptance. The report records only the UI scripts actually run;
the standalone default performs no UI checks. See
[the continuous workflow](../.sandcastle/WORKFLOW.md) for scheduling and recovery.
