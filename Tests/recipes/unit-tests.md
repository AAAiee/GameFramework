# Existing headless UnitTests acceptance

`UnitTests` is the existing C++20 console target using vendored doctest v2.4.12.
Its [runner](../unit/UnitTests.cpp) and shared
[camera inverse check](../src/CameraInverseCheck.cpp) use Camera/GLM headers;
they do not link Engine, OpenGL, windowing or Editor assets. Doctest main is
compiled once. Debug uses static `/MTd`, Release `/MT`; checks remain active
with `NDEBUG`, and `DOCTEST_CONFIG_DISABLE` is rejected.

Use the same prepared worktree, initialized dependencies and VS2022 installation.
The [native driver](native-validation.md) generates projects and schedules this
script in each configuration whenever `Tests/Build-UnitTests.lua` is tracked.
A solution build or Regression camera pass does not run this doctest acceptance.

For an isolated foreground check, set `$prepared` and `$msbuild` to absolute
paths. Generate once and run configurations sequentially, without concurrent
builds in the worktree:

```powershell
Push-Location $prepared
try {
    & '.\Vendor\Binaries\Premake\Windows\premake5.exe' --file=Build.lua vs2022
    if ($LASTEXITCODE -ne 0) { throw 'Premake failed' }
    foreach ($configuration in @('Debug', 'Release')) {
        $binaryRoot = Join-Path $prepared "Binaries/windows-x86_64/$configuration"
        pwsh.exe -NoProfile -File '.\Tests\Windows\unit-tests.ps1' `
          -Configuration $configuration -BinaryRoot $binaryRoot -MSBuild $msbuild
        if ($LASTEXITCODE -ne 0) { throw "$configuration UnitTests acceptance failed" }
    }
} finally { Pop-Location }
```

[`unit-tests.ps1`](../Windows/unit-tests.ps1) builds only UnitTests and owns:

1. Normal execution of all registered cases and a camera-filtered JUnit run.
2. A disposable runner/project with `CHECK(false);` after `CHECK_NOTHROW`.
   It must report the deliberate assertion, exit 1 and exactly one failed JUnit
   case. Crash, timeout, missing case or unexpected exit fails acceptance.
3. Probe cleanup, then normal rebuild and repetition of both passing runs.

It never edits tracked sources. Disposable probe outputs use a short unique
directory directly under this worktree's ignored `Binaries` root to avoid MSVC
tracker path limits; cleanup checks absolute owned ancestry. MSBuild node reuse
is disabled and each invocation uses its own PDB-server endpoint. These
standalone protections do not provide the adapter's whole-tree ownership or
edit-restoration guarantee.

All executions use `--no-breaks`; no `--no-exitcode` override is used.
Build timeouts are 180 seconds and run timeouts 30 seconds; the native driver
allows 900 seconds for the outer script. Keep the existing script's case-count
and JUnit assertions as authority: doctest can accept a filter matching no
cases. Do not infer a pass from exit zero alone.

For manual inspection after a normal build, the existing executable accepts:

```powershell
$exe = Join-Path $prepared 'Binaries/windows-x86_64/Debug/UnitTests/UnitTests.exe'
& $exe --list-test-cases
& $exe '--test-case=camera-inverse' --no-colors --no-breaks
# For a JUnit output, also pass --reporters=junit and --out=<absolute-log-path>.
```

Those inspection commands do not replace the acceptance script.
Retain each configuration's `UnitTests/issue-29/acceptance.log`, stdout/stderr
and JUnit XML. Only the complete script may emit `PASS: unit-tests-Debug` or
`PASS: unit-tests-Release`. A retry replaces stage evidence, so retain each
completed run with its candidate identity.

[The original #29 record](../HISTORY.md#headless-unittests-issue-29) and
[second-review host blocker](../../docs/validation/issue-29.md#second-review-host-execution-blocker)
are historical. This recipe adds no doctest dependency, target, CI foundation
or #27/#30/#31 scope.
