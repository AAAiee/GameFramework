#requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$Worktree, [Parameter(Mandatory)][string]$MSBuild)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = (Resolve-Path -LiteralPath $Worktree).Path
$build = Get-Item -LiteralPath $MSBuild
$vs = $build.Directory.Parent.Parent.Parent.FullName
$toolchain = & (Join-Path $PSScriptRoot 'Resolve-NativeToolchain.ps1') -MSBuild $MSBuild
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
    compiler = $toolchain.compiler
    windowsSdk = $toolchain.windowsSdk
    toolchain = @{ toolset = $toolchain.toolset; sdk = $toolchain.sdk }
    graphics = $graphics
    premake = (Get-FileHash -LiteralPath (Join-Path $root 'Vendor/Binaries/Premake/Windows/premake5.exe') -Algorithm SHA256).Hash
} | ConvertTo-Json -Depth 8 -Compress
