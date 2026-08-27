#Requires -Version 5.1
<#
.SYNOPSIS
  Fail if BNES wallet-CRX allow overlays were lost in an upstream merge.

.DESCRIPTION
  Reads bnes/protected_overlays.json and verifies every required file exists
  and still contains its markers (extension ID, wrap symbols, BNES_GUARD).
  Also rejects the known-wrong overlay path that silently disabled drag-drop.

  Exit 0 = intact. Exit 1 = missing file, missing marker, or forbidden path.

.PARAMETER Root
  Tree to inspect (bnes-brave-core, or mapped src\brave). Default: repo root.

.PARAMETER Repair
  Delete forbidden leftover files (merge-copy does not remove dest-only paths).
  Use on the mapped src\brave tree after SYNC.
#>
[CmdletBinding()]
param(
    [string]$Root = "",
    [switch]$Repair
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$BnesDir = Split-Path $ScriptDir -Parent
$CoreRoot = Split-Path $BnesDir -Parent
if ([string]::IsNullOrWhiteSpace($Root)) {
    $Root = $CoreRoot
}
$Root = [System.IO.Path]::GetFullPath($Root)

$ManifestPath = Join-Path $Root "bnes\protected_overlays.json"
if (-not (Test-Path -LiteralPath $ManifestPath)) {
    Write-Host "[BNES GUARD] FAIL: missing manifest $ManifestPath" -ForegroundColor Red
    Write-Host "  This file is L0 (bnes/). If an upstream merge deleted it, restore from git." -ForegroundColor Red
    exit 1
}

$manifest = Get-Content -LiteralPath $ManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
$expectedId = [string]$manifest.extensionId
$sentinel = [string]$manifest.sentinel
$failures = New-Object System.Collections.Generic.List[string]

function Add-Fail([string]$Message) {
    $script:failures.Add($Message)
}

Write-Host "[BNES GUARD] checking $Root" -ForegroundColor Cyan
Write-Host "[BNES GUARD] extension ID $expectedId" -ForegroundColor Cyan

foreach ($item in $manifest.files) {
    $rel = [string]$item.path
    $full = Join-Path $Root ($rel -replace '/', '\')
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
        Add-Fail "MISSING $($item.owned) file: $rel"
        continue
    }
    $text = Get-Content -LiteralPath $full -Raw -Encoding UTF8
    foreach ($needle in $item.mustContain) {
        if ($text.IndexOf([string]$needle) -lt 0) {
            Add-Fail "LOST MARKER in $rel : '$needle'"
        }
    }
}

foreach ($item in $manifest.forbidden) {
    $rel = [string]$item.path
    $full = Join-Path $Root ($rel -replace '/', '\')
    if (Test-Path -LiteralPath $full -PathType Leaf) {
        if ($Repair) {
            Remove-Item -LiteralPath $full -Force
            Write-Host "[BNES GUARD] removed leftover forbidden path: $rel" -ForegroundColor Yellow
        }
        else {
            Add-Fail "FORBIDDEN PATH present: $rel ($($item.reason) Mapped src\brave is merge-only; leftovers need -Repair.)"
        }
    }
}

$constants = Join-Path $Root "browser\extensions\bnes_extension_constants.h"
if (Test-Path -LiteralPath $constants) {
    $constText = Get-Content -LiteralPath $constants -Raw -Encoding UTF8
    if ($constText.IndexOf($expectedId) -lt 0) {
        Add-Fail "ID mismatch: bnes_extension_constants.h does not contain manifest extensionId $expectedId"
    }
}

if ($failures.Count -gt 0) {
    Write-Host ""
    Write-Host "[BNES GUARD] FAIL - wallet CRX allowlist is incomplete. Do not ship this build." -ForegroundColor Red
    Write-Host "  Likely cause: Brave/Chromium upstream merge overwrote BNES overlays." -ForegroundColor Yellow
    Write-Host "  Restore the files listed below from git, keep BNES_GUARD comments, then re-run." -ForegroundColor Yellow
    Write-Host ""
    foreach ($f in $failures) {
        Write-Host "  - $f" -ForegroundColor Red
    }
    Write-Host ""
    Write-Host "  Manifest: $ManifestPath" -ForegroundColor DarkYellow
    exit 1
}

Write-Host "[BNES GUARD] OK - $($manifest.files.Count) protected overlays intact ($sentinel)." -ForegroundColor Green
exit 0
