# Fails if Editor/src grows an ImGui widget label that UiCatalog.h does not list.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$catalogPath = Join-Path $root 'Tests\src\UiCatalog.h'
$editorSrc = Join-Path $root 'Editor\src'
$catalog = Get-Content -LiteralPath $catalogPath -Raw

$pattern = 'ImGui::(?:MenuItem|BeginMenu|Button|Checkbox|InputText|DragFloat|ColorEdit4|BeginCombo|Selectable|Begin)\(\s*"([^"]+)"'
$found = [System.Collections.Generic.HashSet[string]]::new()
Get-ChildItem -LiteralPath $editorSrc -Recurse -Include *.cpp, *.h | ForEach-Object {
    Select-String -LiteralPath $_.FullName -Pattern $pattern | ForEach-Object {
        [void]$found.Add($_.Matches[0].Groups[1].Value)
    }
}

$missing = @()
foreach ($label in ($found | Sort-Object)) {
    if ($catalog -notmatch [regex]::Escape('"' + $label + '"')) {
        $missing += $label
    }
}

if ($missing.Count -gt 0) {
    Write-Output 'FAIL catalog missing labels:'
    $missing | ForEach-Object { Write-Output "  $_" }
    exit 1
}

Write-Output ("OK catalog covers {0} Editor ImGui labels" -f $found.Count)
exit 0
