# Wrapper that restores the Brave-style PYTHONPATH before invoking the local build.
# Does NOT modify run_remaining.ps1, bnes\, args.gn, or any Chromium/BNES source.
# All paths are already-checked-out local dirs; nothing is downloaded.

[CmdletBinding()]
param(
    [switch]$SkipGnGen,
    [switch]$VerifyOnly,
    [ValidateRange(1, 256)]
    [int]$Jobs = 12
)

$ErrorActionPreference = 'Stop'

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path   # ...\BnesBrowser
$SourceRoot  = Split-Path -Parent $ProjectRoot                   # ...\src

# Same PYTHONPATH components the previous build script set for Chromium-tooled
# Python (ts_library.py imports brave_chromium_utils from <root>\script).
$parts = @(
    (Join-Path $ProjectRoot 'script')
    (Join-Path $SourceRoot 'tools\grit\grit\extern')
    (Join-Path $ProjectRoot 'vendor\requests')
    (Join-Path $ProjectRoot 'third_party\cryptography')
    (Join-Path $ProjectRoot 'third_party\macholib')
    (Join-Path $SourceRoot 'build')
    (Join-Path $SourceRoot 'third_party\depot_tools')
    $env:PYTHONPATH
) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

$env:PYTHONPATH = ($parts -join ';')
$env:PYTHONUTF8 = '1'

$inner = (Join-Path $ProjectRoot 'run_remaining.ps1')
$runArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $inner)
if ($SkipGnGen) { $runArgs += '-SkipGnGen' }
if ($VerifyOnly) { $runArgs += '-VerifyOnly' }
$runArgs += '-Jobs'
$runArgs += "$Jobs"

& pwsh @runArgs
exit $LASTEXITCODE