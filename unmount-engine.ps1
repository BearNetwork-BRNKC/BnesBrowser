<#
.SYNOPSIS
Unmount Engine：解除 mount-engine.ps1 建立的整合狀態。

.DESCRIPTION
僅刪除 .bnes-build/mount-state.json（可逆操作的解除）。
不修改任何源碼、不觸碰 Chromium workspace。

用法：
  pwsh -NoProfile -ExecutionPolicy Bypass -File unmount-engine.ps1 [-SrcRoot <path>]
#>
[CmdletBinding()]
param(
    [string]$SrcRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $SrcRoot) { $SrcRoot = Split-Path -Parent $RepoRoot }
$BuildRoot = Split-Path -Parent $SrcRoot
$StatePath = Join-Path $BuildRoot '.bnes-build\mount-state.json'

if (Test-Path $StatePath) {
    Remove-Item $StatePath -Force
    Write-Host 'Unmount 完成：mount-state.json 已移除。' -ForegroundColor Green
} else {
    Write-Host '目前沒有 mount 狀態（mount-state.json 不存在）。' -ForegroundColor Yellow
}
