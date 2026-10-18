param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration,
    [Parameter(Mandatory = $true)]
    [string] $BinaryRoot,
    [Parameter(Mandatory = $true)]
    [string] $MSBuild
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if (-not [IO.Path]::IsPathRooted($BinaryRoot) -or
    -not [IO.Path]::IsPathRooted($MSBuild)) {
    throw 'BinaryRoot and MSBuild must be absolute paths'
}
$expectedRoot = Join-Path $repository "Binaries/windows-x86_64/$Configuration"
if ([IO.Path]::GetFullPath($BinaryRoot).TrimEnd('\', '/') -ine
    [IO.Path]::GetFullPath($expectedRoot).TrimEnd('\', '/')) {
    throw 'BinaryRoot must name this prepared worktree configuration output directory'
}
$project = Join-Path $repository 'Tests/UnitTests.vcxproj'
if (-not (Test-Path -LiteralPath $project) -or -not (Test-Path -LiteralPath $MSBuild)) {
    throw 'Generate VS2022 projects first and supply the installed MSBuild.exe path'
}
$outputDirectory = Join-Path $BinaryRoot 'UnitTests'
$null = New-Item -ItemType Directory -Force -Path $outputDirectory
$evidence = Join-Path $outputDirectory 'issue-29'
$null = New-Item -ItemType Directory -Force -Path $evidence
$summary = Join-Path $evidence 'acceptance.log'
# Replace prior evidence so a failed retry cannot inherit a success marker.
Set-Content -LiteralPath $summary -Value "Configuration: $Configuration; UTC: $([DateTime]::UtcNow.ToString('o'))"
$commit = & git -C $repository rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot identify candidate commit' }
Add-Content -LiteralPath $summary -Value "Candidate: $commit; MSBuild: $MSBuild"

function Invoke-Recorded([string] $Name, [string] $Executable, [string] $Arguments, [int] $Timeout) {
    $stdout = Join-Path $evidence "$Name.stdout.log"
    $stderr = Join-Path $evidence "$Name.stderr.log"
    $process = $null
    try {
        $process = Start-Process -WindowStyle Hidden -FilePath $Executable -ArgumentList $Arguments `
            -WorkingDirectory $repository -RedirectStandardOutput $stdout `
            -RedirectStandardError $stderr -PassThru
        if (-not $process.WaitForExit($Timeout)) { throw "$Name timed out after $Timeout ms" }
        $process.WaitForExit()
        $code = $process.ExitCode
        Add-Content -LiteralPath $summary -Value "$Name exit=$code; arguments=$Arguments"
        return $code
    } finally {
        if ($null -ne $process) {
            if (-not $process.HasExited) {
                # Stop only this invocation and its MSBuild child processes.
                & taskkill.exe /PID $process.Id /T /F | Out-Null
                if (-not $process.WaitForExit(5000)) {
                    throw "Could not stop $Name after timeout"
                }
            }
            $process.Dispose()
        }
    }
}

function Build-Target([string] $Name, [string] $ProjectPath, [string] $Extra = '') {
    $arguments = '"{0}" /t:Rebuild /m:1 /nr:false /p:Configuration={1} /p:Platform=x64 /v:minimal /nologo {2}' -f $ProjectPath, $Configuration, $Extra
    if ((Invoke-Recorded $Name $MSBuild $arguments 180000) -ne 0) {
        throw "$Name failed; see $evidence"
    }
}

function Assert-JUnit([string] $ReportPath, [int] $Failures) {
    [xml]$report = Get-Content -LiteralPath $ReportPath -Raw
    $suites = @($report.testsuites.testsuite)
    # doctest v2.4.12 reports assertion count in tests, not testcase count.
    $expectedAssertions = [string](1 + $Failures)
    if ($suites.Count -ne 1 -or $suites[0].tests -ne $expectedAssertions -or
        $suites[0].failures -ne [string]$Failures -or $suites[0].errors -ne '0' -or
        @($suites[0].testcase).Count -ne 1 -or $suites[0].testcase.name -ne 'camera-inverse') {
        throw "Unexpected JUnit case/failure counts in $ReportPath"
    }
}

$exe = Join-Path $outputDirectory 'UnitTests.exe'
function Run-Passing([string] $Phase) {
    if ((Invoke-Recorded "$Phase-all" $exe '--no-colors --no-breaks' 30000) -ne 0) {
        throw "$Phase console run failed"
    }
    $reportPath = Join-Path $evidence "$Phase.xml"
    Remove-Item -LiteralPath $reportPath -Force -ErrorAction SilentlyContinue
    $arguments = '--test-case=camera-inverse --reporters=junit --out="{0}" --no-breaks' -f $reportPath
    if ((Invoke-Recorded "$Phase-filter" $exe $arguments 30000) -ne 0) {
        throw "$Phase filtered run failed"
    }
    Assert-JUnit $reportPath 0
}

Build-Target 'build-before' $project
Run-Passing 'before'

# Compile a disposable copy using the generated project's exact settings. Keep it
# beside the original project so all relative include/import/source paths agree.
# Override outputs to isolate the failing binary; never edit the tracked runner.
$token = [Guid]::NewGuid().ToString('N')
$probeProject = Join-Path $repository "Tests/UnitTests-probe-$token.vcxproj"
# Keep disposable build outputs near the prepared root. Deep run-owned worktrees
# otherwise push MSVC FileTracker's Release link-cvtres tlogs beyond normal paths.
$probeDirectory = [IO.Path]::GetFullPath((Join-Path $repository "Binaries/p-$token"))
$ownedBinaries = [IO.Path]::GetFullPath((Join-Path $repository 'Binaries'))
if (-not $probeDirectory.StartsWith($ownedBinaries + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -or
    [IO.Path]::GetFileName($probeDirectory) -ne "p-$token") {
    throw 'Failure probe output must remain inside this prepared worktree Binaries directory'
}
$probeParent = Get-Item -LiteralPath $ownedBinaries
while ($probeParent.FullName -ine $repository) {
    if ($probeParent.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Failure probe directory ancestry must not redirect outside its prepared worktree' }
    $probeParent = $probeParent.Parent
    if ($null -eq $probeParent) { throw 'Failure probe directory is outside its prepared worktree' }
}
$null = New-Item -ItemType Directory -Path $probeDirectory
try {
    $runner = Join-Path $repository 'Tests/unit/UnitTests.cpp'
    $originalHash = (Get-FileHash -LiteralPath $runner -Algorithm SHA256).Hash
    $source = [IO.File]::ReadAllText($runner)
    $anchor = 'CHECK_NOTHROW(Rubber::checkCameraInverse());'
    if ([regex]::Matches($source, [regex]::Escape($anchor)).Count -ne 1) {
        throw 'Expected exactly one camera assertion in runner'
    }
    $probeSource = Join-Path $probeDirectory 'UnitTestsFailureProbe.cpp'
    [IO.File]::WriteAllText($probeSource, $source.Replace($anchor, "$anchor`n    CHECK(false);"))
    [xml]$probe = Get-Content -LiteralPath $project -Raw
    $items = @($probe.SelectNodes("//*[local-name()='ClCompile' and @Include]") |
        Where-Object { $_.Include.Replace('\', '/') -eq 'unit/UnitTests.cpp' })
    if ($items.Count -ne 1) { throw 'Generated project must compile the runner exactly once' }
    $items[0].SetAttribute('Include', $probeSource)
    $probe.Save($probeProject)
    $extra = '/p:OutDir="{0}/" /p:IntDir="{0}/obj/" /p:TargetName=UnitTestsFailureProbe' -f $probeDirectory
    Build-Target 'build-failure-probe' $probeProject $extra
    $probeExe = Join-Path $probeDirectory 'UnitTestsFailureProbe.exe'
    $arguments = '--test-case=camera-inverse --no-colors --no-breaks'
    $code = Invoke-Recorded 'failure-probe' $probeExe $arguments 30000
    $failureText = Get-Content -LiteralPath (Join-Path $evidence 'failure-probe.stdout.log') -Raw
    if ($code -ne 1 -or $failureText -notmatch 'CHECK\( false \) is NOT correct!') {
        throw 'Probe must exit 1 and report the deliberately failing CHECK(false)'
    }
    $reportPath = Join-Path $evidence 'failure-probe.xml'
    Remove-Item -LiteralPath $reportPath -Force -ErrorAction SilentlyContinue
    $arguments = '--test-case=camera-inverse --reporters=junit --out="{0}" --no-breaks' -f $reportPath
    if ((Invoke-Recorded 'failure-probe-junit' $probeExe $arguments 30000) -ne 1) {
        throw 'Failing JUnit run must exit 1'
    }
    Assert-JUnit $reportPath 1
    if ((Get-FileHash -LiteralPath $runner -Algorithm SHA256).Hash -ne $originalHash) {
        throw 'Tracked runner changed during validation'
    }
} finally {
    Remove-Item -LiteralPath $probeProject -Force -ErrorAction SilentlyContinue
    if ((Get-Item -LiteralPath $probeDirectory).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Failure probe directory changed ownership; preserve it for inspection' }
    Remove-Item -LiteralPath $probeDirectory -Recurse -Force
}

# Rebuild and rerun the checked-in runner after removing all failing probe code.
Build-Target 'build-after' $project
Run-Passing 'after'
$marker = "PASS: unit-tests-$Configuration"
Add-Content -LiteralPath $summary -Value $marker
Write-Output $marker
