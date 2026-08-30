# ============================================================
# BNES Browser Transactional Safe Build System
#
# Purpose:
#   Prevent upstream Brave / Chromium changes from directly
#   corrupting the BNES downstream build tree.
#
# Execution:
#
#   pwsh -NoProfile -ExecutionPolicy Bypass `
#       -File S:\Ai_Agent\BNES\BnesBrowser\run_remaining.ps1 `
#       -SafeBuild
#
# Optional:
#
#   -Init
#       Run pnpm run init before build.
#
#   -AllowUpstreamSync
#       Explicitly allow ONE upstream Brave/Chromium gclient sync.
#       BNES is now independent from Brave; the only acknowledged upstream
#       is https://github.com/BearNetwork-BRNKC/BnesBrowser.git.
#       Without this switch, -Init is REFUSED and will not sync with Brave.
#
#   -MapOnly
#       Perform BNES projection only and stop before build.
#
#   -SafeBuild
#       Enable transactional isolation mode.
#

#
#   -VerifyOnly
#       Verify current boundary / manifest / protected overlays
#       without performing a build.
#
#   -ForceRecovery
#       Allow recovery when transactional state is incomplete.
#
# Environment:
#
#   INIT_CHROMIUM=1
#   SKIP_SYNC=1
#   SKIP_HOOKS=1
#   SKIP_GN=1
#   SKIP_BRANDING=1
#   SKIP_BNES_GUARD=1
#   SAFE_DELETE=0
#   NINJA_JOBS=12
#
# Safety model:
#
#   1. BNES canonical source is authoritative.
#   2. Upstream source is never treated as BNES canonical source.
#   3. BNES projection is generated into src\brave.
#   4. Upstream changes are fingerprinted.
#   5. Protected BNES overlay paths are validated before build.
#   6. GN configuration is validated before build.
#   7. Build failures never mutate BNES canonical source.
#   8. Failed transactional builds can be rolled back.
#   9. Last-known-good state is recorded only after a complete
#      successful build and artifact verification.
#  10. Semantic conflicts are NEVER auto-resolved.
#
# Important:
#
#   This system intentionally refuses to guess when upstream
#   changes cross a BNES ownership boundary.
#
# pwsh -NoProfile -ExecutionPolicy Bypass ` -File S:\Ai_Agent\BNES\BnesBrowser\run_remaining.ps1 `-SafeBuild
#
# ============================================================

[CmdletBinding()]
param(
    [switch]$MapOnly,
    [switch]$Init,
    [switch]$AllowUpstreamSync,
    [switch]$SafeBuild,
    [switch]$VerifyOnly,
    [switch]$ForceRecovery
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ============================================================
# GLOBAL PATHS
# ============================================================

$BuildRoot = 'E:\BnesBrowser-build'
$SrcDir = Join-Path $BuildRoot 'src'
$BraveDir = Join-Path $SrcDir 'brave'
$OutDir = Join-Path $SrcDir 'out\Release_GN'
$OutName = 'Release_GN'
$SetupName = 'BnesBrowser_setup.exe'
$SetupPath = Join-Path $BuildRoot $SetupName

$BnesCore = 'S:\Ai_Agent\BNES\BnesBrowser'

$BnesStateDir = Join-Path $BuildRoot '.bnes'
$ManifestRoot = Join-Path $BnesStateDir 'manifests'
$LogRoot = Join-Path $BnesStateDir 'logs'
$LockFile = Join-Path $BnesStateDir 'build.lock'
$TransactionFile = Join-Path $BnesStateDir 'transaction.json'
$LastGoodFile = Join-Path $BnesStateDir 'last-good.json'
$BoundaryFile = Join-Path $BnesStateDir 'boundary.json'

$HooksLog = Join-Path $BuildRoot 'hooks3.log'
$RedirectLog = Join-Path $BuildRoot 'redirect_cc.log'
$GnLog = Join-Path $BuildRoot 'gn_gen.log'
$BrandLog = Join-Path $BuildRoot 'branding.log'
$NinjaLog = Join-Path $BuildRoot 'ninja.log'

# ============================================================
# BUILD MODES
# ============================================================

$TransactionalMode = $SafeBuild -or
    (Get-Item Env:BNES_SAFE_BUILD -ErrorAction SilentlyContinue)

if ($TransactionalMode) {
    $env:BNES_SAFE_BUILD = '1'
}

# ============================================================
# ENVIRONMENT HELPERS
# ============================================================

function Get-EnvFlag {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $value = [Environment]::GetEnvironmentVariable($Name)

    if ($null -eq $value) {
        return $false
    }

    return (
        $value -eq '1' -or
        $value -eq 'true' -or
        $value -eq 'TRUE' -or
        $value -eq 'True'
    )
}

function Write-Stage {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    Write-Host ''
    Write-Host '============================================================' -ForegroundColor Cyan
    Write-Host " $Message" -ForegroundColor Cyan
    Write-Host '============================================================' -ForegroundColor Cyan
}

function Write-Info {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    Write-Host "[INFO] $Message" -ForegroundColor Gray
}

function Write-Ok {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    Write-Host "[OK] $Message" -ForegroundColor Green
}

function Write-Warn {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    Write-Host "[WARN] $Message" -ForegroundColor Yellow
}

function Write-Fail {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    Write-Host "[FAIL] $Message" -ForegroundColor Red
}

function New-Directory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        New-Item -ItemType Directory -Force -Path $Path | Out-Null
    }
}

# ============================================================
# INITIALIZE STATE DIRECTORIES
# ============================================================

New-Directory -Path $BnesStateDir
New-Directory -Path $ManifestRoot
New-Directory -Path $LogRoot

# ============================================================
# LOCKING
#
# Prevent two builds from modifying the same build tree.
# ============================================================

function New-BnesBuildLock {
    if (Test-Path -LiteralPath $LockFile) {
        if ($ForceRecovery) {
            Write-Warn "發現既有 build.lock，因指定 -ForceRecovery，移除舊 lock。"
            Remove-Item -LiteralPath $LockFile -Force
        }
        else {
            throw @"
BNES BUILD LOCK 已存在：

$LockFile

這表示可能仍有另一個 build transaction 正在執行，
或者上一輪 build 異常終止。

請確認沒有其他 build 正在執行後重新執行。

系統拒絕直接覆寫現有 transaction。
"@
        }
    }

    @{
        pid = $PID
        timestamp = (Get-Date).ToString('o')
        computer = $env:COMPUTERNAME
        user = $env:USERNAME
    } |
        ConvertTo-Json -Depth 10 |
        Set-Content -LiteralPath $LockFile -Encoding UTF8

    Write-Ok "BUILD LOCK 已建立。"
}

function Remove-BnesBuildLock {
    if (Test-Path -LiteralPath $LockFile) {
        Remove-Item -LiteralPath $LockFile -Force -ErrorAction SilentlyContinue
    }
}

# ============================================================
# HASH HELPERS
# ============================================================

function Get-FileSha256 {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $null
    }

    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

# ============================================================
# CANONICAL HASH IGNORE LIST
#
# Directories that must NOT participate in the BNES canonical
# source integrity hash. These are volatile VCS / runtime
# metadata directories. Their contents change frequently
# during normal development (git fetch, commit, etc.) and
# have nothing to do with BNES source code identity.
# ============================================================

$canonicalHashIgnore = @(
    '.git',
    '.github',
    '.agents',
    '.claude',
    '.gemini',
    'node_modules',
    'node-win-x64',
    'wintun',
    'win_build_output',
    '.patchinfo',
    '__pycache__'
)

function Get-DirectoryManifest {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root,

        [string[]]$ExcludeTopDirs = $canonicalHashIgnore
    )

    $result = [ordered]@{}

    if (-not (Test-Path -LiteralPath $Root -PathType Container)) {
        return $result
    }

    $rootNormalized = $Root.TrimEnd('\')

    Get-ChildItem -LiteralPath $Root -Recurse -File -Force -ErrorAction SilentlyContinue |
        ForEach-Object {
            $relative = $_.FullName.Substring($rootNormalized.Length).TrimStart('\')

            # 排除任何路徑片段中包含忽略目錄名稱的檔案
            $parts = $relative -split '\\'
            foreach ($part in $parts) {
                if ($ExcludeTopDirs -contains $part) {
                    return
                }
            }

            $relative = $relative -replace '\\', '/'

            $result[$relative] = @{
                size = $_.Length
                sha256 = (Get-FileSha256 -Path $_.FullName)
            }
        }

    return $result
}

function Get-DirectoryManifestHash {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root
    )

    $manifest = Get-DirectoryManifest -Root $Root

    $json = $manifest |
        ConvertTo-Json -Depth 20 -Compress

    $bytes = [System.Text.Encoding]::UTF8.GetBytes($json)
    $sha = [System.Security.Cryptography.SHA256]::Create()

    try {
        return (
            $sha.ComputeHash($bytes) |
                ForEach-Object { $_.ToString('x2') }
        ) -join ''
    }
    finally {
        $sha.Dispose()
    }
}

# ============================================================
# GIT STATE
#
# We record upstream repository state but never assume that
# git state alone proves semantic compatibility.
# ============================================================

function Get-GitState {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root
    )

    $result = [ordered]@{
        exists = $false
        is_git = $false
        branch = $null
        commit = $null
        status = @()
    }

    if (-not (Test-Path -LiteralPath $Root -PathType Container)) {
        return $result
    }

    $gitDir = Join-Path $Root '.git'

    if (-not (Test-Path -LiteralPath $gitDir)) {
        return $result
    }

    $result.exists = $true
    $result.is_git = $true

    Push-Location $Root

    try {
        $branch = git branch --show-current 2>$null
        $commit = git rev-parse HEAD 2>$null
        $status = git status --porcelain 2>$null

        if ($LASTEXITCODE -eq 0) {
            $result.branch = ($branch -join '').Trim()
            $result.commit = ($commit -join '').Trim()
            $result.status = @($status)
        }
    }
    finally {
        Pop-Location
    }

    return $result
}

# ============================================================
# SOURCE OWNERSHIP
#
# BNES canonical source:
#   S:\Ai_Agent\BNES\BnesBrowser
#
# Build projection:
#   E:\BnesBrowser-build\src\brave
#
# Upstream must never become canonical BNES source.
# ============================================================

function New-BoundaryManifest {
    $manifest = [ordered]@{
        version = 1
        created_at = (Get-Date).ToString('o')

        canonical = [ordered]@{
            path = $BnesCore
            role = 'BNES_CANONICAL'
            immutable_during_build = $true
        }

        projection = [ordered]@{
            path = $BraveDir
            role = 'BNES_BUILD_PROJECTION'
            disposable = $true
        }

        upstream = [ordered]@{
            chromium_root = $SrcDir
            brave_root = $BraveDir
            role = 'UPSTREAM_BUILD_INPUT'
        }

        policy = [ordered]@{
            automatic_semantic_merge = $false
            automatic_conflict_resolution = $false
            automatic_upstream_to_bnes = $false
            automatic_delete_outside_projection = $false
            build_on_boundary_violation = $false
            rollback_on_failure = $false
            last_good_only_after_verified_build = $true
        }
    }

    $manifest |
        ConvertTo-Json -Depth 20 |
        Set-Content -LiteralPath $BoundaryFile -Encoding UTF8
}

# ============================================================
# IGNORE RULES
# ============================================================

$syncIgnore = @(
    '.git',
    '.claude',
    '.github',
    '.agents',
    '.gemini',
    'node_modules',
    'node-win-x64',
    'wintun',
    'win_build_output',
    '.patchinfo',
    '__pycache__',
    # ------------------------------------------------------------------
    # BNES 分叉版修正 (2026-08-27):
    # vendor/ 下的項目是上游 Brave 委派的 git submodule /
    # 依賴目錄，並非 BNES canonical 投影內容。
    # 它們必須在 SAFEDEL/投影時被保留，否則每次 build 都會被
    # Remove-StaleBnesPaths 當成「過期項」整個清空，導致
    # ninja 找不到 vendor/web-discovery-project/package.json 等。
    # 再加入會誤刪的委派 submodule。
    # ------------------------------------------------------------------
    'web-discovery-project',
    'gn-project-generators',
    'bat-native-tweetnacl',
    'depot_tools',
    'omaha',
    # ------------------------------------------------------------------
    # BNES 分叉版修正 (2026-08-27):
    # 以下是 gclient / hooks 下載的真實編譯依賴。它們在 BnesBrowser
    # (canonical) 中只是空 stub（已被 .gitignore 排除，不會 commit），
    # 只有建構目錄 E:\...\src\brave 有真實內容可供 ninja 編譯。
    # 若不加進 syncIgnore，Sync-BnesTree 的 Remove-StaleBnesPaths(SAFEDEL)
    # 會把它們當成「E 有、S 缺」的過期項整個刪除，且 gclient 又因
    # hooks3.log 標記 EXIT=0 而被略過、不會拉回，導致編譯找不到依賴而失敗。
    # ------------------------------------------------------------------
    'argon2',
    'ethash',
    'bitcoin-core',
    'playlist_component',
    'bip39wally-core-native',
    'cryptography',
    'reclient_configs',
    'node',
    'node-win-x64',
    'wintun',
    'brave-vpn-wireguard-nt-dlls',
    'brave-vpn-wireguard-tunnel-dlls',
    'rust',
    'wasm',
    'opengrep',
    'ast-grep'
)

# ============================================================
# PROTECTED OVERLAY GUARD
# ============================================================

function Invoke-BnesProtectedOverlayGuard {
    param(
        [Parameter(Mandatory = $true)]
        [string]$TreeRoot,

        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    $checker = Join-Path $BnesCore 'bnes\scripts\check-protected-overlays.ps1'

    if (-not (Test-Path -LiteralPath $checker -PathType Leaf)) {
        throw "BNES protected overlay checker 不存在：$checker"
    }

    Write-Host ''
    Write-Host "[BNES GUARD] $Label" -ForegroundColor Yellow

    Push-Location $BnesCore

    try {
        if ($Label -like '*src\brave*') {
            & $checker -Root $TreeRoot -Repair
        }
        else {
            & $checker -Root $TreeRoot
        }

        $exitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }

    if ($exitCode -ne 0) {
        throw "BNES protected overlay guard 失敗：$Label EXIT=$exitCode"
    }

    Write-Ok "Protected overlay guard 通過：$Label"
}

# ============================================================
# RECURSIVE BNES COPY
# ============================================================

function Copy-BnesTree {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Source,

        [Parameter(Mandatory = $true)]
        [string]$Dest,

        [string[]]$IgnoreNames = @(),

        [System.Collections.Generic.HashSet[string]]$SourceRelSet,

        [string]$RootSource = $Source
    )

    if (-not (Test-Path -LiteralPath $Source -PathType Container)) {
        return
    }

    $skipNames = $IgnoreNames + @(
        '.patchinfo',
        '__pycache__'
    )

    if (Test-Path -LiteralPath $Dest) {
        if (-not (Test-Path -LiteralPath $Dest -PathType Container)) {
            Remove-Item -LiteralPath $Dest -Force
        }
    }

    if (-not (Test-Path -LiteralPath $Dest -PathType Container)) {
        New-Item -ItemType Directory -Force -Path $Dest | Out-Null
    }

    $children = Get-ChildItem -LiteralPath $Source -Force -ErrorAction Stop

    foreach ($item in $children) {

        if ($skipNames -contains $item.Name) {
            continue
        }

        $target = Join-Path $Dest $item.Name
        $parentDir = Split-Path -Parent $target

        if ($item.PSIsContainer) {

            if ($SourceRelSet) {
                $rel = (
                    $item.FullName.Substring(
                        $RootSource.TrimEnd('\').Length
                    ).TrimStart('\')
                ) -replace '\\', '/'

                [void]$SourceRelSet.Add($rel)
            }

            if (Test-Path -LiteralPath $target -PathType Leaf) {
                Remove-Item -LiteralPath $target -Force
            }

            if (-not (Test-Path -LiteralPath $target -PathType Container)) {
                if (-not (Test-Path -LiteralPath $parentDir -PathType Container)) {
                    New-Item -ItemType Directory -Force -Path $parentDir | Out-Null
                }

                New-Item -ItemType Directory -Force -Path $target | Out-Null
            }

            Copy-BnesTree `
                -Source $item.FullName `
                -Dest $target `
                -IgnoreNames $IgnoreNames `
                -SourceRelSet $SourceRelSet `
                -RootSource $RootSource
        }
        else {

            if ($SourceRelSet) {
                $rel = (
                    $item.FullName.Substring(
                        $RootSource.TrimEnd('\').Length
                    ).TrimStart('\')
                ) -replace '\\', '/'

                [void]$SourceRelSet.Add($rel)
            }

            if (Test-Path -LiteralPath $target -PathType Container) {
                Remove-Item -LiteralPath $target -Recurse -Force
            }

            if (-not (Test-Path -LiteralPath $parentDir -PathType Container)) {
                New-Item -ItemType Directory -Force -Path $parentDir | Out-Null
            }

            Copy-Item `
                -LiteralPath $item.FullName `
                -Destination $target `
                -Force
        }
    }
}

# ============================================================
# SAFE DELETE
# ============================================================

function Remove-StaleBnesPaths {
    param(
        [Parameter(Mandatory = $true)]
        [string]$TopDest,

        [Parameter(Mandatory = $true)]
        [System.Collections.Generic.HashSet[string]]$SourceRelSet
    )

    if (-not (Test-Path -LiteralPath $TopDest -PathType Container)) {
        return
    }

    $bravePrefix = $BraveDir.TrimEnd('\') + '\'

    if (-not ($TopDest -like ($bravePrefix + '*'))) {
        throw "SAFE DELETE boundary violation：$TopDest"
    }

    $staleFiles = [System.Collections.Generic.List[string]]::new()
    $staleDirs = [System.Collections.Generic.List[string]]::new()

    Get-ChildItem `
        -LiteralPath $TopDest `
        -Recurse `
        -Force `
        -ErrorAction SilentlyContinue |
        ForEach-Object {

            $item = $_
            $name = $item.Name

            if ($syncIgnore -contains $name) {
                return
            }

            $relativeFromTop = (
                $item.FullName.Substring(
                    $TopDest.TrimEnd('\').Length
                ).TrimStart('\')
            )

            $parentChain = $relativeFromTop -split '\\'

            if ($parentChain.Count -gt 1) {
                $parentChain = $parentChain[0..($parentChain.Count - 2)]

                foreach ($parent in $parentChain) {
                    if ($syncIgnore -contains $parent) {
                        return
                    }
                }
            }

            $rel = $relativeFromTop -replace '\\', '/'

            if ($SourceRelSet.Contains($rel)) {
                return
            }

            if ($item.PSIsContainer) {
                [void]$staleDirs.Add($item.FullName)
            }
            else {
                [void]$staleFiles.Add($item.FullName)
            }
        }

    foreach ($file in $staleFiles) {

        if (Test-Path -LiteralPath $file -PathType Leaf) {
            Remove-Item -LiteralPath $file -Force
            Write-Host "[SAFEDEL] 移除過期檔案：$file" -ForegroundColor DarkGray
        }
    }

    $staleDirs =
        $staleDirs |
        Sort-Object -Property @{
            Expression = {
                ($_ -split '\\').Count
            }
        } -Descending

    foreach ($dir in $staleDirs) {

        if (Test-Path -LiteralPath $dir -PathType Container) {

            $children =
                Get-ChildItem `
                    -LiteralPath $dir `
                    -Force `
                    -ErrorAction SilentlyContinue

            if (-not $children) {
                Remove-Item -LiteralPath $dir -Force
                Write-Host "[SAFEDEL] 移除空目錄：$dir" -ForegroundColor DarkGray
            }
        }
    }
}

# ============================================================
# BNES SOURCE IMMUTABILITY
#
# This is the most important protection.
#
# We record a canonical manifest BEFORE the build and compare it
# AFTER the build.
#
# If canonical BNES source changed:
#
#   STOP
#
# No automatic repair.
# No automatic overwrite.
# No guessing.
# ============================================================

function New-BnesCanonicalManifest {
    Write-Stage '建立 BNES canonical source manifest'

    if (-not (Test-Path -LiteralPath $BnesCore -PathType Container)) {
        throw "BNES canonical source 不存在：$BnesCore"
    }

    $manifest = [ordered]@{
        created_at = (Get-Date).ToString('o')
        root = $BnesCore
        git = Get-GitState -Root $BnesCore
        directory_sha256 = Get-DirectoryManifestHash -Root $BnesCore
        files = Get-DirectoryManifest -Root $BnesCore
    }

    $path = Join-Path `
        $ManifestRoot `
        'canonical-before.json'

    $manifest |
        ConvertTo-Json -Depth 30 |
        Set-Content -LiteralPath $path -Encoding UTF8

    Write-Ok "BNES canonical manifest 建立完成。"
    Write-Info "SHA256 = $($manifest.directory_sha256)"

    return $manifest
}

function Test-BnesCanonicalIntegrity {
    param(
        [Parameter(Mandatory = $true)]
        [hashtable]$BeforeManifest
    )

    Write-Stage '驗證 BNES canonical source 完整性'

    $currentHash =
        Get-DirectoryManifestHash -Root $BnesCore

    $beforeHash =
        $BeforeManifest.directory_sha256

    if ($currentHash -ne $beforeHash) {

        Write-Fail "BNES canonical source 發生變化。"

        Write-Host ''
        Write-Host "BEFORE : $beforeHash" -ForegroundColor Red
        Write-Host "AFTER  : $currentHash" -ForegroundColor Red
        Write-Host ''

        throw @"
FATAL BOUNDARY VIOLATION

BNES canonical source 在 transaction 期間發生修改。

Canonical:
$BnesCore

此系統拒絕自動修復。

原因：
BNES canonical source 是下游唯一權威來源。
任何 build / upstream / hook / script 都不得修改它。
"@
    }

    Write-Ok "BNES canonical source 未被污染。"
}

# ============================================================
# TRANSACTION STATE
# ============================================================

function Write-TransactionState {
    param(
        [Parameter(Mandatory = $true)]
        [hashtable]$State
    )

    $State |
        ConvertTo-Json -Depth 30 |
        Set-Content `
            -LiteralPath $TransactionFile `
            -Encoding UTF8
}

function Read-TransactionState {
    if (-not (Test-Path -LiteralPath $TransactionFile -PathType Leaf)) {
        return $null
    }

    return Get-Content `
        -LiteralPath $TransactionFile `
        -Raw |
        ConvertFrom-Json
}

# ============================================================
# ROLLBACK COMMAND
# ============================================================
#
# IMPORTANT:
#
# This is a projection operation.
#
# BNES canonical source -> build projection.
#
# Never:
#
# upstream -> BNES canonical source.
# ============================================================

function Sync-BnesTree {

    Write-Stage 'BNES canonical → build projection'

    if (-not (Test-Path -LiteralPath $BnesCore -PathType Container)) {
        throw "BNES canonical source 不存在：$BnesCore"
    }

    if (-not (Test-Path -LiteralPath $BraveDir -PathType Container)) {
        New-Directory -Path $BraveDir
    }

    $safeDelete = -not (Get-EnvFlag 'SAFE_DELETE')

    foreach (
        $directory in
        (
            Get-ChildItem `
                -LiteralPath $BnesCore `
                -Force |
            Where-Object {
                $_.PSIsContainer -and
                $syncIgnore -notcontains $_.Name
            }
        )
    ) {

        $sourceSet =
            [System.Collections.Generic.HashSet[string]]::new(
                [System.StringComparer]::OrdinalIgnoreCase
            )

        $destination =
            Join-Path `
                $BraveDir `
                $directory.Name

        Copy-BnesTree `
            -Source $directory.FullName `
            -Dest $destination `
            -IgnoreNames $syncIgnore `
            -SourceRelSet $sourceSet `
            -RootSource $directory.FullName

        if ($safeDelete) {
            Remove-StaleBnesPaths `
                -TopDest $destination `
                -SourceRelSet $sourceSet
        }

        Write-Ok "$($directory.Name) → src\brave\$($directory.Name)"
    }

    foreach (
        $file in
        (
            Get-ChildItem `
                -LiteralPath $BnesCore `
                -Force |
            Where-Object {
                -not $_.PSIsContainer -and
                $syncIgnore -notcontains $_.Name
            }
        )
    ) {

        $target =
            Join-Path `
                $BraveDir `
                $file.Name

        if (Test-Path -LiteralPath $target -PathType Container) {
            Remove-Item `
                -LiteralPath $target `
                -Recurse `
                -Force
        }

        Copy-Item `
            -LiteralPath $file.FullName `
            -Destination $target `
            -Force
    }

    Write-Ok 'BNES projection 完成。'
}

# ============================================================
# PROJECTION MANIFEST
# ============================================================

function New-ProjectionManifest {

    Write-Stage '建立 BNES projection manifest'

    $manifest = [ordered]@{
        created_at = (Get-Date).ToString('o')
        root = $BraveDir
        directory_sha256 =
            Get-DirectoryManifestHash -Root $BraveDir
        files =
            Get-DirectoryManifest -Root $BraveDir
    }

    $path =
        Join-Path `
            $ManifestRoot `
            'projection.json'

    $manifest |
        ConvertTo-Json -Depth 30 |
        Set-Content `
            -LiteralPath $path `
            -Encoding UTF8

    Write-Ok "Projection manifest 建立完成。"

    return $manifest
}

# ============================================================
# UPSTREAM STATE
# ============================================================

function New-UpstreamState {

    Write-Stage '建立 upstream state fingerprint'

    $state = [ordered]@{
        created_at = (Get-Date).ToString('o')

        chromium = @{
            root = $SrcDir
            git = Get-GitState -Root $SrcDir
        }

        brave = @{
            root = $BraveDir
            git = Get-GitState -Root $BraveDir
        }

        projection = @{
            hash =
                Get-DirectoryManifestHash -Root $BraveDir
        }
    }

    $path =
        Join-Path `
            $ManifestRoot `
            'upstream.json'

    $state |
        ConvertTo-Json -Depth 30 |
        Set-Content `
            -LiteralPath $path `
            -Encoding UTF8

    Write-Ok "Upstream fingerprint 完成。"

    return $state
}

# ============================================================
# OWNERSHIP VALIDATION
#
# The important principle:
#
# A file can be physically present in the Brave tree while still
# being logically owned by BNES.
#
# We therefore validate protected overlays independently of git.
# ============================================================

function Test-OwnershipBoundary {

    Write-Stage '驗證 BNES ownership boundary'

    $checker =
        Join-Path `
            $BnesCore `
            'bnes\scripts\check-protected-overlays.ps1'

    if (-not (Test-Path -LiteralPath $checker -PathType Leaf)) {
        throw "Ownership checker 不存在：$checker"
    }

    Push-Location $BnesCore

    try {

        & $checker `
            -Root $BnesCore

        $sourceExit = $LASTEXITCODE

        if ($sourceExit -ne 0) {
            throw "BNES canonical ownership 驗證失敗。EXIT=$sourceExit"
        }

        if (Test-Path -LiteralPath $BraveDir -PathType Container) {

            & $checker `
                -Root $BraveDir

            $projectionExit = $LASTEXITCODE

            if ($projectionExit -ne 0) {
                throw "BNES projection ownership 驗證失敗。EXIT=$projectionExit"
            }
        }
    }
    finally {
        Pop-Location
    }

    Write-Ok 'Ownership boundary 通過。'
}

# ============================================================
# GN CONFIGURATION VALIDATION
#
# Do not attempt to fix GN automatically.
#
# GN errors are semantic build graph errors and therefore must
# remain observable.
# ============================================================

function Test-GnInputs {

    Write-Stage '驗證 GN inputs'

    $argsGn =
        Join-Path `
            $OutDir `
            'args.gn'

    if (-not (Test-Path -LiteralPath $argsGn -PathType Leaf)) {
        Write-Warn "args.gn 尚不存在，GN validation 延後。"
        return
    }

    $content =
        Get-Content `
            -LiteralPath $argsGn `
            -Raw

    $required = @(
        'brave_defaults.gni',
        'branding_defaults.gni'
    )

    foreach ($token in $required) {

        if ($content -notmatch [regex]::Escape($token)) {
            throw "args.gn 缺少必要 import：$token"
        }
    }

    if ($content -notmatch 'target_cpu\s*=\s*"x64"') {
        throw 'args.gn target_cpu 不是 x64。'
    }

    if ($content -notmatch 'use_siso\s*=\s*true') {
        Write-Warn 'args.gn 沒有 use_siso=true。'
    }

    Write-Ok 'GN input validation 通過。'
}

# ============================================================
# ENVIRONMENT SETUP
# ============================================================

function Initialize-BnesEnvironment {

    Write-Stage '初始化 BNES build environment'

    $depotTools = @(
        Join-Path $BraveDir 'vendor\depot_tools'
        Join-Path $SrcDir 'third_party\depot_tools'
    ) |
        Where-Object {
            Test-Path -LiteralPath $_ -PathType Container
        }

    if ($depotTools.Count -gt 0) {
        $env:PATH =
            (($depotTools + $env:PATH) -join ';')
    }

    $env:DEPOT_TOOLS_WIN_TOOLCHAIN = '0'
    $env:GYP_MSVS_VERSION = '2022'

    $pythonPaths = @(
        Join-Path $BraveDir 'script'
        Join-Path $SrcDir 'tools\grit\grit\extern'
        Join-Path $BraveDir 'vendor\requests'
        Join-Path $BraveDir 'third_party\cryptography'
        Join-Path $BraveDir 'third_party\macholib'
        Join-Path $SrcDir 'build'
        Join-Path $SrcDir 'third_party\depot_tools'
        $env:PYTHONPATH
    ) |
        Where-Object {
            -not [string]::IsNullOrWhiteSpace($_)
        }

    $env:PYTHONPATH =
        $pythonPaths -join ';'

    $env:PYTHONUNBUFFERED = '1'
    $env:PYTHONUTF8 = '1'
    $env:GSUTIL_ENABLE_LUCI_AUTH = '0'
    $env:RUSTUP_HOME =
        Join-Path `
            $SrcDir `
            'third_party\rust-toolchain'

    Write-Ok 'Build environment 初始化完成。'
}

# ============================================================
# INIT CHROMIUM / BRAVE
# ============================================================

function Invoke-BnesRemotePolicy {
    param(
        [switch]$AttemptingSync
    )

    $upstreamUrl = [string]''

    $gitDir = Join-Path $BnesCore '.git'

    if (Test-Path -LiteralPath $gitDir -PathType Container) {
        Push-Location $BnesCore
        try {
            $upstreamUrl = ((git remote get-url upstream 2>$null) | Out-String).Trim()
        }
        catch {
            $upstreamUrl = [string]''
        }
        finally {
            Pop-Location
        }
    }

    if (-not $AttemptingSync) {
        if ([string]::IsNullOrWhiteSpace($upstreamUrl)) {
            Write-Info "未設定 git remote 'upstream'（符合 BNES 獨立政策，最快）。"
        }
        elseif ($upstreamUrl -like '*brave/*') {
            Write-Warn "偵測到 git remote 'upstream' 仍指向 Brave：$upstreamUrl"
            Write-Warn 'BNES 目前唯一認同的上游是 BearNetwork-BRNKC/BnesBrowser。'
            Write-Warn "建議移除：git -C $BnesCore remote remove upstream"
        }
        else {
            Write-Info "git remote 'upstream' = $upstreamUrl"
        }
        return
    }

    if ([string]::IsNullOrWhiteSpace($upstreamUrl)) {
        Write-Info '未設定上游 remote；允許 sync。'
        return
    }

    if ($upstreamUrl -like '*brave/*') {
        Write-Warn "偵測到 git remote 'upstream' 仍指向 Brave：$upstreamUrl"
        throw @"
UPSTREAM REMOTE POLICY VIOLATION

git remote 'upstream' 仍指向 Brave：
$upstreamUrl

BNES 已獨立於 Brave，唯一認同的上游是：
https://github.com/BearNetwork-BRNKC/BnesBrowser.git

此系統拒絕對 Brave 做 gclient sync（-Init）。
請先移除該 remote：
  git -C $BnesCore remote remove upstream
"@
    }

    Write-Info "上游 remote 非 Brave，允許 sync：$upstreamUrl"
}

function Invoke-UpstreamInit {

    Write-Stage '上游 Chromium / Brave toolchain initialization'

    if (-not $Init -and -not (Get-EnvFlag 'INIT_CHROMIUM')) {
        Write-Info '未要求 INIT_CHROMIUM，略過 upstream init。'
        return 0
    }

    Write-Stage 'BNES 上游同步政策稽核'

    Invoke-BnesRemotePolicy -AttemptingSync $true

    # --------------------------------------------------------
    # BNES 已獨立於 Brave。封裝打包不應對 Brave 做 gclient sync。
    # 需要 -Init 時，必須同時明確給 -AllowUpstreamSync 才會放行。
    # --------------------------------------------------------
    if (-not $AllowUpstreamSync) {
        throw @'
UPSTREAM SYNC REFUSED BY BNES POLICY

你要求執行 upstream init（pnpm run init = gclient sync）。
但 BNES 已與 Brave 分叉，唯一認同的上游是：
  https://github.com/BearNetwork-BRNKC/BnesBrowser.git

封裝打包不應對 Brave/Chromium 做同步。

若你真的需要一次上游 sync（例如 Chromium 大版本升級），
必須同時明確傳入：

  pwsh ...\run_remaining.ps1 -SafeBuild -Init -AllowUpstreamSync

並確認 git remote 'upstream' 不是 brave/brave-core。
'@
    }

    $lockFiles =
        Get-ChildItem `
            -Path $SrcDir `
            -Filter 'index.lock' `
            -Recurse `
            -Force `
            -ErrorAction SilentlyContinue

    foreach ($lock in $lockFiles) {
        Remove-Item `
            -LiteralPath $lock.FullName `
            -Force `
            -ErrorAction SilentlyContinue
    }

    Push-Location $BraveDir

    try {

        Write-Host '[INIT] pnpm run init ...' -ForegroundColor Yellow

        & pnpm run init 2>&1 |
            Tee-Object `
                -FilePath (
                    Join-Path `
                        $LogRoot `
                        'upstream-init.log'
                )

        $exitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }

    if ($exitCode -ne 0) {
        throw "pnpm run init 失敗。EXIT=$exitCode"
    }

    Write-Ok '上游初始化完成。'

    return 0
}

# ============================================================
# HOOKS
# ============================================================

function Invoke-GclientHooks {

    Write-Stage 'gclient runhooks'

    $skip =
        (Get-EnvFlag 'SKIP_HOOKS') -or
        (
            (Test-Path -LiteralPath $HooksLog -PathType Leaf) -and
            (
                Select-String `
                    -Path $HooksLog `
                    -Pattern '====GCLIENT_EXIT=0====' `
                    -Quiet `
                    -ErrorAction SilentlyContinue
            )
        )

    if ($skip) {
        Write-Warn 'gclient runhooks 已略過。'
        return 0
    }

    Push-Location $BuildRoot

    try {

        gclient runhooks 2>&1 |
            Tee-Object `
                -FilePath $HooksLog

        $exitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }

    Add-Content `
        -LiteralPath $HooksLog `
        -Value "====GCLIENT_EXIT=$exitCode===="

    if ($exitCode -ne 0) {
        throw "gclient runhooks 失敗。EXIT=$exitCode"
    }

    Write-Ok 'gclient runhooks 完成。'

    return 0
}

# ============================================================
# HOOK ARTIFACT VALIDATION
# ============================================================

function Test-HookArtifacts {

    Write-Stage '確認 hook 產物'

    $checks = @(
        @{
            Name = 'brave/build/version.gni'
            Path = Join-Path $BraveDir 'build\version.gni'
        },
        @{
            Name = 'build/util/LASTCHANGE'
            Path = Join-Path $SrcDir 'build\util\LASTCHANGE'
        },
        @{
            Name = 'brave/third_party/wintun/bin/x64/wintun.dll'
            Path = Join-Path $BraveDir 'third_party\wintun\bin\x64\wintun.dll'
        },
        @{
            Name = 'wireguard-nt'
            Path = Join-Path $BraveDir 'third_party\brave-vpn-wireguard-nt-dlls'
        },
        @{
            Name = 'wireguard-tunnel'
            Path = Join-Path $BraveDir 'third_party\brave-vpn-wireguard-tunnel-dlls'
        },
        @{
            Name = 'node-win-x64'
            Path = Join-Path $BraveDir 'third_party\node\node-win-x64'
        }
    )

    foreach ($check in $checks) {

        if (Test-Path -LiteralPath $check.Path) {
            Write-Ok $check.Name
        }
        else {
            Write-Warn "MISSING: $($check.Name)"
        }
    }
}

# ============================================================
# REDIRECT_CC
# ============================================================

function Invoke-RedirectCc {

    Write-Stage 'Brave redirect_cc'

    $redirectDir =
        Join-Path `
            $SrcDir `
            'out\redirect_cc'

    $redirectExe =
        Join-Path `
            $redirectDir `
            'redirect_cc.exe'

    $mainArgsGn =
        Join-Path `
            $OutDir `
            'args.gn'

    $useSiso = $false

    if (Test-Path -LiteralPath $mainArgsGn -PathType Leaf) {

        $useSiso =
            Select-String `
                -Path $mainArgsGn `
                -Pattern '^\s*use_siso\s*=\s*true' `
                -Quiet
    }

    if ($useSiso) {
        Write-Info 'use_siso=true，略過 redirect_cc。'
        return 0
    }

    if (Test-Path -LiteralPath $redirectExe -PathType Leaf) {
        Write-Ok 'redirect_cc.exe 已存在。'
        return 0
    }

    New-Directory -Path $redirectDir

    $redirectArgs =
        Join-Path `
            $redirectDir `
            'args.gn'

@'
import("//brave/tools/redirect_cc/args.gni")
use_remoteexec = false
use_siso = false
real_rewrapper = "E:/BnesBrowser-build/src/buildtools/reclient/rewrapper"
translate_genders = false
enable_pseudolocales = false
'@ |
        Set-Content `
            -LiteralPath $redirectArgs `
            -Encoding UTF8

    Push-Location $SrcDir

    try {

        gn gen out/redirect_cc 2>&1 |
            Tee-Object `
                -FilePath $RedirectLog

        $gnExit = $LASTEXITCODE

        if ($gnExit -ne 0) {
            throw "redirect_cc gn gen 失敗。EXIT=$gnExit"
        }

        ninja `
            -C out/redirect_cc `
            brave/tools/redirect_cc `
            -j12 2>&1 |
            Tee-Object `
                -FilePath $RedirectLog `
                -Append

        $ninjaExit = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }

    if ($ninjaExit -ne 0) {
        throw "redirect_cc build 失敗。EXIT=$ninjaExit"
    }

    if (-not (Test-Path -LiteralPath $redirectExe -PathType Leaf)) {
        throw "redirect_cc.exe 未產出。"
    }

    Write-Ok "redirect_cc 就緒：$redirectExe"

    return 0
}

# ============================================================
# ARGS.GN
# ============================================================

function New-ArgsGn {

    Write-Stage '建立 / 驗證 args.gn'

    New-Directory -Path $OutDir

    $argsGn =
        Join-Path `
            $OutDir `
            'args.gn'

    if (Test-Path -LiteralPath $argsGn -PathType Leaf) {

        Write-Info "args.gn 已存在：$argsGn"
        Get-Content -LiteralPath $argsGn |
            Write-Host

        return
    }

@'
# ============================================================
# BNES BnesBrowser Release build args
# ============================================================

import("//brave/build/args/brave_defaults.gni")
import("//brave/build/args/branding_defaults.gni")

is_official_build = false
is_debug = false
is_component_build = false
target_cpu = "x64"

# ------------------------------------------------------------
# BNES branding
# ------------------------------------------------------------

brave_product_name = "BnesBrowser"

# ------------------------------------------------------------
# General build
# ------------------------------------------------------------

enable_widevine = true
treat_warnings_as_errors = false
enable_pseudolocales = false

# ------------------------------------------------------------
# BNES intentionally removes Brave commercial/service modules.
# ------------------------------------------------------------

enable_brave_ads = false
enable_brave_news = false
enable_brave_rewards = false
enable_brave_wallet = false
# NOTE: `enable_brave_vpn` is a computed variable (v1 || v2) and CANNOT be
# overridden in args.gn. To actually strip the Brave VPN module (incl. the
# wireguard service & v2 apps), override BOTH architecture flags below.
enable_brave_vpn_v1 = false
enable_brave_vpn_v2 = false

# ------------------------------------------------------------
# BNES independence: these depend on Brave private services / auth
# that BNES does not (and should not) obtain.
# ------------------------------------------------------------

# Tor client is fetched at runtime from go-updater.brave.com via a component
# that requires Brave's private signed auth. BNES cannot obtain that, so Tor
# can never function -> strip the feature + UI entirely.
enable_tor = false

# AI Chat (Leo) requires Brave's hosted AI services. Stripped for full
# independence and to remove a non-functional entry point.
enable_ai_chat = false

# Keep the "Brave Origin" paid variant disabled. This also disables the
# Origin card in brave://settings/system and suppresses Purchased checks.
is_brave_origin_branded = false

# ------------------------------------------------------------
# Windows installer
# ------------------------------------------------------------

build_omaha = false
skip_signing = true

# ------------------------------------------------------------
# Local build
# ------------------------------------------------------------

use_remoteexec = false
use_siso = true

# ------------------------------------------------------------
# Symbols
# ------------------------------------------------------------

symbol_level = 0
blink_symbol_level = 0

# ------------------------------------------------------------
# PGO
# ------------------------------------------------------------

chrome_pgo_phase = 0

# ------------------------------------------------------------
# BNES does not use Brave remote services.
# Non-empty placeholders satisfy Brave assertions.
# ------------------------------------------------------------

brave_services_key = "BNES_PLACEHOLDER"
service_key_stt = "BNES_PLACEHOLDER"
service_key_search = "BNES_PLACEHOLDER"
service_key_aichat = "BNES_PLACEHOLDER"

uphold_production_api_url = "https://placeholder"
zebpay_production_api_url = "https://placeholder"

translate_genders = false
'@ |
        Set-Content `
            -LiteralPath $argsGn `
            -Encoding UTF8

    Write-Ok "args.gn 已建立：$argsGn"
}

# ============================================================
# GN GENERATION
# ============================================================

function Invoke-GnGen {

    Write-Stage 'GN generation'

    $gnLog =
        $GnLog

    $skip =
        (Get-EnvFlag 'SKIP_GN') -or
        (
            (Test-Path -LiteralPath (Join-Path $OutDir 'build.ninja')) -and
            (Test-Path -LiteralPath $gnLog -PathType Leaf) -and
            (
                Select-String `
                    -Path $gnLog `
                    -Pattern '====GN_GEN_EXIT=0====' `
                    -Quiet `
                    -ErrorAction SilentlyContinue
            )
        )

    if ($skip) {
        Write-Warn 'gn gen 已略過。'
        return 0
    }

    Push-Location $SrcDir

    try {

        gn gen "out/$OutName" 2>&1 |
            Tee-Object `
                -FilePath $gnLog

        $exitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }

    Add-Content `
        -LiteralPath $gnLog `
        -Value "====GN_GEN_EXIT=$exitCode===="

    if ($exitCode -ne 0) {
        throw "gn gen 失敗。EXIT=$exitCode"
    }

    if (-not (Test-Path -LiteralPath (Join-Path $OutDir 'build.ninja') -PathType Leaf)) {
        throw 'gn gen EXIT=0，但 build.ninja 不存在。'
    }

    Write-Ok 'GN generation 完成。'

    return 0
}

# ============================================================
# NODE DEPENDENCIES
# ============================================================

function Install-BnesNodeDependencies {

    Write-Stage '安裝 Brave Node.js 依賴'

    Push-Location $BraveDir

    try {

        $env:NODE_ENV = 'development'

        $packageJsonPath = Join-Path $BraveDir 'package.json'

        if (Test-Path -LiteralPath $packageJsonPath -PathType Leaf) {
            $pkg = Get-Content -LiteralPath $packageJsonPath -Raw | ConvertFrom-Json
            if ($pkg.devEngines.PSObject.Properties['packageManager']) {
                $pkg.devEngines.PSObject.Properties.Remove('packageManager')
                $pkg | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $packageJsonPath -Encoding UTF8
                Write-Warn '已移除 package.json 中的 devEngines.packageManager 欄位以避免 pnpm 解析失敗。'
            }
        }

        $nodeModulesPath = Join-Path $BraveDir 'node_modules'

        if (Test-Path -LiteralPath $nodeModulesPath -PathType Container) {
            Write-Warn '清理既有的 node_modules 以避免 pnpm store 損毀。'
            Remove-Item -LiteralPath $nodeModulesPath -Recurse -Force -ErrorAction SilentlyContinue
        }

        pnpm install --frozen-lockfile 2>&1 |
            Tee-Object `
                -FilePath (
                    Join-Path `
                        $BuildRoot `
                        'pnpm_install.log'
                )

        $exitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }

    if ($exitCode -ne 0) {
        throw "pnpm install 失敗。EXIT=$exitCode"
    }

    Write-Ok 'Brave Node.js dependencies 安裝完成。'

    return 0
}

# ============================================================
# BRANDING
# ============================================================

function Invoke-Branding {

    Write-Stage 'Brave branding preparation'

    $brandingFile =
        Join-Path `
            $SrcDir `
            'chrome\app\brave_strings.grd'

    $skip =
        (Get-EnvFlag 'SKIP_BRANDING') -and
        (Test-Path -LiteralPath $brandingFile -PathType Leaf)

    if ($skip) {
        Write-Warn 'branding 已略過。'
        return 0
    }

    Install-BnesNodeDependencies

    Push-Location $BraveDir

    try {

        node `
            ./build/commands/scripts/build.ts `
            --prepare_only `
            -C $OutName `
            --skip_signing 2>&1 |
            Tee-Object `
                -FilePath $BrandLog

        $exitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }

    Add-Content `
        -LiteralPath $BrandLog `
        -Value "====BRANDING_EXIT=$exitCode===="

    if ($exitCode -ne 0) {
        throw "branding preparation 失敗。EXIT=$exitCode"
    }

    if (-not (Test-Path -LiteralPath $brandingFile -PathType Leaf)) {
        throw "branding 成功返回，但 $brandingFile 不存在。"
    }

    Write-Ok 'Brave branding 完成。'

    return 0
}

# ============================================================
# WEB-DISCOVERY-PROJECT CORRUPTION GUARD
#
# web-discovery-project 是上游 Brave 委派的固定 git checkout，
# 只在 build tree（E:）真實存在，且位於 $syncIgnore，
# 並不會被 BNES canonical 投影 / SAFEDEL 碰觸。
#
# 但先前有「遞迴複製 / 快照還原」機制會在該目錄內造成損壞：
#   - modules/parser/*（含 package.json、index.js）被刪除
#   - 產生巢狀重複目錄：modules/modules/、configs/configs/、...
#
# 導致 ninja 的 web_discovery_project_resources 失敗：
#   Module not found: Can't resolve '@web-discovery-project/parser'
#
# 本函式在 ninja 前就地偵測並修復該 checkout。
# 只動可丟棄的 build tree，絕不碰 BNES canonical source。
# 對健康的工作樹是 no-op（安全、可重入）。
# ============================================================

function Repair-WebDiscoveryProject {

    $wdp = Join-Path $BraveDir 'vendor\web-discovery-project'

    if (-not (Test-Path -LiteralPath $wdp -PathType Container)) {
        return $false
    }

    $gitDir = Join-Path $wdp '.git'
    if (-not (Test-Path -LiteralPath $gitDir -PathType Container)) {
        Write-Info 'web-discovery-project 非 git checkout，略過損壞檢查。'
        return $false
    }

    # --------------------------------------------------------
    # DETECT
    # --------------------------------------------------------

    Push-Location $wdp
    try {
        $null = git status --porcelain 2>$null
        if ($LASTEXITCODE -ne 0) {
            Write-Warn "web-discovery-project git status 失敗（EXIT=$LASTEXITCODE），略過。"
            return $false
        }
    }
    finally {
        Pop-Location
    }

    $parserPkg = Join-Path $wdp 'modules\parser\package.json'
    $corrupt = -not (Test-Path -LiteralPath $parserPkg -PathType Leaf)

    if (-not $corrupt) {

        $nested =
            Get-ChildItem `
                -LiteralPath $wdp `
                -Directory `
                -Force `
                -ErrorAction SilentlyContinue |
            Where-Object {
                Test-Path `
                    -LiteralPath (Join-Path $_.FullName $_.Name) `
                    -PathType Container
            } |
            Select-Object -First 1

        if ($nested) {
            $corrupt = $true
        }
    }

    $link = Join-Path $wdp 'node_modules\@web-discovery-project\parser'

    if (-not $corrupt) {
        $corrupt = -not (
            Test-Path `
                -LiteralPath (Join-Path $link 'package.json') `
                -PathType Leaf
        )
    }

    if (-not $corrupt) {
        return $false
    }

    # --------------------------------------------------------
    # REPAIR (disposable build tree only)
    # --------------------------------------------------------

    Write-Warn '偵測到 web-discovery-project 工作樹損壞，開始自動修復。'
    Write-Info "  路徑：$wdp"

    Push-Location $wdp
    try {

        & git reset --hard HEAD 2>&1 |
            Out-Null

        if ($LASTEXITCODE -ne 0) {
            throw "git reset --hard 失敗。EXIT=$LASTEXITCODE"
        }

        # 只清除未追蹤檔/目錄；node_modules 已被 .gitignore 排除，
        # 因此既有的 webpack 依賴會被保留，僅重建需要的 link。
        & git clean -fd 2>&1 |
            Out-Null

        if ($LASTEXITCODE -ne 0) {
            throw "git clean -fd 失敗。EXIT=$LASTEXITCODE"
        }

        Write-Ok 'git reset / clean 完成。'

        if (-not (Test-Path -LiteralPath $parserPkg -PathType Leaf)) {
            throw "git reset 後 modules/parser/package.json 仍不存在：$parserPkg"
        }

        # 重建 npm workspace link: @web-discovery-project/parser -> modules/parser
        $scoped = Join-Path $wdp 'node_modules\@web-discovery-project'
        $target = Join-Path $wdp 'modules\parser'

        if (-not (Test-Path -LiteralPath $scoped -PathType Container)) {
            New-Item -ItemType Directory -Force -Path $scoped |
                Out-Null
        }

        if (Test-Path -LiteralPath $link) {
            Remove-Item `
                -LiteralPath $link `
                -Recurse `
                -Force `
                -ErrorAction SilentlyContinue
        }

        New-Item `
            -ItemType Junction `
            -Path $link `
            -Target $target |
            Out-Null

        if (-not (Test-Path -LiteralPath (Join-Path $link 'package.json') -PathType Leaf)) {
            throw 'parser junction 建立後 package.json 仍無法解析。'
        }
    }
    finally {
        Pop-Location
    }

    Write-Ok 'web-discovery-project 已自動修復。'

    return $true
}

# ============================================================
# NINJA RESOLUTION
# ============================================================
# ============================================================
# NINJA RESOLUTION
# ============================================================

function Resolve-Ninja {

    $candidates = @(
        Join-Path $SrcDir 'third_party\ninja\ninja.exe'
        Join-Path $SrcDir 'third_party\depot_tools\ninja.exe'
        Join-Path $SrcDir 'third_party\depot_tools\autoninja.bat'
        Join-Path $BraveDir 'vendor\depot_tools\ninja.exe'
        Join-Path $BraveDir 'vendor\depot_tools\autoninja.bat'
    )

    foreach ($candidate in $candidates) {

        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return $candidate
        }
    }

    $command =
        Get-Command `
            ninja `
            -ErrorAction SilentlyContinue

    if ($command) {
        return $command.Source
    }

    return $null
}

# ============================================================
# NINJA BUILD
# ============================================================

function Invoke-NinjaBuild {

    Write-Stage 'Ninja build'

    $jobs =
        if ($env:NINJA_JOBS) {
            $env:NINJA_JOBS
        }
        else {
            '12'
        }

    $clangCl =
        Join-Path `
            $SrcDir `
            'third_party\llvm-build\Release+Asserts\bin\clang-cl.exe'

    if (Test-Path -LiteralPath $clangCl -PathType Leaf) {

        $env:CC = $clangCl
        $env:CXX = $clangCl
        $env:CFLAGS = '/EHsc'
        $env:CXXFLAGS = '/EHsc'
    }

    Remove-Item `
        Env:CL `
        -ErrorAction SilentlyContinue

    Remove-Item `
        Env:_CL_ `
        -ErrorAction SilentlyContinue

    $ninjaCmd =
        Resolve-Ninja

    if (-not $ninjaCmd) {
        throw '找不到 ninja.exe / autoninja.bat。'
    }

    Write-Info "$ninjaCmd -C out/$OutName create_dist -j$jobs"

    Push-Location $SrcDir

    try {

        & $ninjaCmd `
            -C "out/$OutName" `
            create_dist `
            "-j$jobs" 2>&1 |
            Tee-Object `
                -FilePath $NinjaLog

        $exitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }

    Add-Content `
        -LiteralPath $NinjaLog `
        -Value "====NINJA_EXIT=$exitCode===="

    if ($exitCode -ne 0) {
        throw "Ninja build 失敗。EXIT=$exitCode"
    }

    Write-Ok 'Ninja build 完成。'

    return 0
}

# ============================================================
# INSTALLER DISCOVERY
# ============================================================

function Find-Installer {

    $candidates = @(
        Join-Path $OutDir 'brave_installer.exe'
        Join-Path $OutDir 'mini_installer.exe'
        Join-Path $OutDir 'chrome_installer.exe'
    )

    $distDir =
        Join-Path `
            $OutDir `
            'dist'

    if (Test-Path -LiteralPath $distDir -PathType Container) {

        $candidates += @(
            Get-ChildItem `
                -LiteralPath $distDir `
                -Filter '*Setup*.exe' `
                -ErrorAction SilentlyContinue |
                ForEach-Object {
                    $_.FullName
                }
        )

        $candidates += @(
            Get-ChildItem `
                -LiteralPath $distDir `
                -Filter '*installer*.exe' `
                -ErrorAction SilentlyContinue |
                ForEach-Object {
                    $_.FullName
                }
        )
    }

    return (
        $candidates |
            Where-Object {
                $_ -and
                (Test-Path -LiteralPath $_ -PathType Leaf)
            } |
            Select-Object -First 1
    )
}

# ============================================================
# ARTIFACT VERIFICATION
# ============================================================

function Test-BuildArtifact {

    Write-Stage 'Artifact verification'

    $installer =
        Find-Installer

    if (-not $installer) {
        throw 'Ninja 成功但找不到 installer artifact。'
    }

    $file =
        Get-Item `
            -LiteralPath $installer

    if ($file.Length -le 0) {
        throw "Installer artifact 為空：$installer"
    }

    $hash =
        Get-FileSha256 `
            -Path $installer

    Write-Info "Installer : $installer"
    Write-Info "Size      : $([math]::Round($file.Length / 1MB, 2)) MB"
    Write-Info "SHA256    : $hash"

    return @{
        path = $installer
        size = $file.Length
        sha256 = $hash
    }
}

# ============================================================
# PROMOTE ARTIFACT
#
# Only after build + verification.
# ============================================================

function Move-Artifact {
    param(
        [Parameter(Mandatory = $true)]
        [hashtable]$Artifact
    )

    Write-Stage 'Artifact promotion'

    New-Item `
        -ItemType Directory `
        -Force `
        -Path $BuildRoot |
        Out-Null

    Copy-Item `
        -LiteralPath $Artifact.path `
        -Destination $SetupPath `
        -Force

    $distDir =
        Join-Path `
            $OutDir `
            'dist'

    New-Directory -Path $distDir

    Copy-Item `
        -LiteralPath $Artifact.path `
        -Destination (
            Join-Path `
                $distDir `
                $SetupName
        ) `
        -Force

    $promoted =
        Get-Item `
            -LiteralPath $SetupPath

    $promotedHash =
        Get-FileSha256 `
            -Path $SetupPath

    if ($promotedHash -ne $Artifact.sha256) {
        throw @"
Artifact promotion integrity failure.

Original:
$($Artifact.sha256)

Promoted:
$promotedHash
"@
    }

    Write-Ok "Installer 已安全提升：$SetupPath"

    return @{
        path = $SetupPath
        size = $promoted.Length
        sha256 = $promotedHash
    }
}

# ============================================================
# LAST KNOWN GOOD
#
# IMPORTANT:
#
# This file is written ONLY after:
#
#   canonical integrity
#   ownership validation
#   GN success
#   branding success
#   Ninja success
#   artifact verification
#   artifact promotion
#
# have all succeeded.
# ============================================================

function Write-LastKnownGood {
    param(
        [Parameter(Mandatory = $true)]
        [hashtable]$Artifact,

        [Parameter(Mandatory = $true)]
        [hashtable]$CanonicalManifest,

        [Parameter(Mandatory = $true)]
        [hashtable]$UpstreamState,

        [Parameter(Mandatory = $true)]
        [hashtable]$ProjectionManifest,

        [Parameter(Mandatory = $true)]
        [string]$TransactionId
    )

    Write-Stage '更新 Last Known Good'

    $good = [ordered]@{
        version = 1
        status = 'SUCCESS'
        transaction_id = $TransactionId
        timestamp = (Get-Date).ToString('o')

        canonical = @{
            root = $BnesCore
            sha256 = $CanonicalManifest.directory_sha256
            git = $CanonicalManifest.git
        }

        upstream = $UpstreamState

        projection = @{
            root = $BraveDir
            sha256 = $ProjectionManifest.directory_sha256
        }

        artifact = $Artifact
    }

    $good |
        ConvertTo-Json -Depth 40 |
        Set-Content `
            -LiteralPath $LastGoodFile `
            -Encoding UTF8

    Write-Ok 'Last Known Good 已更新。'
}

# ============================================================
# TRANSACTION FAILURE HANDLER
# ============================================================

function Invoke-TransactionFailure {
    param(
        [Parameter(Mandatory = $true)]
        [System.Management.Automation.ErrorRecord]$ErrorRecord,

        [Parameter(Mandatory = $true)]
        [hashtable]$State,

        [Parameter(Mandatory = $true)]
        [hashtable]$CanonicalManifest
    )

    Write-Host ''
    Write-Host '############################################################' -ForegroundColor Red
    Write-Host '# BNES TRANSACTION FAILED' -ForegroundColor Red
    Write-Host '############################################################' -ForegroundColor Red
    Write-Host ''

    Write-Fail $ErrorRecord.Exception.Message

    $State.status = 'FAILED'
    $State.failed_at = (Get-Date).ToString('o')
    $State.error = $ErrorRecord.Exception.Message

    try {
        Write-TransactionState -State $State
    }
    catch {
        Write-Warn "無法寫入 transaction failure state：$($_.Exception.Message)"
    }

    try {
        Test-BnesCanonicalIntegrity `
            -BeforeManifest $CanonicalManifest
    }
    catch {
        Write-Host ''
        Write-Host '!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!' -ForegroundColor Red
        Write-Host 'CRITICAL: BNES CANONICAL SOURCE MAY HAVE BEEN MODIFIED' -ForegroundColor Red
        Write-Host '!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!' -ForegroundColor Red
        Write-Host ''
        Write-Host $_.Exception.Message -ForegroundColor Red
    }

    Write-Host ''
    Write-Host 'BNES SAFE BUILD POLICY:' -ForegroundColor Yellow
    Write-Host '  不自動修改 canonical source'
    Write-Host '  不自動解決 semantic conflict'
    Write-Host '  不自動猜測 GN dependency'
    Write-Host '  不自動把 upstream 修正寫回 BNES'
    Write-Host '  保留 logs / manifests'
    Write-Host ''

    return 1
}

# ============================================================
# VERIFY ONLY
# ============================================================

function Invoke-VerifyOnly {

    Write-Stage 'BNES VERIFY ONLY'

    if (-not (Test-Path -LiteralPath $BnesCore -PathType Container)) {
        throw "BNES canonical source 不存在：$BnesCore"
    }

    if (Test-Path -LiteralPath $BoundaryFile -PathType Leaf) {
        Write-Ok "Boundary manifest：$BoundaryFile"
    }
    else {
        New-BoundaryManifest
        Write-Ok "Boundary manifest 已建立：$BoundaryFile"
    }

    Invoke-BnesProtectedOverlayGuard `
        -TreeRoot $BnesCore `
        -Label 'source BnesBrowser'

    if (Test-Path -LiteralPath $BraveDir -PathType Container) {

        Invoke-BnesProtectedOverlayGuard `
            -TreeRoot $BraveDir `
            -Label 'mapped src\brave'
    }

    Test-OwnershipBoundary

    $canonical =
        New-BnesCanonicalManifest

    Test-BnesCanonicalIntegrity `
        -BeforeManifest $canonical

    Write-Stage 'VERIFY ONLY SUCCESS'

    Write-Ok '所有目前可驗證的 BNES boundary checks 通過。'
}

# ============================================================
# MAIN
# ============================================================

$transactionId =
    (
        Get-Date
    ).ToString('yyyyMMdd_HHmmss_fff')

$transactionState = @{
    version = 1
    transaction_id = $transactionId
    status = 'STARTING'
    started_at = (Get-Date).ToString('o')

    safe_build = $TransactionalMode

    build_root = $BuildRoot
    canonical_root = $BnesCore
    projection_root = $BraveDir

    stages = @{
        init = 'PENDING'
        sync = 'PENDING'
        hooks = 'PENDING'
        gn = 'PENDING'
        branding = 'PENDING'
        ninja = 'PENDING'
        artifact = 'PENDING'
        promotion = 'PENDING'
    }
}

try {

    # --------------------------------------------------------
    # BASIC PATH VALIDATION
    # --------------------------------------------------------

    Write-Stage 'BNES SAFE BUILD INITIALIZATION'

    Write-Info "BuildRoot   : $BuildRoot"
    Write-Info "SourceRoot  : $SrcDir"
    Write-Info "BNES Core   : $BnesCore"
    Write-Info "Projection  : $BraveDir"
    Write-Info "Safe Build  : $TransactionalMode"

    if (-not (Test-Path -LiteralPath $BnesCore -PathType Container)) {
        throw "BNES canonical source 不存在：$BnesCore"
    }

    if (-not (Test-Path -LiteralPath $SrcDir -PathType Container)) {
        throw "Chromium build source 不存在：$SrcDir"
    }

    New-BoundaryManifest

    Invoke-BnesRemotePolicy -AttemptingSync $false

    # --------------------------------------------------------
    # SPECIAL COMMANDS
    # --------------------------------------------------------

    if ($VerifyOnly) {

        New-BnesBuildLock

        try {
            Invoke-VerifyOnly
        }
        finally {
            Remove-BnesBuildLock
        }

        exit 0
    }

    # --------------------------------------------------------
    # LOCK
    # --------------------------------------------------------

    New-BnesBuildLock

    # --------------------------------------------------------
    # CANONICAL MANIFEST
    # --------------------------------------------------------

    $canonicalManifest =
        New-BnesCanonicalManifest

    # --------------------------------------------------------
    # ENVIRONMENT
    # --------------------------------------------------------

    Initialize-BnesEnvironment

    # --------------------------------------------------------
    # UPSTREAM STATE BEFORE INIT
    # --------------------------------------------------------

    $upstreamBefore =
        New-UpstreamState

    $transactionState.upstream_before =
        $upstreamBefore

    Write-TransactionState `
        -State $transactionState

    # --------------------------------------------------------
    # STEP 0.8
    # --------------------------------------------------------

    Invoke-UpstreamInit

    $transactionState.stages.init = 'SUCCESS'

    # --------------------------------------------------------
    # CRITICAL:
    #
    # Upstream init may modify Chromium / Brave.
    #
    # Re-check BNES canonical source immediately.
    # --------------------------------------------------------

    Test-BnesCanonicalIntegrity `
        -BeforeManifest $canonicalManifest

    # --------------------------------------------------------
    # UPSTREAM STATE AFTER INIT
    # --------------------------------------------------------

    $upstreamAfterInit =
        New-UpstreamState

    $transactionState.upstream_after_init =
        $upstreamAfterInit

    Write-TransactionState `
        -State $transactionState

    # --------------------------------------------------------
    # STEP 1
    # --------------------------------------------------------

    Invoke-GclientHooks

    $transactionState.stages.hooks = 'SUCCESS'

    Test-BnesCanonicalIntegrity `
        -BeforeManifest $canonicalManifest

    Write-TransactionState `
        -State $transactionState

    # --------------------------------------------------------
    # STEP 2
    # --------------------------------------------------------

    Test-HookArtifacts

    # --------------------------------------------------------
    # STEP 3:
    #
    # Generate BNES projection AFTER upstream synchronization.
    #
    # The projection is disposable.
    # The canonical source is not.
    # --------------------------------------------------------

    if (-not (Get-EnvFlag 'SKIP_SYNC')) {

        Sync-BnesTree

        $transactionState.stages.sync = 'SUCCESS'

        Test-BnesCanonicalIntegrity `
            -BeforeManifest $canonicalManifest
    }
    else {

        Write-Warn 'SKIP_SYNC=1：BNES projection sync 已略過。'

        $transactionState.stages.sync = 'SKIPPED'
    }

    # --------------------------------------------------------
    # PROTECTED OVERLAY GUARD
    # --------------------------------------------------------

    if (Get-EnvFlag 'SKIP_BNES_GUARD') {

        Write-Warn @'
SKIP_BNES_GUARD=1

BNES protected overlay validation 被略過。

正式 SafeBuild 不建議這樣做。
'@
    }
    else {

        Invoke-BnesProtectedOverlayGuard `
            -TreeRoot $BnesCore `
            -Label 'source BnesBrowser'

        Invoke-BnesProtectedOverlayGuard `
            -TreeRoot $BraveDir `
            -Label 'mapped src\brave'
    }

    # --------------------------------------------------------
    # OWNERSHIP
    # --------------------------------------------------------

    Test-OwnershipBoundary

    # --------------------------------------------------------
    # PROJECTION MANIFEST
    # --------------------------------------------------------

    $projectionManifest =
        New-ProjectionManifest

    $transactionState.projection =
        @{
            hash = $projectionManifest.directory_sha256
        }

    Write-TransactionState `
        -State $transactionState

    # --------------------------------------------------------
    # STEP 2.5
    # --------------------------------------------------------

    Invoke-RedirectCc

    # --------------------------------------------------------
    # STEP 3
    # --------------------------------------------------------

    New-ArgsGn

    Test-GnInputs

    Invoke-GnGen

    $transactionState.stages.gn = 'SUCCESS'

    # --------------------------------------------------------
    # CRITICAL:
    #
    # GN must not have modified BNES canonical source.
    # --------------------------------------------------------

    Test-BnesCanonicalIntegrity `
        -BeforeManifest $canonicalManifest

    Write-TransactionState `
        -State $transactionState

    # --------------------------------------------------------
    # STEP 3.5
    # --------------------------------------------------------

    Invoke-Branding

    $transactionState.stages.branding = 'SUCCESS'

    Test-BnesCanonicalIntegrity `
        -BeforeManifest $canonicalManifest

    Write-TransactionState `
        -State $transactionState

    # --------------------------------------------------------
    # MAP ONLY
    # --------------------------------------------------------

    if ($MapOnly) {

        Write-Stage 'MAP ONLY COMPLETE'

        Test-BnesCanonicalIntegrity `
            -BeforeManifest $canonicalManifest

        $transactionState.status = 'MAP_ONLY_SUCCESS'
        $transactionState.finished_at =
            (Get-Date).ToString('o')

        Write-TransactionState `
            -State $transactionState

        Write-Ok 'MapOnly 完成。'
        exit 0
    }

    # --------------------------------------------------------
    # FINAL PRE-BUILD BOUNDARY CHECK
    #
    # This is intentionally repeated immediately before Ninja.
    # --------------------------------------------------------

    Write-Stage 'FINAL PRE-BUILD BOUNDARY CHECK'

    Test-BnesCanonicalIntegrity `
        -BeforeManifest $canonicalManifest

    if (-not (Get-EnvFlag 'SKIP_BNES_GUARD')) {

        Invoke-BnesProtectedOverlayGuard `
            -TreeRoot $BnesCore `
            -Label 'source BnesBrowser FINAL'

        Invoke-BnesProtectedOverlayGuard `
            -TreeRoot $BraveDir `
            -Label 'mapped src\brave FINAL'
    }

    Test-OwnershipBoundary

    Test-GnInputs

    Write-Ok 'Final boundary check 通過。'
    Write-Host ''
    Write-Host 'BNES BUILD MAY PROCEED.' -ForegroundColor Green

    # --------------------------------------------------------
    # STEP 4
    # --------------------------------------------------------

    # web-discovery-project 損壞自動偵測/修復（僅限可丟棄 build tree；
    # 健康時為 no-op）。
    Repair-WebDiscoveryProject

    Invoke-NinjaBuild

    $transactionState.stages.ninja = 'SUCCESS'

    Test-BnesCanonicalIntegrity `
        -BeforeManifest $canonicalManifest

    Write-TransactionState `
        -State $transactionState

    # --------------------------------------------------------
    # ARTIFACT
    # --------------------------------------------------------

    $artifact =
        Test-BuildArtifact

    $transactionState.stages.artifact = 'SUCCESS'
    $transactionState.artifact = $artifact

    Write-TransactionState `
        -State $transactionState

    # --------------------------------------------------------
    # PROMOTION
    # --------------------------------------------------------

    $promoted =
        Move-Artifact `
            -Artifact $artifact

    $transactionState.stages.promotion = 'SUCCESS'
    $transactionState.promoted_artifact = $promoted

    Write-TransactionState `
        -State $transactionState

    # --------------------------------------------------------
    # FINAL CANONICAL CHECK
    # --------------------------------------------------------

    Test-BnesCanonicalIntegrity `
        -BeforeManifest $canonicalManifest

    # --------------------------------------------------------
    # FINAL PROTECTED OVERLAY CHECK
    # --------------------------------------------------------

    if (-not (Get-EnvFlag 'SKIP_BNES_GUARD')) {

        Invoke-BnesProtectedOverlayGuard `
            -TreeRoot $BnesCore `
            -Label 'source BnesBrowser POST-BUILD'

        Invoke-BnesProtectedOverlayGuard `
            -TreeRoot $BraveDir `
            -Label 'mapped src\brave POST-BUILD'
    }

    # --------------------------------------------------------
    # LAST KNOWN GOOD
    # --------------------------------------------------------

    Write-LastKnownGood `
        -Artifact $promoted `
        -CanonicalManifest $canonicalManifest `
        -UpstreamState $upstreamAfterInit `
        -ProjectionManifest $projectionManifest `
        -TransactionId $transactionId

    # --------------------------------------------------------
    # SUCCESS
    # --------------------------------------------------------

    $transactionState.status = 'SUCCESS'
    $transactionState.finished_at =
        (Get-Date).ToString('o')

    Write-TransactionState `
        -State $transactionState

    Write-Stage 'BNES SAFE BUILD SUCCESS'

    Write-Host "Transaction : $transactionId" -ForegroundColor Cyan
    Write-Host "Installer   : $SetupPath" -ForegroundColor Cyan
    Write-Host "SHA256      : $($promoted.sha256)" -ForegroundColor Cyan
    Write-Host ''
    Write-Host '============================================================' -ForegroundColor Green
    Write-Host ' BNES BUILD SUCCESS' -ForegroundColor Green
    Write-Host '============================================================' -ForegroundColor Green
    Write-Host ''

    exit 0
}
catch {

    $primaryError = $_
    Write-Host ''
    Write-Host "PRIMARY BUILD FAILURE: $($primaryError.Exception.Message)" -ForegroundColor Red
    
    $exitCode = 1

    try {

        $manifestArg = if (Get-Variable canonicalManifest -ErrorAction SilentlyContinue) { $canonicalManifest } else { $null }
        Invoke-TransactionFailure `
            -ErrorRecord $primaryError `
            -State $transactionState `
            -CanonicalManifest $manifestArg

        $exitCode = 1
    }
    catch {

        Write-Host ''
        Write-Host 'SECONDARY FAILURE WHILE HANDLING BUILD FAILURE' -ForegroundColor Red
        Write-Host $_.Exception.Message -ForegroundColor Red
        $exitCode = 2
    }

    exit $exitCode
}
finally {

    Remove-BnesBuildLock
}
