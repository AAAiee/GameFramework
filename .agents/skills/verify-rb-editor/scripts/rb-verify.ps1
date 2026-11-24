# RB Editor verification helper. Invocation is documented in ../SKILL.md.
[CmdletBinding()]
param(
    [Parameter(Position = 0, Mandatory = $true)]
    [ValidateSet('build', 'doctor', 'launch-editor', 'drive', 'capture', 'cleanup')]
    [string] $Command,

    [Parameter(Position = 1)]
    [string] $Feature = '',

    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Debug',

    [string] $RunId = '',

    [string] $RepoRoot = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Get-RepoRoot {
    param([string] $Hint)
    if ($Hint) {
        $resolved = [IO.Path]::GetFullPath($Hint)
        if (Test-Path -LiteralPath (Join-Path $resolved 'Build.lua')) { return $resolved }
        throw "RepoRoot does not contain Build.lua: $resolved"
    }
    $cursor = $PSScriptRoot
    while ($cursor) {
        $build = Join-Path $cursor 'Build.lua'
        $editor = Join-Path $cursor 'Editor\src\Editor.cpp'
        if ((Test-Path -LiteralPath $build) -and (Test-Path -LiteralPath $editor)) {
            return $cursor
        }
        $parent = Split-Path $cursor -Parent
        if ($parent -eq $cursor) { break }
        $cursor = $parent
    }
    throw 'Could not find the GameFramework repo root (Build.lua + Editor/src/Editor.cpp)'
}

function Get-MSBuild {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $vswhere) {
        $found = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' |
            Select-Object -First 1
        if ($found) { return $found }
    }
    $cmd = Get-Command msbuild -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    throw 'MSBuild not found. Install Visual Studio 2022 C++ Build Tools.'
}

function Get-Head {
    param([string] $Repo)
    Push-Location $Repo
    try { return (git rev-parse HEAD).Trim() }
    finally { Pop-Location }
}

function Get-BinaryRoot {
    param([string] $Repo, [string] $Configuration)
    return Join-Path $Repo "Binaries\windows-x86_64\$Configuration"
}

function Get-EditorExe {
    param([string] $Repo, [string] $Configuration)
    return Join-Path (Get-BinaryRoot $Repo $Configuration) 'Editor\Editor.exe'
}

function Get-RegressionExe {
    param([string] $Repo, [string] $Configuration)
    return Join-Path (Get-BinaryRoot $Repo $Configuration) 'Regression\Regression.exe'
}

function Assert-EditorAssets {
    param([string] $WorkDir, [string] $Label)
    $shader = Join-Path $WorkDir 'assets\shader\CommonShader.glsl'
    $font = Join-Path $WorkDir 'assets\fonts\OpenSans\static\OpenSans-Regular.ttf'
    if (-not (Test-Path -LiteralPath $shader)) { throw "$Label missing shader: $shader" }
    if (-not (Test-Path -LiteralPath $font)) { throw "$Label missing font: $font" }
}

$script:FeatureMap = @{
    'scene-hierarchy'  = @{ Arg = 'editor-issue-16'; Marker = 'PASS: editor-issue-16'; TimeoutMs = 60000 }
    'inspector'        = @{ Arg = 'editor-issue-16'; Marker = 'PASS: editor-issue-16'; TimeoutMs = 60000 }
    'entity-tags'      = @{ Arg = 'editor-issue-16'; Marker = 'PASS: editor-issue-16'; TimeoutMs = 60000 }
    'editor-issue-16'  = @{ Arg = 'editor-issue-16'; Marker = 'PASS: editor-issue-16'; TimeoutMs = 60000 }
    'scene-files'      = @{ Arg = 'editor-issue-15'; Marker = 'PASS: editor-issue-15'; TimeoutMs = 240000 }
    'editor-issue-15'  = @{ Arg = 'editor-issue-15'; Marker = 'PASS: editor-issue-15'; TimeoutMs = 240000 }
    'viewport'         = @{ Arg = 'editor-issue-17'; Marker = 'PASS: editor-issue-17'; TimeoutMs = 240000 }
    'viewport-pixels'  = @{ Arg = 'editor-issue-17'; Marker = 'PASS: editor-issue-17'; TimeoutMs = 240000 }
    'editor-issue-17'  = @{ Arg = 'editor-issue-17'; Marker = 'PASS: editor-issue-17'; TimeoutMs = 240000 }
    'window-lifecycle' = @{ Arg = 'editor-issue-18'; Marker = 'PASS: editor-issue-18'; TimeoutMs = 60000 }
    'minimize-restore' = @{ Arg = 'editor-issue-18'; Marker = 'PASS: editor-issue-18'; TimeoutMs = 60000 }
    'editor-issue-18'  = @{ Arg = 'editor-issue-18'; Marker = 'PASS: editor-issue-18'; TimeoutMs = 60000 }
    'editor-issue-12'  = @{ Arg = 'editor-issue-12'; Marker = 'PASS: editor-issue-12'; TimeoutMs = 60000 }
    'editor-ui'        = @{ Arg = 'editor-ui'; Marker = 'PASS: editor-ui'; TimeoutMs = 120000 }
    'editor-ui-escape' = @{ Arg = 'editor-ui-escape'; Marker = 'PASS: editor-ui-escape'; TimeoutMs = 60000 }
    'scene-hooks'      = @{ Arg = 'scene-hooks'; Marker = 'PASS: scene-hooks'; TimeoutMs = 60000 }
    'camera-inverse'   = @{ Arg = 'camera-inverse'; Marker = 'PASS: camera-inverse'; TimeoutMs = 30000 }
}

function Resolve-DriveSpec {
    param([string] $Name)
    if (-not $Name) { throw 'drive requires a feature id from features/ or a Regression selector' }
    if ($script:FeatureMap.ContainsKey($Name)) { return $script:FeatureMap[$Name] }
    return @{ Arg = $Name; Marker = "PASS: $Name"; TimeoutMs = 120000 }
}

function New-RunId {
    return ('{0:yyyyMMdd-HHmmss}-{1}' -f (Get-Date), $PID)
}

function Get-VerifyRoot {
    param([string] $Repo)
    return Join-Path $Repo 'Binaries\Verify'
}

function Get-RunPaths {
    param([string] $Repo, [string] $RunId)
    $root = Join-Path (Get-VerifyRoot $Repo) $RunId
    return [pscustomobject]@{
        Root      = $root
        Evidence  = Join-Path $root 'evidence'
        Instance  = Join-Path $root 'instance'
        StateFile = Join-Path $root 'instance\state.json'
        EditorDir = Join-Path $root 'instance\Editor'
    }
}

function Ensure-Evidence {
    param($Paths)
    New-Item -ItemType Directory -Force -Path $Paths.Evidence | Out-Null
}

function Read-State {
    param($Paths)
    if (-not (Test-Path -LiteralPath $Paths.StateFile)) { return $null }
    return Get-Content -LiteralPath $Paths.StateFile -Raw | ConvertFrom-Json
}

function Write-State {
    param($Paths, $State)
    New-Item -ItemType Directory -Force -Path $Paths.Instance | Out-Null
    ($State | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath $Paths.StateFile -Encoding utf8
}

function Ensure-Win32 {
    if ('RbVerify.Native' -as [type]) { return }
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;
namespace RbVerify {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    public static class Native {
        public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
        [DllImport("user32.dll")]
        public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
        [DllImport("user32.dll")]
        public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
        [DllImport("user32.dll")]
        public static extern bool IsWindowVisible(IntPtr hWnd);
        [DllImport("user32.dll")]
        public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
        [DllImport("user32.dll")]
        public static extern bool SetForegroundWindow(IntPtr hWnd);
        [DllImport("user32.dll")]
        public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    }
}
'@
}

function Get-EditorHwnd {
    param([int] $ProcessId)
    Ensure-Win32
    $holder = @{ hwnd = [IntPtr]::Zero }
    $callback = [RbVerify.Native+EnumWindowsProc] {
        param([IntPtr] $Hwnd, [IntPtr] $LParam)
        $windowPid = [uint32]0
        [void][RbVerify.Native]::GetWindowThreadProcessId($Hwnd, [ref]$windowPid)
        if ([int]$windowPid -ne $ProcessId) { return $true }
        if (-not [RbVerify.Native]::IsWindowVisible($Hwnd)) { return $true }
        $title = New-Object System.Text.StringBuilder 256
        [void][RbVerify.Native]::GetWindowText($Hwnd, $title, $title.Capacity)
        if ($title.ToString() -eq 'Editor') {
            $holder.hwnd = $Hwnd
            return $false
        }
        return $true
    }
    [void][RbVerify.Native]::EnumWindows($callback, [IntPtr]::Zero)
    return $holder.hwnd
}

function Stop-OwnedProcess {
    param([int] $ProcessId)
    if ($ProcessId -le 0) { return }
    $proc = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
    if (-not $proc) { return }
    try {
        $proc.CloseMainWindow() | Out-Null
        if (-not $proc.WaitForExit(4000)) {
            $proc.Kill()
            $proc.WaitForExit()
        }
    } catch {
        Stop-Process -Id $ProcessId -Force -ErrorAction SilentlyContinue
    }
}

function Write-DoctorReport {
    param(
        [string] $Repo,
        [string] $Configuration,
        $Paths
    )
    $lines = New-Object System.Collections.Generic.List[string]
    $status = @{ failed = $false }
    function Add-Check {
        param([bool] $Ok, [string] $Name, [string] $Detail)
        $prefix = if ($Ok) { 'OK' } else { 'FAIL' }
        if (-not $Ok) { $status.failed = $true }
        $lines.Add("$prefix $Name=$Detail") | Out-Null
    }

    $head = Get-Head $Repo
    Add-Check $true 'repo' $Repo
    Add-Check $true 'head' $head
    Add-Check $true 'configuration' $Configuration

    $editor = Get-EditorExe $Repo $Configuration
    $regression = Get-RegressionExe $Repo $Configuration
    Add-Check (Test-Path -LiteralPath $editor) 'editor' $editor
    Add-Check (Test-Path -LiteralPath $regression) 'regression' $regression
    if (Test-Path -LiteralPath $editor) {
        try { Assert-EditorAssets (Split-Path $editor -Parent) 'Editor'; Add-Check $true 'editor_assets' (Split-Path $editor -Parent) }
        catch { Add-Check $false 'editor_assets' $_.Exception.Message }
    }
    if (Test-Path -LiteralPath $regression) {
        try { Assert-EditorAssets (Split-Path $regression -Parent) 'Regression'; Add-Check $true 'regression_assets' (Split-Path $regression -Parent) }
        catch { Add-Check $false 'regression_assets' $_.Exception.Message }
    }

    $msbuildOk = $true
    try { $msbuild = Get-MSBuild } catch { $msbuildOk = $false; $msbuild = $_.Exception.Message }
    Add-Check $msbuildOk 'msbuild' $msbuild

    if ($Paths -and (Test-Path -LiteralPath $Paths.StateFile)) {
        $state = Read-State $Paths
        $proc = Get-Process -Id ([int]$state.pid) -ErrorAction SilentlyContinue
        Add-Check ($null -ne $proc) 'instance_pid' ([string]$state.pid)
        if ($proc) {
            $hwnd = Get-EditorHwnd ([int]$state.pid)
            Add-Check ($hwnd -ne [IntPtr]::Zero) 'instance_hwnd' ('0x{0:X}' -f $hwnd.ToInt64())
            $log = Join-Path $Paths.Instance 'editor.stdout.log'
            $logText = if (Test-Path -LiteralPath $log) { Get-Content -LiteralPath $log -Raw } else { '' }
            $attached = $logText -match 'Window System Init' -or $logText -match 'Window Created!: Editor'
            Add-Check $attached 'instance_log' $log
            $exePath = $proc.Path
            Add-Check ($exePath -eq $state.exe) 'instance_exe' $exePath
        }
        Add-Check $true 'run_id' $state.runId
        Add-Check $true 'work_dir' $state.workDir
    }

    $report = $lines -join [Environment]::NewLine
    Write-Output $report
    if ($status.failed) { throw 'doctor failed' }
}

function Invoke-Build {
    param([string] $Repo, [string] $Configuration)
    $premake = Join-Path $Repo 'Vendor\Binaries\Premake\Windows\premake5.exe'
    if (-not (Test-Path -LiteralPath $premake)) { throw "premake missing: $premake" }
    Push-Location $Repo
    try {
        & $premake --file=Build.lua vs2022
        if ($LASTEXITCODE -ne 0) { throw "premake failed: $LASTEXITCODE" }
        $msbuild = Get-MSBuild
        foreach ($project in @('Editor\Editor.vcxproj', 'Tests\Regression.vcxproj')) {
            $path = Join-Path $Repo $project
            & $msbuild $path /t:Build /m /nr:false "/p:Configuration=$Configuration" /p:Platform=x64 /v:minimal /nologo
            if ($LASTEXITCODE -ne 0) { throw "$project $Configuration build failed: $LASTEXITCODE" }
        }
    } finally { Pop-Location }
    Assert-EditorAssets (Split-Path (Get-EditorExe $Repo $Configuration) -Parent) 'Editor'
    Assert-EditorAssets (Split-Path (Get-RegressionExe $Repo $Configuration) -Parent) 'Regression'
    Write-Output "BUILD_OK configuration=$Configuration editor=$(Get-EditorExe $Repo $Configuration) regression=$(Get-RegressionExe $Repo $Configuration)"
}

function Invoke-LaunchEditor {
    param([string] $Repo, [string] $Configuration, [string] $RunId)
    $paths = Get-RunPaths $Repo $RunId
    if (Test-Path -LiteralPath $paths.StateFile) {
        $existing = Read-State $paths
        $alive = Get-Process -Id ([int]$existing.pid) -ErrorAction SilentlyContinue
        if ($alive) {
            throw "Run $RunId already has a live Editor pid $($existing.pid). Cleanup first. Refusing to double-drive."
        }
    }
    $source = Split-Path (Get-EditorExe $Repo $Configuration) -Parent
    if (-not (Test-Path -LiteralPath (Join-Path $source 'Editor.exe'))) {
        throw "Editor.exe missing. Run build first: $source"
    }
    Assert-EditorAssets $source 'Editor'
    if (Test-Path -LiteralPath $paths.EditorDir) {
        Remove-Item -LiteralPath $paths.EditorDir -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $paths.Instance | Out-Null
    Copy-Item -LiteralPath $source -Destination $paths.EditorDir -Recurse
    Ensure-Evidence $paths
    $exe = Join-Path $paths.EditorDir 'Editor.exe'
    $stdout = Join-Path $paths.Instance 'editor.stdout.log'
    $stderr = Join-Path $paths.Instance 'editor.stderr.log'
    $proc = Start-Process -FilePath $exe -WorkingDirectory $paths.EditorDir `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
        -WindowStyle Hidden -PassThru
    $state = [pscustomobject]@{
        runId     = $RunId
        pid       = $proc.Id
        exe       = $exe
        workDir   = $paths.EditorDir
        startedUtc = [DateTime]::UtcNow.ToString('o')
        configuration = $Configuration
        head      = Get-Head $Repo
    }
    Write-State $paths $state
    $deadline = (Get-Date).AddSeconds(30)
    $hwnd = [IntPtr]::Zero
    $attached = $false
    do {
        if ($proc.HasExited) {
            $out = if (Test-Path $stdout) { Get-Content $stdout -Raw } else { '' }
            $err = if (Test-Path $stderr) { Get-Content $stderr -Raw } else { '' }
            throw "Editor.exe exited $($proc.ExitCode) before becoming ready.`n$out`n$err"
        }
        $hwnd = Get-EditorHwnd $proc.Id
        $logText = if (Test-Path -LiteralPath $stdout) { Get-Content -LiteralPath $stdout -Raw } else { '' }
        $attached = $logText -match 'Window System Init' -or $logText -match 'Window Created!: Editor'
        if (($hwnd -ne [IntPtr]::Zero) -and $attached) { break }
        Start-Sleep -Milliseconds 200
    } while ((Get-Date) -lt $deadline)
    if ($hwnd -eq [IntPtr]::Zero) { throw "Editor window titled Editor did not appear for pid $($proc.Id) within 30s" }
    if (-not $attached) {
        Start-Sleep -Seconds 2
        $hwnd = Get-EditorHwnd $proc.Id
        if ($hwnd -eq [IntPtr]::Zero -or $proc.HasExited) {
            throw "Editor window appeared but attach did not log Window System Init. See $stdout"
        }
    }
    Copy-Item -LiteralPath $stdout -Destination (Join-Path $paths.Evidence 'editor.stdout.log') -Force
    if (Test-Path -LiteralPath $stderr) {
        Copy-Item -LiteralPath $stderr -Destination (Join-Path $paths.Evidence 'editor.stderr.log') -Force
    }
    Write-Output "LAUNCH_OK run_id=$RunId pid=$($proc.Id) hwnd=0x$('{0:X}' -f $hwnd.ToInt64()) work_dir=$($paths.EditorDir)"
    Write-Output "EVIDENCE=$($paths.Evidence)"
}

function Invoke-Capture {
    param([string] $Repo, [string] $RunId)
    $paths = Get-RunPaths $Repo $RunId
    $state = Read-State $paths
    if (-not $state) { throw "No instance state for run $RunId. launch-editor first, or skip capture for Regression drives." }
    $proc = Get-Process -Id ([int]$state.pid) -ErrorAction SilentlyContinue
    if (-not $proc) { throw "Editor pid $($state.pid) is not running" }
    $hwnd = Get-EditorHwnd ([int]$state.pid)
    if ($hwnd -eq [IntPtr]::Zero) { throw "No visible window titled Editor for pid $($state.pid)" }
    Ensure-Win32
    [void][RbVerify.Native]::ShowWindow($hwnd, 9)
    [void][RbVerify.Native]::SetForegroundWindow($hwnd)
    Start-Sleep -Milliseconds 400
    Add-Type -AssemblyName System.Drawing
    $rect = New-Object RbVerify.RECT
    if (-not [RbVerify.Native]::GetWindowRect($hwnd, [ref]$rect)) { throw 'GetWindowRect failed' }
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    if ($width -le 0 -or $height -le 0) { throw "Editor window has empty bounds ${width}x${height}" }
    Ensure-Evidence $paths
    $png = Join-Path $paths.Evidence 'editor.png'
    $bmp = New-Object System.Drawing.Bitmap $width, $height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    try {
        $g.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bmp.Size)
        $bmp.Save($png, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $g.Dispose()
        $bmp.Dispose()
    }
    Write-Output "CAPTURE_OK path=$png size=${width}x${height}"
    Write-Output "EVIDENCE=$($paths.Evidence)"
}

function Invoke-Drive {
    param([string] $Repo, [string] $Configuration, [string] $Feature, [string] $RunId)
    $spec = Resolve-DriveSpec $Feature
    $paths = Get-RunPaths $Repo $RunId
    Ensure-Evidence $paths
    $regression = Get-RegressionExe $Repo $Configuration
    if (-not (Test-Path -LiteralPath $regression)) { throw "Regression.exe missing. Run build. $regression" }
    $workDir = Split-Path $regression -Parent
    Assert-EditorAssets $workDir 'Regression'
    $verifyRoot = Get-VerifyRoot $Repo
    New-Item -ItemType Directory -Force -Path $verifyRoot | Out-Null
    $lockPath = Join-Path $verifyRoot '.drive.lock'
    if (Test-Path -LiteralPath $lockPath) {
        $lockPid = [int]((Get-Content -LiteralPath $lockPath -Raw).Trim())
        if (Get-Process -Id $lockPid -ErrorAction SilentlyContinue) {
            throw "Another rb-verify drive is running (pid $lockPid). Refuse to double-drive Regression."
        }
    }
    Set-Content -LiteralPath $lockPath -Value $PID -Encoding ascii
    $stdout = Join-Path $paths.Evidence 'stdout.log'
    $stderr = Join-Path $paths.Evidence 'stderr.log'
    $summary = Join-Path $paths.Evidence 'summary.txt'
    $proc = $null
    $exitCode = -1
    try {
        @"
run_id=$RunId
feature=$Feature
selector=$($spec.Arg)
marker=$($spec.Marker)
configuration=$Configuration
head=$(Get-Head $Repo)
exe=$regression
work_dir=$workDir
"@ | Set-Content -LiteralPath $summary -Encoding utf8
        $proc = Start-Process -FilePath $regression -ArgumentList $spec.Arg `
            -WorkingDirectory $workDir -RedirectStandardOutput $stdout `
            -RedirectStandardError $stderr -WindowStyle Hidden -PassThru
        $state = [pscustomobject]@{
            runId     = $RunId
            pid       = $proc.Id
            exe       = $regression
            workDir   = $workDir
            feature   = $Feature
            selector  = $spec.Arg
            startedUtc = [DateTime]::UtcNow.ToString('o')
            kind      = 'regression'
            configuration = $Configuration
            head      = Get-Head $Repo
        }
        Write-State $paths $state
        if (-not $proc.WaitForExit([int]$spec.TimeoutMs)) {
            throw "Regression $($spec.Arg) exceeded $($spec.TimeoutMs) ms"
        }
        $proc.WaitForExit()
        $exitCode = $proc.ExitCode
        $text = if (Test-Path $stdout) { Get-Content -LiteralPath $stdout -Raw } else { '' }
        $pass = $text -match ('(?m)^' + [regex]::Escape($spec.Marker) + '\r?$')
        Add-Content -LiteralPath $summary -Value "exit=$exitCode"
        Add-Content -LiteralPath $summary -Value "pass_marker=$pass"
        if ($text) {
            $text -split "`n" | Where-Object { $_ -match '^(PASS:|FAIL:|OBSERVED:|DIALOG)' } |
                ForEach-Object { Add-Content -LiteralPath $summary -Value $_.TrimEnd() }
        }
        Get-ChildItem -LiteralPath $workDir -Directory -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -like 'issue-*-work-*' } |
            ForEach-Object {
                Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $paths.Evidence $_.Name) -Recurse -Force
            }
        if ($exitCode -ne 0) { throw "Regression $($spec.Arg) exited $exitCode. See $stdout and $stderr" }
        if (-not $pass) { throw "Regression $($spec.Arg) did not print $($spec.Marker). See $stdout" }
        Write-Output "DRIVE_OK run_id=$RunId feature=$Feature selector=$($spec.Arg) marker=$($spec.Marker)"
        Write-Output "EVIDENCE=$($paths.Evidence)"
    } finally {
        if ($null -ne $proc) {
            if (-not $proc.HasExited) {
                $proc.Kill()
                $proc.WaitForExit()
            }
            $proc.Dispose()
        }
        if (Test-Path -LiteralPath $paths.StateFile) {
            $st = Read-State $paths
            if ($st.kind -eq 'regression') {
                Remove-Item -LiteralPath $paths.StateFile -Force
            }
        }
        if (Test-Path -LiteralPath $lockPath) {
            $lockPid = [int]((Get-Content -LiteralPath $lockPath -Raw).Trim())
            if ($lockPid -eq $PID) { Remove-Item -LiteralPath $lockPath -Force }
        }
    }
}

function Invoke-Cleanup {
    param([string] $Repo, [string] $RunId)
    $verifyRoot = Get-VerifyRoot $Repo
    $targets = @()
    if ($RunId) {
        $targets = @((Get-RunPaths $Repo $RunId).Root)
    } elseif (Test-Path -LiteralPath $verifyRoot) {
        $targets = @(Get-ChildItem -LiteralPath $verifyRoot -Directory | ForEach-Object { $_.FullName })
    }
    foreach ($root in $targets) {
        $stateFile = Join-Path $root 'instance\state.json'
        if (Test-Path -LiteralPath $stateFile) {
            $state = Get-Content -LiteralPath $stateFile -Raw | ConvertFrom-Json
            if ($state.pid) { Stop-OwnedProcess ([int]$state.pid) }
        }
        $instance = Join-Path $root 'instance'
        if (Test-Path -LiteralPath $instance) {
            Remove-Item -LiteralPath $instance -Recurse -Force
        }
        $evidence = Join-Path $root 'evidence'
        if (Test-Path -LiteralPath $evidence) {
            Write-Output "CLEANUP_OK run=$(Split-Path $root -Leaf) evidence=$evidence"
        } else {
            Write-Output "CLEANUP_OK run=$(Split-Path $root -Leaf) evidence=none"
        }
    }
    $lockPath = Join-Path $verifyRoot '.drive.lock'
    if (Test-Path -LiteralPath $lockPath) {
        $lockPid = [int]((Get-Content -LiteralPath $lockPath -Raw).Trim())
        if (-not (Get-Process -Id $lockPid -ErrorAction SilentlyContinue)) {
            Remove-Item -LiteralPath $lockPath -Force
        }
    }
}

$Repo = Get-RepoRoot $RepoRoot
$RunIdProvided = -not [string]::IsNullOrWhiteSpace($RunId)
if (-not $RunIdProvided) { $RunId = New-RunId }
$Paths = Get-RunPaths $Repo $RunId

switch ($Command) {
    'build' {
        Invoke-Build -Repo $Repo -Configuration $Configuration
    }
    'doctor' {
        $doctorPaths = $null
        if ($RunIdProvided) { $doctorPaths = $Paths }
        Write-DoctorReport -Repo $Repo -Configuration $Configuration -Paths $doctorPaths
    }
    'launch-editor' {
        Invoke-LaunchEditor -Repo $Repo -Configuration $Configuration -RunId $RunId
    }
    'drive' {
        Invoke-Drive -Repo $Repo -Configuration $Configuration -Feature $Feature -RunId $RunId
    }
    'capture' {
        if (-not $RunIdProvided) { throw 'capture requires -RunId of a launch-editor instance' }
        Invoke-Capture -Repo $Repo -RunId $RunId
    }
    'cleanup' {
        if ($RunIdProvided) {
            Invoke-Cleanup -Repo $Repo -RunId $RunId
        } else {
            Invoke-Cleanup -Repo $Repo -RunId ''
        }
    }
}
