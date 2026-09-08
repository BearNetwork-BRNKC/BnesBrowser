<#
.SYNOPSIS
建置鎖版之本機 BnesBrowser Windows 安裝程式。

.DESCRIPTION
本進入點純粹以本機建置為導向。絕不執行 gclient、pnpm sync、
hooks、補丁投影、原始碼複製、原始碼刪除或自動修補。
既有的簽出樹即為建置樹，且 bnes\ 在建置前後皆會進行嚴格驗證。

範例：
pwsh -NoProfile -ExecutionPolicy Bypass -File 'E:\BnesBrowser-build\src\BnesBrowser\run_remaining.ps1' -SkipGnGen

#>
[CmdletBinding()]
param(
    [switch]$VerifyOnly,
    [switch]$SkipGnGen,
    [ValidateRange(1, 256)]
    [int]$Jobs = $(if ($env:NINJA_JOBS) { [int]$env:NINJA_JOBS } else { 12 })
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

# 從鎖定的本機 Chromium 樹建置；絕不重新取得或更新外部工具鏈。
$env:DEPOT_TOOLS_WIN_TOOLCHAIN = '0'
$env:DEPOT_TOOLS_UPDATE = '0'
$env:PYTHONUTF8 = '1'

$ProjectRoot = $PSScriptRoot
$SourceRoot = Split-Path -Parent $ProjectRoot
$BuildRoot = Split-Path -Parent $SourceRoot

# 設定 PYTHONPATH 包含 BnesBrowser/script 與 brave/script，供 Python 工具鏈載入 brave_chromium_utils
$pythonPaths = @(
    (Join-Path $ProjectRoot 'script'),
    (Join-Path $SourceRoot 'brave\script')
)
if ($env:PYTHONPATH) {
    $env:PYTHONPATH = ($pythonPaths -join [System.IO.Path]::PathSeparator) + [System.IO.Path]::PathSeparator + $env:PYTHONPATH
} else {
    $env:PYTHONPATH = $pythonPaths -join [System.IO.Path]::PathSeparator
}
$BnesRoot = Join-Path $ProjectRoot 'bnes'
$OutName = 'Release_GN'
$OutDir = Join-Path $SourceRoot "out\$OutName"
$ArgsGn = Join-Path $OutDir 'args.gn'
$StateDir = Join-Path $BuildRoot '.bnes-build'
$LogPath = Join-Path $StateDir 'build.log'
$ManifestPath = Join-Path $StateDir 'last-build.json'
$SetupPath = Join-Path $BuildRoot 'BnesBrowser_setup.exe'
$ExpectedChromiumVersion = '152.0.7977.64'

function Fail([string]$Message) {
    throw "BNES 建置失敗: $Message"
}

function Write-Stage([string]$Message) {
    Write-Host "`n=== $Message ===" -ForegroundColor Cyan
}

function Get-TreeHash([string]$Root) {
    if (-not (Test-Path -LiteralPath $Root -PathType Container)) {
        Fail "目錄不存在：$Root"
    }

    $prefix = $Root.TrimEnd('\') + '\'
    $lines = foreach ($item in Get-ChildItem -LiteralPath $Root -File -Recurse -Force) {
        $relative = $item.FullName.Substring($prefix.Length).Replace('\', '/')
        "$relative`t$($item.Length)`t$((Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash)"
    }
    $bytes = [Text.Encoding]::UTF8.GetBytes(($lines | Sort-Object) -join "`n")
    ([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes))).ToLowerInvariant()
}

function Assert-PinnedBaseline {
    $packageJson = Join-Path $ProjectRoot 'package.json'
    if (-not (Test-Path -LiteralPath $packageJson -PathType Leaf)) {
        Fail "找不到 package.json：$packageJson"
    }
    try {
        $package = Get-Content -LiteralPath $packageJson -Raw | ConvertFrom-Json
        $actual = [string]$package.config.projects.chrome.tag
    }
    catch {
        Fail "無法讀取 Chromium 基準版本：$($_.Exception.Message)"
    }
    if ($actual -ne $ExpectedChromiumVersion) {
        Fail "Chromium 基準版本應為 $ExpectedChromiumVersion，目前為 '$actual'；禁止自行升級。"
    }
}

function Find-Tool([string[]]$Candidates, [string]$Name) {
    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            return $candidate
        }
    }
    Fail "找不到 $Name。請確認本機 Chromium 152 工具鏈已就緒。"
}

function Invoke-Logged([string]$File, [string[]]$Arguments, [string]$Description) {
    Write-Host "> $File $($Arguments -join ' ')" -ForegroundColor DarkGray
    & $File @Arguments 2>&1 | Tee-Object -FilePath $LogPath -Append
    if ($LASTEXITCODE -ne 0) {
        Fail "$Description 執行失敗，EXIT=$LASTEXITCODE。詳細日誌：$LogPath"
    }
}

function Find-Installer {
    $candidates = @(
        (Join-Path $OutDir 'BnesBrowser_setup.exe'),
        (Join-Path $OutDir 'brave_installer.exe'),
        (Join-Path $OutDir 'mini_installer.exe'),
        (Join-Path $OutDir 'chrome_installer.exe')
    )
    $distDir = Join-Path $OutDir 'dist'
    if (Test-Path -LiteralPath $distDir -PathType Container) {
        $candidates += Get-ChildItem -LiteralPath $distDir -File -Filter '*.exe' |
            Where-Object { $_.Name -match '(?i)(setup|installer|mini_installer)' } |
            ForEach-Object FullName
    }
    foreach ($candidate in $candidates) {
        if (-not $candidate -or -not (Test-Path -LiteralPath $candidate -PathType Leaf)) { continue }
        $file = Get-Item -LiteralPath $candidate
        if ($file.Length -lt 1024) { continue }
        $header = Get-Content -LiteralPath $candidate -Encoding Byte -TotalCount 2
        if ($header.Count -eq 2 -and $header[0] -eq 0x4d -and $header[1] -eq 0x5a) { return $file }
    }
    Fail 'create_dist 完成後找不到有效的 Windows installer (PE 格式)。'
}

try {
    New-Item -ItemType Directory -Force -Path $StateDir | Out-Null
    Set-Content -LiteralPath $LogPath -Encoding utf8 -Value "BNES 本機建置開始: $(Get-Date -Format o)"

    Write-Stage '驗證鎖定建置樹與基準線'
    if (-not (Test-Path -LiteralPath $BnesRoot -PathType Container)) {
        Fail "BNES 受保護原始碼目錄不存在：$BnesRoot"
    }
    Assert-PinnedBaseline
    $bnesHashBefore = Get-TreeHash $BnesRoot
    Write-Host "Chromium baseline : $ExpectedChromiumVersion"
    Write-Host "BNES source hash  : $bnesHashBefore"

    if ($VerifyOnly) {
        Write-Host '驗證完成！不執行 GN 與 Ninja。' -ForegroundColor Green
        exit 0
    }
    if (-not (Test-Path -LiteralPath $ArgsGn -PathType Leaf)) {
        Fail "找不到 $ArgsGn；請先建立已審核的 Release GN 組態。腳本不會自動重寫 args.gn。"
    }

    $gn = Find-Tool @(
        (Join-Path $SourceRoot 'buildtools\win\gn.exe'),
        (Join-Path $SourceRoot 'third_party\depot_tools\gn.exe')
    ) 'gn.exe'
    $ninja = Find-Tool @(
        (Join-Path $SourceRoot 'third_party\ninja\ninja.exe'),
        (Join-Path $SourceRoot 'third_party\depot_tools\autoninja.bat'),
        (Join-Path $ProjectRoot 'vendor\depot_tools\autoninja.bat')
    ) 'ninja/autoninja'

    foreach ($name in 'CL', '_CL_', 'CFLAGS', 'CXXFLAGS', 'CC', 'CXX') {
        Remove-Item "Env:$name" -ErrorAction SilentlyContinue
    }

    Push-Location $SourceRoot
    try {
        if (-not $SkipGnGen) {
            Write-Stage 'GN 產製 (保留既有 args.gn)'
            Invoke-Logged $gn @('gen', "out/$OutName") 'GN generation'
        }
        elseif (-not (Test-Path -LiteralPath (Join-Path $OutDir 'build.ninja') -PathType Leaf)) {
            Fail '已指定 -SkipGnGen，但 build.ninja 不存在。'
        }
        Write-Stage "Ninja 執行 create_dist (並行數: $Jobs)"
        Invoke-Logged $ninja @('-C', "out/$OutName", 'create_dist', "-j$Jobs") 'Ninja create_dist'
    }
    finally {
        Pop-Location
    }

    $bnesHashAfter = Get-TreeHash $BnesRoot
    if ($bnesHashAfter -ne $bnesHashBefore) {
        Fail "建置過程導致 bnes\ 原始碼遭到篡改！建置前: $bnesHashBefore，建置後: $bnesHashAfter"
    }

    Write-Stage '驗證並提升安裝程式'
    $installer = Find-Installer
    Copy-Item -LiteralPath $installer.FullName -Destination $SetupPath -Force
    $sourceHash = (Get-FileHash -LiteralPath $installer.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    $setupHash = (Get-FileHash -LiteralPath $SetupPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($sourceHash -ne $setupHash) { Fail '安裝程式複製後 SHA256 雜湊不一致。' }

    [ordered]@{
        status = 'SUCCESS'; timestamp = (Get-Date).ToString('o')
        chromium_baseline = $ExpectedChromiumVersion; bnes_sha256 = $bnesHashAfter
        source_artifact = $installer.FullName; setup_artifact = $SetupPath
        size_bytes = (Get-Item -LiteralPath $SetupPath).Length; sha256 = $setupHash
    } | ConvertTo-Json | Set-Content -LiteralPath $ManifestPath -Encoding utf8

    Write-Host "建置成功：$SetupPath" -ForegroundColor Green
    Write-Host "SHA256：$setupHash" -ForegroundColor Green
}
catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}
