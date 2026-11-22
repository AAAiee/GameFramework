# Layering proof for the baseline UnitTests suite on an MSBuild host.
#
# Step 1 (engine coverage): patch SceneCamera::recalculateMatrix to always
# write the identity matrix, rebuild and run UnitTests, expect a NONZERO exit,
# restore. A suite that stays green here does not actually cover the engine.
#
# Step 2 (editor independence): patch EditorLayer::saveSceneTo to skip the
# serialize call, rebuild and run UnitTests, expect a ZERO exit, restore.
# The engine-level suite must not depend on editor code or the editor save
# path. SceneSerializer itself is never patched.
#
# Every patched file is restored with git checkout --, including on failure,
# and the script fails if the tree is ever left dirty.
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Debug'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
Set-Location $repository

$evidence = Join-Path $repository 'Binaries/layering-proof'
New-Item -ItemType Directory -Force -Path $evidence | Out-Null

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

function Build-UnitTests([string] $Label) {
    $project = Join-Path $repository 'Tests\UnitTests.vcxproj'
    & $msbuild $project /t:Rebuild /m /nr:false "/p:Configuration=$Configuration" /p:Platform=x64 /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) { throw "UnitTests $Label build failed with exit $LASTEXITCODE" }
}

function Invoke-UnitTests([string] $Label) {
    $exe = Join-Path $repository "Binaries\windows-x86_64\$Configuration\UnitTests\UnitTests.exe"
    if (-not (Test-Path $exe)) { throw "UnitTests exe missing at $exe" }
    $stdout = Join-Path $evidence "$Label.stdout.log"
    $stderr = Join-Path $evidence "$Label.stderr.log"
    $process = Start-Process -FilePath $exe -ArgumentList '--no-colors', '--no-breaks' `
        -WorkingDirectory (Split-Path $exe -Parent) -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr -PassThru -NoNewWindow
    if (-not $process.WaitForExit(120000)) {
        & taskkill.exe /PID $process.Id /T /F | Out-Null
        throw "UnitTests $Label timed out"
    }
    $process.WaitForExit()
    return $process.ExitCode
}

function Assert-SingleOccurrence([string] $Text, [string] $Needle, [string] $Path) {
    $count = ([regex]::Matches($Text, [regex]::Escape($Needle))).Count
    if ($count -ne 1) {
        throw "expected exactly one occurrence of the patch needle in $Path, found $count"
    }
}

$cameraPath = 'Engine/Source/Rubber/Scene/SceneCamera.cpp'
$editorPath = 'Editor/src/EditorLayer.cpp'
$identity = 'm_ProjectionMatrix = glm::mat4(1.0f);'

try {
    # ---- Step 1: engine behavior must be covered -----------------------------
    $cameraOriginal = Get-Content -Raw $cameraPath
    $orthoNeedle = 'm_ProjectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_OrthoNear, m_OrthoFar);'
    $perspNeedle = 'm_ProjectionMatrix = glm::perspective(m_PersFovY, m_AspectRatio, m_PersNear, m_PersFar);'
    Assert-SingleOccurrence $cameraOriginal $orthoNeedle $cameraPath
    Assert-SingleOccurrence $cameraOriginal $perspNeedle $cameraPath

    $cameraPatched = $cameraOriginal.Replace($orthoNeedle, $identity).Replace($perspNeedle, $identity)
    Set-Content -LiteralPath $cameraPath -Value $cameraPatched -NoNewline

    Build-UnitTests 'patched-camera'
    $cameraExit = Invoke-UnitTests 'patched-camera-run'
    if ($cameraExit -eq 0) {
        throw 'Step 1 failed: UnitTests stayed green with identity camera math; engine coverage is not proven'
    }
    Write-Host "LAYERING_STEP1_PASS identity camera math -> UnitTests exit $cameraExit (nonzero as expected)"

    # ---- Step 2: the editor layer must not be load-bearing ------------------
    $editorOriginal = Get-Content -Raw $editorPath
    $saveNeedle = 'serializer.serialize(filepath);'
    Assert-SingleOccurrence $editorOriginal $saveNeedle $editorPath

    $editorPatched = $editorOriginal.Replace($saveNeedle, '(void)filepath;')
    Set-Content -LiteralPath $editorPath -Value $editorPatched -NoNewline

    Build-UnitTests 'patched-editor-save'
    $editorExit = Invoke-UnitTests 'patched-editor-save-run'
    if ($editorExit -ne 0) {
        throw "Step 2 failed: UnitTests exited $editorExit with the editor save path skipped; UnitTests must not depend on the editor layer"
    }
    Write-Host "LAYERING_STEP2_PASS skipped editor save -> UnitTests exit $editorExit (zero as expected)"
}
finally {
    git checkout -- $cameraPath
    git checkout -- $editorPath
}

$dirty = git status --porcelain
if ($dirty) { throw "worktree left dirty after layering proof: $dirty" }
Write-Host "LAYERING_PROOF_PASS $Configuration"
