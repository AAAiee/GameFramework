param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string] $EvidenceDir
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$catalogPath = Join-Path $root 'Tests\src\UiCatalog.h'

$evidence = (Resolve-Path -LiteralPath $EvidenceDir).Path
$catalogText = [IO.File]::ReadAllText($catalogPath)
$catalogBlock = [regex]::Match($catalogText, 'kUiCatalog\[\]\s*=\s*\{(?<rows>.*?)\n\s*\};', 'Singleline')
if (-not $catalogBlock.Success) {
    Write-Output 'FAIL cannot read kUiCatalog from UiCatalog.h'
    exit 1
}

$ids = [System.Collections.Generic.List[string]]::new()
$kinds = @{}
foreach ($row in [regex]::Matches($catalogBlock.Groups['rows'].Value, '\{\s*"(?<id>[^"]+)"\s*,\s*UiKind::(?<kind>\w+)')) {
    $id = $row.Groups['id'].Value
    $ids.Add($id)
    $kinds[$id] = $row.Groups['kind'].Value
}

$logs = @(Get-ChildItem -LiteralPath $evidence -Recurse -File -Filter *.log)
if ($logs.Count -eq 0) {
    Write-Output "FAIL no .log files under $evidence"
    exit 1
}

$driven = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$labels = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($log in $logs) {
    $text = [IO.File]::ReadAllText($log.FullName)
    foreach ($hit in [regex]::Matches($text, '(?m)^DRIVEN: (?<path>.+?)\r?$')) {
        [void]$driven.Add($hit.Groups['path'].Value.Trim())
    }
    foreach ($hit in [regex]::Matches($text, "label='(?<id>[^']*)'")) {
        [void]$labels.Add($hit.Groups['id'].Value)
    }
}

$missing = [System.Collections.Generic.List[string]]::new()
foreach ($id in $ids) {
    if ($driven.Contains($id)) { continue }
    if ($kinds[$id] -eq 'Window' -and $labels.Contains($id)) { continue }
    $missing.Add($id)
}

if ($missing.Count -gt 0) {
    Write-Output 'FAIL catalog ids never driven:'
    $missing | Sort-Object | ForEach-Object { Write-Output "  $_" }
    exit 1
}

Write-Output ("OK catalog {0} ids, {1} DRIVEN paths, 0 pending" -f $ids.Count, $driven.Count)
exit 0
