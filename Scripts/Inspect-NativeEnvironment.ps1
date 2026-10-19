#requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$Worktree, [Parameter(Mandatory)][string]$MSBuild)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = (Resolve-Path -LiteralPath $Worktree).Path
$build = Get-Item -LiteralPath $MSBuild
$vs = $build.Directory.Parent.Parent.Parent.FullName
$compiler = Get-ChildItem -LiteralPath (Join-Path $vs 'VC/Tools/MSVC') -Directory |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (-not $compiler) { throw 'Installed Visual C++ compiler is unavailable.' }
$cl = Get-Item -LiteralPath (Join-Path $compiler.FullName 'bin/Hostx64/x64/cl.exe')
$kitsRoot = (Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots').KitsRoot10
$sdks = @(Get-ChildItem -LiteralPath (Join-Path $kitsRoot 'Include') -Directory |
    Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'um/Windows.h') } |
    Sort-Object Name | ForEach-Object { @{ version = $_.Name; windowsHeader = (Get-FileHash -LiteralPath (Join-Path $_.FullName 'um/Windows.h') -Algorithm SHA256).Hash } })
if ($sdks.Count -eq 0) { throw 'Installed Windows SDK headers are unavailable.' }
$graphics = @(Get-CimInstance -ClassName Win32_VideoController | Sort-Object PNPDeviceID |
    ForEach-Object { @{ name = $_.Name; driverVersion = $_.DriverVersion; driverDate = "$($_.DriverDate)"; device = $_.PNPDeviceID; status = $_.Status } })
if ($graphics.Count -eq 0 -or @($graphics | Where-Object { -not $_.driverVersion -or $_.status -ne 'OK' }).Count -gt 0) {
    throw 'Actual graphics driver discovery is unavailable or unhealthy; no input-desktop fallback is permitted.'
}
$os = Get-CimInstance -ClassName Win32_OperatingSystem
@{
    os = @{ version = $os.Version; build = $os.BuildNumber; architecture = $os.OSArchitecture }
    powershell = "$($PSVersionTable.PSVersion)"
    msbuild = @{ path = $build.FullName; version = $build.VersionInfo.FileVersion; sha256 = (Get-FileHash -LiteralPath $build.FullName -Algorithm SHA256).Hash }
    compiler = @{ path = $cl.FullName; version = $cl.VersionInfo.FileVersion; sha256 = (Get-FileHash -LiteralPath $cl.FullName -Algorithm SHA256).Hash }
    windowsSdks = $sdks
    graphics = $graphics
    premake = (Get-FileHash -LiteralPath (Join-Path $root 'Vendor/Binaries/Premake/Windows/premake5.exe') -Algorithm SHA256).Hash
} | ConvertTo-Json -Depth 8 -Compress
