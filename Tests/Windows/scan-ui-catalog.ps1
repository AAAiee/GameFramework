# Fails if Editor/src widget labels and kUiCatalog drift apart in either direction.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$catalogPath = Join-Path $root 'Tests\src\UiCatalog.h'
$editorSrc = Join-Path $root 'Editor\src'
$noLiteral = @('Orthographic', 'Perspective', '##Splitter')

$catalogText = [IO.File]::ReadAllText($catalogPath)
$catalogBlock = [regex]::Match($catalogText, 'kUiCatalog\[\]\s*=\s*\{(?<rows>.*?)\n\s*\};', 'Singleline')
if (-not $catalogBlock.Success) {
    Write-Output 'FAIL cannot read kUiCatalog from UiCatalog.h'
    exit 1
}

$catalog = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($row in [regex]::Matches($catalogBlock.Groups['rows'].Value, '\{\s*"(?<id>[^"]+)"')) {
    [void]$catalog.Add($row.Groups['id'].Value)
}

$pattern = '(?:ImGui::(?:MenuItem|BeginMenu|Button|Checkbox|InputText|DragFloat|ColorEdit4|BeginCombo|Selectable|Begin)|EditorUi::(?:colorEdit4|beginCombo))\(\s*"(?<id>[^"]+)"'
$source = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($file in Get-ChildItem -LiteralPath $editorSrc -Recurse -File -Include *.cpp, *.h) {
    $text = [IO.File]::ReadAllText($file.FullName)
    foreach ($hit in [regex]::Matches($text, $pattern)) {
        [void]$source.Add($hit.Groups['id'].Value)
    }
}

$missingFromCatalog = @($source | Where-Object { -not $catalog.Contains($_) } | Sort-Object)
$missingFromSource = @($catalog | Where-Object { -not $source.Contains($_) -and $noLiteral -notcontains $_ } | Sort-Object)

if ($missingFromCatalog.Count -gt 0) {
    Write-Output 'FAIL Editor/src labels missing from kUiCatalog:'
    $missingFromCatalog | ForEach-Object { Write-Output "  $_" }
}
if ($missingFromSource.Count -gt 0) {
    Write-Output 'FAIL catalog ids with no Editor/src widget literal:'
    $missingFromSource | ForEach-Object { Write-Output "  $_" }
}
if ($missingFromCatalog.Count -gt 0 -or $missingFromSource.Count -gt 0) {
    exit 1
}

Write-Output ("OK catalog {0} ids and {1} Editor/src labels" -f $catalog.Count, $source.Count)
exit 0
