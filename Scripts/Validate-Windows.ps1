#requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$Worktree,
    [string]$MSBuild,
    [ValidateSet('diagnose', 'validate')]
    [string]$Phase = 'validate',
    [ValidateSet('Build', 'Rebuild')]
    [string]$BuildTarget = 'Rebuild',
    [string]$Selector = 'scene-hooks',
    [string]$PlanPath,
    [string]$ReportPath
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $Worktree).Path
$previousProtocol = $env:GIT_ALLOW_PROTOCOL
$previousLazyFetch = $env:GIT_NO_LAZY_FETCH
$previousNodeReuse = $env:MSBUILDDISABLENODEREUSE
$previousPdbEndpoint = $env:_MSPDBSRV_ENDPOINT_
$env:GIT_ALLOW_PROTOCOL = 'file'
$env:GIT_NO_LAZY_FETCH = '1'
$env:MSBUILDDISABLENODEREUSE = '1'
# Prevent cl/link from delegating writes to another invocation's shared server.
$env:_MSPDBSRV_ENDPOINT_ = "native-$([Guid]::NewGuid().ToString('N'))"
$results = [System.Collections.Generic.List[string]]::new()
$report = $null
$checks = [System.Collections.Generic.List[object]]::new()
$validationStatus = 'failed'
$head = $null
$failureKind = 'prerequisite'
$failureMessage = $null

function Read-Git {
    param([string[]]$GitArguments)
    $output = & git -C $root @GitArguments 2>&1
    if ($LASTEXITCODE -ne 0) { throw "git $GitArguments failed: $output" }
    return $output
}

function Invoke-Check {
    param(
        [string]$Name,
        [string]$Executable,
        [string[]]$CommandArguments,
        [string]$Directory,
        [int]$TimeoutSeconds = 1800,
        [string]$ExpectedLine,
        [string]$ExpectedPattern
    )
    $log = Join-Path $logDirectory "$Name.log"
    $clock = [System.Diagnostics.Stopwatch]::StartNew()
    $info = [System.Diagnostics.ProcessStartInfo]::new()
    $info.FileName = $Executable
    $info.WorkingDirectory = $Directory
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    foreach ($argument in $CommandArguments) { $info.ArgumentList.Add($argument) }
    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $info
    Write-Host "Running $Name (log: $log)"
    try {
        [void]$process.Start()
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        $timedOut = -not $process.WaitForExit($TimeoutSeconds * 1000)
        if ($timedOut) {
            $process.Kill($true)
            $process.WaitForExit()
        }
        $output = $stdout.GetAwaiter().GetResult()
        $errors = $stderr.GetAwaiter().GetResult()
        @(
            "Executable: $Executable"
            "Arguments: $($CommandArguments | ConvertTo-Json -Compress)"
            "Working directory: $Directory"
            "Exit code: $($process.ExitCode)"
            "Timed out: $timedOut"
            $output
            $errors
        ) | Set-Content -LiteralPath $log -Encoding utf8
        $results.Add("- ${Name}: exit $($process.ExitCode); timeout=$timedOut; log=$log")
        $results.Add("  Executable: $Executable; arguments: $($CommandArguments | ConvertTo-Json -Compress); cwd: $Directory")
        $markerPresent = (-not $ExpectedLine -or $ExpectedLine -in ($output -split '\r?\n')) -and
            (-not $ExpectedPattern -or $output -match $ExpectedPattern)
        $passed = -not $timedOut -and $process.ExitCode -eq 0 -and $markerPresent
        $checks.Add(@{ name = $Name; exitCode = $process.ExitCode; timedOut = $timedOut; markerPresent = $markerPresent; log = $log; durationMs = $clock.Elapsed.TotalMilliseconds; passed = $passed })
        if (-not $passed) { $script:failureKind = 'substantive' }
        if ($timedOut) { throw "$Name exceeded $TimeoutSeconds seconds. See $log" }
        if ($process.ExitCode -ne 0) { throw "$Name failed. See $log" }
        if (-not $markerPresent) {
            throw "$Name did not print the required success marker. See $log"
        }
        Write-Host "PASS: $Name"
    }
    finally { $clock.Stop(); $process.Dispose() }
}

try {
    if ($ReportPath) {
        $ReportPath = [System.IO.Path]::GetFullPath($ReportPath)
        $logDirectory = Split-Path -Parent $ReportPath
    } else {
        $logDirectory = Join-Path $root "Binaries/Validation/$(Get-Date -Format 'yyyyMMdd-HHmmss-fff')"
        $ReportPath = Join-Path $logDirectory 'results.json'
    }
    [void](New-Item -ItemType Directory -Path $logDirectory -Force)
    $report = Join-Path $logDirectory 'results.md'
    $head = Read-Git @('rev-parse', 'HEAD')
    $dirty = Read-Git @('status', '--porcelain', '--untracked-files=no')
    if ($dirty) { throw "Validation requires unchanged tracked files in $root. Existing edits were preserved: $dirty" }
    $untracked = Read-Git @('ls-files', '--others', '--exclude-standard')
    if ($untracked) { throw "Untracked files could affect validation in $root. Existing files were preserved: $untracked" }
    $submodules = @(Read-Git @('submodule', 'status', '--recursive'))
    if ($submodules | Where-Object { "$_" -match '^[-+U]' }) {
        throw "Dependencies are missing or differ from the pinned commits. Reuse a prepared checkout or populate dependencies from local repositories first. No network fetch was attempted.`n$($submodules -join "`n")"
    }
    foreach ($required in @('Tests/Build-Regression.lua', 'Vendor/Binaries/Premake/Windows/premake5.exe')) {
        if (-not (Test-Path -LiteralPath (Join-Path $root $required))) { throw "Missing $required in $root" }
    }
    if (-not $MSBuild) {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
        if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Pass -MSBuild with the path to Visual Studio 2022 MSBuild.exe.' }
        $MSBuild = & $vswhere -latest -products '*' -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
        if (-not $MSBuild) { throw 'Visual Studio 2022 with the x64 C++ toolchain was not found. Pass -MSBuild explicitly.' }
    }
    $MSBuild = (Resolve-Path -LiteralPath $MSBuild).Path
    $allSelectors = @('scene-hooks', 'camera-inverse', 'renderer-batches', 'entity-tags', 'viewport-pixels', 'minimize-restore', 'layer-partition', 'typed-events', 'event-names', 'font-atlas', 'atlas-placeholders', 'gl-boundaries', 'gl-buffers')
    $allWindows = @(12, 15, 16, 17, 18 | ForEach-Object { @{ script = "Tests/Windows/issue-$_.ps1"; marker = "PASS: editor-issue-$_" } })
    $unit = @{ whenTracked = 'Tests/Build-UnitTests.lua'; script = 'Tests/Windows/unit-tests.ps1'; marker = 'PASS: unit-tests-{configuration}'; msbuild = $true; timeoutSeconds = 900 }
    $selectors = $allSelectors
    $windowsChecks = $allWindows
    $optional = @($unit)
    $fullSuite = $true
    $configurations = if ($Phase -eq 'diagnose') { @('Debug') } else { @('Debug', 'Release') }
    if (($Phase -eq 'validate' -and $BuildTarget -ne 'Rebuild') -or ($Phase -eq 'diagnose' -and $BuildTarget -ne 'Build')) {
        throw 'Final acceptance requires Rebuild; diagnostics require incremental Build.'
    }
    if ($PlanPath) {
        $plan = Get-Content -LiteralPath $PlanPath -Raw | ConvertFrom-Json
        if ($plan.phase -ne $Phase -or $plan.buildTarget -ne $BuildTarget -or
            (@($plan.configurations) -join ',') -ne ($configurations -join ',')) { throw 'Native plan phase/build/configurations do not match the required gates.' }
        if ($Phase -eq 'diagnose') {
            $selectors = @($plan.selectors)
            $windowsChecks = @($plan.windowsChecks)
            $optional = @($plan.optionalWindowsChecks)
            $fullSuite = $plan.fullSuite -eq $true
        } else {
            # Candidate plans may add coverage, never subtract required acceptance.
            $selectors = @($allSelectors + @($plan.selectors) | Select-Object -Unique)
            foreach ($check in @($plan.windowsChecks)) {
                $baseline = @($allWindows | Where-Object { $_.script -eq $check.script })
                if ($baseline.Count -gt 0) {
                    if ($check.marker -ne $baseline[0].marker) { throw "Cannot override required marker for $($check.script)" }
                } else { $windowsChecks += $check }
            }
        }
        # Feature suites follow the candidate's tracked build targets, regardless
        # of which GitHub issue introduced them. Missing suite scripts fail below.
    }
        foreach ($check in $optional) {
            if ($check -and $check.whenTracked) {
                $tracked = & git -C $root ls-files --error-unmatch -- $check.whenTracked 2>$null
                if ($LASTEXITCODE -eq 0 -and $tracked) { $windowsChecks += $check }
            }
        }
    foreach ($name in $selectors) {
        if ($name -notmatch '^[a-z0-9][a-z0-9-]*$') { throw "Invalid regression selector: $name" }
    }
    foreach ($check in $windowsChecks) {
        $script = [System.IO.Path]::GetFullPath((Join-Path $root $check.script))
        if (-not $script.StartsWith($root + [System.IO.Path]::DirectorySeparatorChar, [System.StringComparison]::OrdinalIgnoreCase) -or
            [System.IO.Path]::GetExtension($script) -ne '.ps1' -or -not (Test-Path -LiteralPath $script)) {
            throw "Missing or invalid Windows acceptance script: $($check.script)"
        }
        [void](Read-Git @('ls-files', '--error-unmatch', '--', $check.script))
        if (-not $check.marker) { throw "Missing success marker for $($check.script)" }
    }
    $results.Add("Commit: $head")
    $results.Add("Worktree: $root")
    $results.Add("MSBuild: $MSBuild")
    $results.Add("Build target: $BuildTarget")
    $results.Add("Dependency commits:`n$($submodules -join "`n")")
    $results.Add("Windows acceptance scripts: $($windowsChecks.Count). UI coverage is limited to these scripts.")
    Invoke-Check -Name 'generate' -Executable (Join-Path $root 'Vendor/Binaries/Premake/Windows/premake5.exe') -CommandArguments @('--file=Build.lua', 'vs2022') -Directory $root
    foreach ($configuration in $configurations) {
        Invoke-Check -Name "build-$configuration" -Executable $MSBuild -CommandArguments @('Rubber Engine.sln', "/t:$BuildTarget", '/m', '/nr:false', "/p:Configuration=$configuration", '/p:Platform=x64', '/v:minimal', '/nologo') -Directory $root
        $regressionDirectory = Join-Path $root "Binaries/windows-x86_64/$configuration/Regression"
        if ($fullSuite) {
            Invoke-Check -Name "regression-all-$configuration" -Executable (Join-Path $regressionDirectory 'Regression.exe') -CommandArguments @() -Directory $regressionDirectory -TimeoutSeconds 240 -ExpectedPattern '(?m)^PASS: '
        }
        foreach ($name in $selectors) {
            Invoke-Check -Name "regression-$name-$configuration" -Executable (Join-Path $regressionDirectory 'Regression.exe') -CommandArguments @($name) -Directory $regressionDirectory -TimeoutSeconds 120 -ExpectedLine "PASS: $name"
        }
        foreach ($check in $windowsChecks) {
            $script = Join-Path $root $check.script
            $binaryRoot = Join-Path $root "Binaries/windows-x86_64/$configuration"
            $arguments = @('-NoProfile', '-File', $script, '-Configuration', $configuration, '-BinaryRoot', $binaryRoot)
            if ($check.msbuild) { $arguments += @('-MSBuild', $MSBuild) }
            $timeout = if ($check.timeoutSeconds) { [int]$check.timeoutSeconds } else { 240 }
            $marker = $check.marker.Replace('{configuration}', $configuration)
            Invoke-Check -Name "windows-$([System.IO.Path]::GetFileNameWithoutExtension($script))-$configuration" -Executable (Join-Path $PSHOME 'pwsh.exe') -CommandArguments $arguments -Directory $root -TimeoutSeconds $timeout -ExpectedLine $marker
        }
    }
    $validationStatus = 'passed'
    $results.Add('Automated validation: PASS')
    Write-Host "PASS: $Phase native checks ($($configurations -join ', '))."
}
catch {
    $failureMessage = $_.Exception.Message
    $results.Add("Automated validation: FAILED — $($_.Exception.Message)")
    throw
}
finally {
    if ($report) {
        $results | Set-Content -LiteralPath $report -Encoding utf8
        @{ status = $validationStatus; phase = $Phase; buildTarget = $BuildTarget; configurations = @($configurations); commit = $head; checks = @($checks.ToArray()); report = $report; failureKind = $(if ($validationStatus -eq 'passed') { $null } else { $failureKind }); failure = $failureMessage } |
            ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $ReportPath -Encoding utf8
        Write-Host "Validation report: $report"
    }
    $env:GIT_ALLOW_PROTOCOL = $previousProtocol
    $env:GIT_NO_LAZY_FETCH = $previousLazyFetch
    $env:MSBUILDDISABLENODEREUSE = $previousNodeReuse
    $env:_MSPDBSRV_ENDPOINT_ = $previousPdbEndpoint
}
