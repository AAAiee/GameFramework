param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration,
    [Parameter(Mandatory = $true)]
    [string] $BinaryRoot
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not [IO.Path]::IsPathRooted($BinaryRoot)) {
    throw 'BinaryRoot must be an absolute path'
}

# Accept either the configuration output root or its parent binary root.
$candidates = @(
    (Join-Path $BinaryRoot 'Regression/Regression.exe'),
    (Join-Path $BinaryRoot "$Configuration/Regression/Regression.exe"),
    (Join-Path $BinaryRoot "windows-x86_64/$Configuration/Regression/Regression.exe")
)
$binary = $candidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
if (-not $binary) { throw "Regression.exe not found for $Configuration under $BinaryRoot" }
$outputDirectory = Split-Path -Parent $binary
$stdout = Join-Path $outputDirectory 'issue-16.stdout.log'
$stderr = Join-Path $outputDirectory 'issue-16.stderr.log'
$process = $null
try {
    $process = Start-Process -FilePath $binary -ArgumentList 'editor-issue-16' `
        -WorkingDirectory $outputDirectory -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr -WindowStyle Hidden -PassThru
    if (-not $process.WaitForExit(60000)) {
        throw 'Editor fixture exceeded its 60 second timeout'
    }
    $process.WaitForExit()
    $exitCode = $process.ExitCode
    $result = Get-Content -LiteralPath $stdout -Raw
    if ($exitCode -ne 0) {
        throw "Editor fixture exited $exitCode. See $stdout and $stderr"
    }
    if ($result -notmatch '(?m)^PASS: editor-issue-16\r?$') {
        throw "Editor fixture did not report completed behavioral assertions. See $stdout"
    }
    Write-Output 'PASS: editor-issue-16'
} finally {
    if ($null -ne $process) {
        if (-not $process.HasExited) {
            $process.Kill()
            $process.WaitForExit()
        }
        $process.Dispose()
    }
}
