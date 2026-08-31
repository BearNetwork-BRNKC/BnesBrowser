
# generate_bnes_filterlist.ps1
# Generates bnes_default_list.txt and bnes_default_list.h
# All sources are public GitHub text files - no Brave private keys required.

param(
    [string]$OutputDir = "$PSScriptRoot\..\components\brave_shields\core\browser",
    [switch]$SkipDownload = $false
)

$ErrorActionPreference = "Stop"
$OutputTxt  = Join-Path $OutputDir "bnes_default_list.txt"
$OutputH    = Join-Path $OutputDir "bnes_default_list.h"
$CacheDir   = Join-Path $env:TEMP "bnes_filterlist_cache"
$CustomList = Join-Path $PSScriptRoot "bnes_custom_exceptions.txt"

Write-Host "=== BNES Filter List Generator ===" -ForegroundColor Cyan
Write-Host "Output: $OutputDir"

if (-not (Test-Path $CacheDir)) { New-Item -ItemType Directory -Path $CacheDir | Out-Null }
if (-not (Test-Path $OutputDir)) { New-Item -ItemType Directory -Path $OutputDir | Out-Null }

# --- Define lists to merge ---
# All are publicly available text files. No Brave server keys needed.
$lists = @(
    @{
        Name = "EasyList"
        Url  = "https://easylist.to/easylist/easylist.txt"
        Tag  = "easylist.txt"
    },
    @{
        Name = "EasyPrivacy"
        Url  = "https://easylist.to/easylist/easyprivacy.txt"
        Tag  = "easyprivacy.txt"
    },
    @{
        # KEY: Brave WebCompat exception list (brave-unbreak.txt)
        # Fixes EasyList over-blocking of streaming CDNs, players, etc.
        # Publicly available on GitHub - no private keys required.
        Name = "Brave-Unbreak (WebCompat Exceptions)"
        Url  = "https://raw.githubusercontent.com/brave/adblock-lists/master/brave-unbreak.txt"
        Tag  = "brave-unbreak.txt"
    },
    @{
        Name = "Brave-Core"
        Url  = "https://raw.githubusercontent.com/brave/adblock-lists/master/brave-core-ext.txt"
        Tag  = "brave-core-ext.txt"
    },
    @{
        Name = "uBlock-Unbreak"
        Url  = "https://raw.githubusercontent.com/uBlockOrigin/uAssets/master/filters/unbreak.txt"
        Tag  = "ublock-unbreak.txt"
    }
)

# --- Download or use cache ---
$combinedContent = [System.Text.StringBuilder]::new()

foreach ($list in $lists) {
    $cacheFile = Join-Path $CacheDir $list.Tag
    $content   = $null

    if (-not $SkipDownload) {
        Write-Host "`nDownloading: $($list.Name) ..." -NoNewline
        try {
            $response = Invoke-WebRequest -Uri $list.Url -UseBasicParsing -TimeoutSec 60
            $content  = $response.Content
            [System.IO.File]::WriteAllText($cacheFile, $content, [System.Text.Encoding]::UTF8)
            Write-Host " OK ($([Math]::Round($content.Length/1024))KB)" -ForegroundColor Green
        } catch {
            Write-Host " FAILED. Trying cache..." -ForegroundColor Yellow
            if (Test-Path $cacheFile) {
                $content = [System.IO.File]::ReadAllText($cacheFile, [System.Text.Encoding]::UTF8)
                Write-Host "   Using cache: $cacheFile" -ForegroundColor Yellow
            } else {
                Write-Host "   No cache, skipping!" -ForegroundColor Red
                continue
            }
        }
    } else {
        if (Test-Path $cacheFile) {
            $content = [System.IO.File]::ReadAllText($cacheFile, [System.Text.Encoding]::UTF8)
            Write-Host "Using cache: $($list.Name)"
        } else {
            Write-Host "Skipping (no cache): $($list.Name)" -ForegroundColor Yellow
            continue
        }
    }

    [void]$combinedContent.AppendLine("! ===== BNES bundled filter list: $($list.Tag) =====")
    [void]$combinedContent.AppendLine($content)
    [void]$combinedContent.AppendLine("")
}

# --- Add custom exceptions ---
Write-Host "`nAdding custom exceptions..."
if (Test-Path $CustomList) {
    $customContent = [System.IO.File]::ReadAllText($CustomList, [System.Text.Encoding]::UTF8)
    [void]$combinedContent.AppendLine("! ===== BNES Custom Exceptions =====")
    [void]$combinedContent.AppendLine($customContent)
    Write-Host "  Loaded: $CustomList" -ForegroundColor Green
} else {
    Write-Host "  No custom exceptions file found at $CustomList (skipping)" -ForegroundColor Yellow
}

# --- Write merged .txt ---
Write-Host "`nWriting merged list..."
$finalContent = $combinedContent.ToString()

# --- Strip Brave cosmetic `+js(set, navigator.connection, {})` for twitch.tv ---
# This is a brave-fix webcompat rule that breaks Twitch player (Error #4000).
# `+js(set, ...)` cannot be disabled via `@@||` allow rules; we must remove
# the rule entirely from the bundled list so the bundled default engine never
# injects navigator.connection = {} on twitch.tv frames.
$removedRules = 0
$finalContent = [regex]::Replace(
    $finalContent,
    '(?m)^[^\r\n]*\+js\(set,\s*navigator\.connection,\s*\{\s*\}\s*\)[^\r\n]*$',
    { param($m)
        $script:removedRules++
        $null
    }
)
if ($removedRules -gt 0) {
    Write-Host "  Stripped $removedRules navigator.connection={} cosmetic rule(s) (twitch player compat)" -ForegroundColor Yellow
}

[System.IO.File]::WriteAllText($OutputTxt, $finalContent, [System.Text.Encoding]::UTF8)
$txtSizeKB = [Math]::Round((Get-Item $OutputTxt).Length / 1024)
Write-Host "  Written: $OutputTxt" -ForegroundColor Green
Write-Host "  Size: $txtSizeKB KB"

# --- Convert .txt to C++ byte array header ---
Write-Host "`nGenerating C++ header..."

$bytes      = [System.IO.File]::ReadAllBytes($OutputTxt)
$totalBytes = $bytes.Length

$sb = [System.Text.StringBuilder]::new()
[void]$sb.AppendLine("// Copyright (c) 2026 The Bnes. All rights reserved.")
[void]$sb.AppendLine("// Auto-generated by tools/generate_bnes_filterlist.ps1")
[void]$sb.AppendLine("// DO NOT EDIT MANUALLY. Re-run the script to regenerate.")
[void]$sb.AppendLine("#ifndef BNES_DEFAULT_LIST_H_")
[void]$sb.AppendLine("#define BNES_DEFAULT_LIST_H_")
[void]$sb.AppendLine("#include <cstddef>")
[void]$sb.AppendLine("#include <cstdint>")
[void]$sb.AppendLine("namespace bnes_filterlist {")
[void]$sb.AppendLine("constexpr unsigned char kBnesDefaultList[] = {")

$lineBuilder = [System.Text.StringBuilder]::new()
for ($i = 0; $i -lt $bytes.Length; $i++) {
    [void]$lineBuilder.Append($bytes[$i])
    if ($i -lt $bytes.Length - 1) { [void]$lineBuilder.Append(",") }
    if (($i + 1) % 16 -eq 0 -or $i -eq $bytes.Length - 1) {
        [void]$sb.AppendLine("  " + $lineBuilder.ToString())
        $lineBuilder.Clear()
    }
}

[void]$sb.AppendLine("};")
[void]$sb.AppendLine("constexpr size_t kBnesDefaultListSize = $totalBytes;")
[void]$sb.AppendLine("}  // namespace bnes_filterlist")
[void]$sb.AppendLine("#endif  // BNES_DEFAULT_LIST_H_")

[System.IO.File]::WriteAllText($OutputH, $sb.ToString(), [System.Text.Encoding]::UTF8)
$hSizeMB = [Math]::Round((Get-Item $OutputH).Length / 1024 / 1024, 1)

Write-Host "  Written: $OutputH" -ForegroundColor Green
Write-Host "  Size: $hSizeMB MB"

Write-Host "`n=== DONE ===" -ForegroundColor Cyan
Write-Host "Merged list:  $txtSizeKB KB"
Write-Host "C++ Header:   $hSizeMB MB"
Write-Host "Total bytes:  $totalBytes"
Write-Host ""
Write-Host "Next step: Rebuild BnesBrowser" -ForegroundColor Yellow
Write-Host "  autoninja -C out\Release brave"
