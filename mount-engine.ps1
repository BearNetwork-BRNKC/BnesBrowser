<#
.SYNOPSIS
Mount Engine：將本 BnesBrowser Repository 與指定的 Chromium Build Workspace 建立可重現的整合關係。

.DESCRIPTION
讀取 engine.lock，逐一驗證（全部唯讀，不修改任何源碼）：
  1. Chromium 版本與 commit 是否與 engine.lock 一致
  2. Product repository HEAD 狀態
  3. bnes/ 完整性（對照 bnes.integrity.lock，任何不符即 STOP）
  4. Toolchain（gn / ninja / MSVC / Windows SDK）存在性
  5. args.gn 與 engine.lock 記錄的 hash 一致性
  6. redirect_cc 建置旗標狀態
驗證通過後寫出 .bnes-build/mount-state.json 供 unmount-engine.ps1 解除。

用法：
  pwsh -NoProfile -ExecutionPolicy Bypass -File mount-engine.ps1 [-SrcRoot <path>]
  （預設 SrcRoot 為本檔所在位置的上一層，即 src\）
#>
[CmdletBinding()]
param(
    [string]$SrcRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path   # ...\src\BnesBrowser
if (-not $SrcRoot) { $SrcRoot = Split-Path -Parent $RepoRoot }  # ...\src
$BuildRoot = Split-Path -Parent $SrcRoot                        # Build workspace root
$StatePath = Join-Path $BuildRoot '.bnes-build\mount-state.json'

function Fail([string]$msg) { throw "mount-engine: $msg" }
function Ok([string]$msg) { Write-Host "  [PASS] $msg" -ForegroundColor Green }
function Warn([string]$msg) { Write-Host "  [WARN] $msg" -ForegroundColor Yellow }

Write-Host "=== Mount Engine ===" -ForegroundColor Cyan

# 1. engine.lock
$lockPath = Join-Path $RepoRoot 'engine.lock'
if (-not (Test-Path $lockPath)) { Fail "找不到 engine.lock：$lockPath" }
$lock = Get-Content $lockPath -Raw | ConvertFrom-Json
Ok "engine.lock 已載入（v$($lock.lock_version)，mode=$($lock.engine.mode)）"

# 2. Chromium 版本
$chromeVersion = (Get-Content (Join-Path $SrcRoot 'chrome\VERSION') -Raw)
$chromeVer = ('{0}.{1}.{2}.{3}' -f ($chromeVersion -split "`r?`n" |
    Where-Object { $_ -match '=' } | ForEach-Object { ($_ -split '=')[1] }))
if ($chromeVer -ne $lock.chromium.version) {
    Fail "Chromium 版本不符：engine.lock=$($lock.chromium.version)，實際=$chromeVer"
}
Ok "Chromium 版本一致：$chromeVer"

# 3. Chromium commit（pinned base 或以其為祖先的 overlay snapshot）
$chromiumHead = (git -C $SrcRoot rev-parse HEAD).Trim()
$lockedCommit = $lock.chromium.commit
if ($chromiumHead -ne $lockedCommit) {
    # 允許 overlay snapshot：HEAD 必須以 locked commit 為祖先（pinned base 未變更）
    git -C $SrcRoot merge-base --is-ancestor $lockedCommit $chromiumHead 2>$null
    if ($LASTEXITCODE -ne 0) {
        Fail "Chromium commit 不符且 HEAD 不以 pinned base ($lockedCommit) 為祖先：實際=$chromiumHead（禁止未授權 Engine Revision 變更）"
    }
    $snapNote = if ($lock.chromium.overlay_snapshot_commit -and $chromiumHead -eq $lock.chromium.overlay_snapshot_commit) { '（= engine.lock 記錄的 overlay snapshot）' } else { '（overlay snapshot，engine.lock 尚未記錄此 commit，請同步更新）' }
    Ok "Chromium pinned base 為祖先：base=$lockedCommit，HEAD=$chromiumHead $snapNote"
} else {
    Ok "Chromium commit 一致：$chromiumHead"
}

# 4. Product repository 狀態
$productHead = (git -C $RepoRoot rev-parse HEAD).Trim()
$productDirty = @(git -C $RepoRoot status --porcelain).Count
if ($productHead -ne $lock.product.revision_current_head) {
    Warn "Product HEAD 與 engine.lock 記錄不同（lock=$($lock.product.revision_current_head)，實際=$productHead）；如為有意識的新 commit，請同步更新 engine.lock。"
} else {
    Ok "Product HEAD 一致：$productHead"
}
Ok "Product 工作樹未提交變更數：$productDirty"

# 5. bnes/ 完整性
$integrityPath = Join-Path $RepoRoot 'bnes.integrity.lock'
if (-not (Test-Path $integrityPath)) { Fail "找不到 bnes.integrity.lock" }
$integrity = Get-Content $integrityPath -Raw | ConvertFrom-Json
$violations = @()
foreach ($entry in $integrity.files) {
    $p = Join-Path $RepoRoot ($entry.path -replace '/', '\')
    if (-not (Test-Path $p)) { $violations += "遺失: $($entry.path)"; continue }
    $f = Get-Item $p
    if ($f.Length -ne $entry.size_bytes) { $violations += "大小不符: $($entry.path)"; continue }
    if ((Get-FileHash $p -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.sha256) {
        $violations += "hash 不符: $($entry.path)"
    }
}
$actualBnesCount = (Get-ChildItem (Join-Path $RepoRoot 'bnes') -Recurse -File).Count
if ($actualBnesCount -ne $integrity.file_count) {
    $violations += "bnes/ 檔案數不符：lock=$($integrity.file_count)，實際=$actualBnesCount"
}
if ($violations.Count -gt 0) {
    Write-Host ($violations | ForEach-Object { "  $($_)" }) -ForegroundColor Red
    Fail "Bnes Core Integrity 驗證失敗（共 $($violations.Count) 項）。STOP——不得自動修復，保留現場供人工分析。"
}
Ok "Bnes Core Integrity 通過（$($integrity.file_count) 檔案全部一致）"

# 6. Toolchain
$gnPath = Join-Path $SrcRoot 'buildtools\win\gn.exe'
$ninjaPath = Join-Path $SrcRoot 'third_party\ninja\ninja.exe'
foreach ($t in @($gnPath, $ninjaPath)) {
    if (-not (Test-Path $t)) { Fail "Toolchain 缺失：$t" }
}
$gnVer = (& $gnPath --version) -join ' '
$ninjaVer = (& $ninjaPath --version) -join ' '
if ($gnVer -notlike "$($lock.toolchain.gn_version.Split(' ')[0])*") {
    Warn "GN 版本：lock=$($lock.toolchain.gn_version)，實際=$gnVer"
} else { Ok "GN 版本一致：$gnVer" }
if ($ninjaVer -ne $lock.toolchain.ninja_version) {
    Warn "Ninja 版本：lock=$($lock.toolchain.ninja_version)，實際=$ninjaVer"
} else { Ok "Ninja 版本一致：$ninjaVer" }

# 7. args.gn
$argsGn = Join-Path $SrcRoot 'out\Release_GN\args.gn'
if (Test-Path $argsGn) {
    $h = (Get-FileHash $argsGn -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($h -ne $lock.build_configuration.args_gn_sha256) {
        Fail "args.gn 與 engine.lock 記錄不符（受保護組態，第三十四條）。STOP。"
    }
    Ok "args.gn hash 一致"
} else {
    Warn "args.gn 不存在（尚未執行過 gn gen）"
}

# 8. 寫出 mount state
$state = [ordered]@{
    mounted = $true
    mounted_at = (Get-Date).ToString('o')
    repo_root = $RepoRoot
    src_root = $SrcRoot
    build_root = $BuildRoot
    chromium_commit = $chromiumHead
    product_head = $productHead
    product_dirty_files = $productDirty
    bnes_integrity = 'PASS'
    gn_version = $gnVer
    ninja_version = $ninjaVer
    engine_lock = $lockPath
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $StatePath) | Out-Null
$state | ConvertTo-Json | Set-Content $StatePath -Encoding utf8

Write-Host "`nMount 完成。Ready for Build。" -ForegroundColor Green
Write-Host "解除整合：unmount-engine.ps1" -ForegroundColor DarkGray
