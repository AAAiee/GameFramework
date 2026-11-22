# Hosted-runner probe for the baseline test split.
# Records which targets compile and which selectors exit 0 without a GPU.
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('unit', 'regression', 'all')]
    [string] $Stage
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
Set-Location $repository

$evidence = Join-Path $repository 'Binaries/hosted-ci-probe'
New-Item -ItemType Directory -Force -Path $evidence | Out-Null
$report = Join-Path $evidence 'results.tsv'
if (-not (Test-Path $report)) {
    Set-Content -LiteralPath $report -Value "ts`tstage`tname`texit`tpass_marker`tnote"
}

function Write-Row([string] $Name, [int] $Code, [string] $Marker, [string] $Note) {
    $ts = [DateTime]::UtcNow.ToString('o')
    Add-Content -LiteralPath $report -Value "$ts`t$Stage`t$Name`t$Code`t$Marker`t$Note"
    Write-Host "PROBE $Name exit=$Code marker=$Marker note=$Note"
}

function Find-MSBuild {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $found = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
        if ($found) { return $found }
    }
    if (Get-Command msbuild -ErrorAction SilentlyContinue) {
        return (Get-Command msbuild).Source
    }
    throw 'MSBuild not found'
}

$msbuild = Find-MSBuild
Write-Host "MSBuild $msbuild"

$premake = Join-Path $repository 'Vendor\Binaries\Premake\Windows\premake5.exe'
& $premake --file=Build.lua vs2022
if ($LASTEXITCODE -ne 0) { throw "premake failed: $LASTEXITCODE" }

function Invoke-Logged([string] $Name, [string] $File, [string] $Arguments, [string] $WorkDir, [int] $TimeoutMs) {
    $stdout = Join-Path $evidence "$Name.stdout.log"
    $stderr = Join-Path $evidence "$Name.stderr.log"
    $process = Start-Process -FilePath $File -ArgumentList $Arguments `
        -WorkingDirectory $WorkDir -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr -PassThru -NoNewWindow
    if (-not $process.WaitForExit($TimeoutMs)) {
        & taskkill.exe /PID $process.Id /T /F | Out-Null
        throw "$Name timed out after $TimeoutMs ms"
    }
    $process.WaitForExit()
    return $process.ExitCode
}

function Show-LogTail([string] $Name) {
    foreach ($kind in @('stdout', 'stderr')) {
        $path = Join-Path $evidence "$Name.$kind.log"
        if (Test-Path -LiteralPath $path) {
            Write-Host "----- $Name $kind -----"
            Get-Content -LiteralPath $path -Tail 80
        }
    }
}

function Build-Project([string] $Name, [string] $Project, [string] $Configuration) {
    $arguments = '"{0}" /t:Rebuild /m /nr:false /p:Configuration={1} /p:Platform=x64 /v:minimal /nologo' -f $Project, $Configuration
    $code = Invoke-Logged $Name $msbuild $arguments $repository 600000
    Write-Row $Name $code '' "msbuild $Project $Configuration"
    if ($code -ne 0) {
        Show-LogTail $Name
        throw "$Name failed"
    }
}

if ($Stage -eq 'unit' -or $Stage -eq 'all') {
    foreach ($configuration in @('Debug', 'Release')) {
        Build-Project "unittests-build-$configuration" (Join-Path $repository 'Tests\UnitTests.vcxproj') $configuration
        $exe = Join-Path $repository "Binaries\windows-x86_64\$configuration\UnitTests\UnitTests.exe"
        $code = Invoke-Logged "unittests-run-$configuration" $exe '--no-colors --no-breaks' (Split-Path $exe -Parent) 60000
        Write-Row "unittests-run-$configuration" $code '' 'UnitTests.exe --no-colors --no-breaks'
        if ($code -ne 0) {
            Show-LogTail "unittests-run-$configuration"
            throw "UnitTests $configuration failed"
        }
    }
}

if ($Stage -eq 'regression' -or $Stage -eq 'all') {
    Build-Project 'regression-build-Debug' (Join-Path $repository 'Tests\Regression.vcxproj') 'Debug'
    $regDir = Join-Path $repository 'Binaries\windows-x86_64\Debug\Regression'
    $regExe = Join-Path $regDir 'Regression.exe'
    $selectors = @(
        'camera-inverse',
        'layer-partition',
        'typed-events',
        'event-names',
        'font-atlas',
        'gl-buffers'
    )
    foreach ($selector in $selectors) {
        $code = 0
        $marker = ''
        $note = $selector
        try {
            $code = Invoke-Logged "regression-$selector" $regExe $selector $regDir 120000
            $stdout = Get-Content -LiteralPath (Join-Path $evidence "regression-$selector.stdout.log") -Raw -ErrorAction SilentlyContinue
            if ($stdout -match "PASS: $selector") { $marker = "PASS: $selector" }
            $stderr = Get-Content -LiteralPath (Join-Path $evidence "regression-$selector.stderr.log") -Raw -ErrorAction SilentlyContinue
            if ($stderr) { $note = ($stderr -split "`n")[0].Trim() }
        } catch {
            $code = -1
            $note = $_.Exception.Message
        }
        Write-Row "regression-$selector" $code $marker $note
    }
}

Write-Host "PROBE_REPORT $report"
Get-Content -LiteralPath $report
