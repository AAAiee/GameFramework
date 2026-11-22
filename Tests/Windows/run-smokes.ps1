# Builds and runs EngineSmoke and EditorSmoke for one configuration.
# Needs a display host with a working WGL/OpenGL driver; hosted windows-latest
# has neither, so run this only on a real display host. Fails on any nonzero
# build or run exit code.
param(
    [ValidateSet('Debug', 'Release', 'Dist')]
    [string] $Configuration = 'Debug'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
Set-Location $repository

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

foreach ($project in @('EngineSmoke', 'EditorSmoke')) {
    $projectFile = Join-Path $repository "Tests\$project.vcxproj"
    & $msbuild $projectFile /t:Rebuild /m /nr:false "/p:Configuration=$Configuration" /p:Platform=x64 /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) { throw "$project $Configuration build failed with exit $LASTEXITCODE" }

    $exeDir = Join-Path $repository "Binaries\windows-x86_64\$Configuration\$project"
    $exe = Join-Path $exeDir "$project.exe"
    if (-not (Test-Path $exe)) { throw "$project exe missing at $exe" }

    # Run from the exe folder: the post-build step copies Editor/assets there,
    # and the engine's shader load expects them relative to the working dir.
    Push-Location $exeDir
    try {
        & $exe
        $exit = $LASTEXITCODE
    } finally {
        Pop-Location
    }
    if ($exit -ne 0) { throw "$project $Configuration failed with exit $exit" }
    Write-Host "SMOKE_PASS $project $Configuration"
}

Write-Host "SMOKES_PASS $Configuration"
