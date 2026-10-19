#requires -Version 7.0
# Resolve the same v143/x64 defaults as generated projects, not newest-installed.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$MSBuild, [string]$VCToolsVersion, [string]$WindowsTargetPlatformVersion)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$build = Get-Item -LiteralPath $MSBuild
$vs = $build.Directory.Parent.Parent.Parent.FullName
$targets = Join-Path $vs 'MSBuild/Microsoft/VC/v170'
$probe = Join-Path ([IO.Path]::GetTempPath()) ('native-toolchain-' + [Guid]::NewGuid().ToString('N') + '.vcxproj')
try {
    @'
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup><Configuration>Debug</Configuration><Platform>x64</Platform><PlatformToolset>v143</PlatformToolset><PreferredToolArchitecture>x64</PreferredToolArchitecture></PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
</Project>
'@ | Set-Content -LiteralPath $probe -Encoding utf8
    $arguments = @($probe, '/nologo', '/nr:false', "/p:VCTargetsPath=$targets\", '/getProperty:VCToolsVersion,VCToolsInstallDir,TargetPlatformVersion,WindowsSdkDir')
    if ($VCToolsVersion) { $arguments += "/p:VCToolsVersion=$VCToolsVersion" }
    if ($WindowsTargetPlatformVersion) { $arguments += "/p:WindowsTargetPlatformVersion=$WindowsTargetPlatformVersion" }
    $output = & $MSBuild @arguments
    if ($LASTEXITCODE -ne 0) { throw "Cannot resolve actual v143 toolchain: $output" }
    $selection = ($output -join "`n" | ConvertFrom-Json).Properties
    if ($selection.VCToolsVersion -notmatch '^\d+\.\d+\.\d+$' -or $selection.TargetPlatformVersion -notmatch '^\d+\.\d+\.\d+\.\d+$') { throw "Unresolved: $($selection | ConvertTo-Json -Compress)" }
    $cl = Get-Item -LiteralPath (Join-Path $selection.VCToolsInstallDir 'bin/Hostx64/x64/cl.exe')
    $header = Get-Item -LiteralPath (Join-Path $selection.WindowsSdkDir "Include/$($selection.TargetPlatformVersion)/um/Windows.h")
    @{
        toolset = $selection.VCToolsVersion
        sdk = $selection.TargetPlatformVersion
        compiler = @{ path = $cl.FullName; version = $cl.VersionInfo.FileVersion; sha256 = (Get-FileHash -LiteralPath $cl.FullName -Algorithm SHA256).Hash }
        windowsSdk = @{ version = $selection.TargetPlatformVersion; windowsHeader = $header.FullName; sha256 = (Get-FileHash -LiteralPath $header.FullName -Algorithm SHA256).Hash }
    }
} finally { Remove-Item -LiteralPath $probe -Force -ErrorAction SilentlyContinue }


