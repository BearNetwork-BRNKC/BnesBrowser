Set-Location E:\BnesBrowser-build\src
$env:DEPOT_TOOLS_WIN_TOOLCHAIN='0'
$out = & .\buildtools\win\gn.exe --root=. -q gen out/Release_GN 2>&1
$ec = $LASTEXITCODE
$out | Out-File 'S:\Ai_Agent\BNES\BnesBrowser\_gngen.log' -Encoding UTF8
$ec | Out-File 'S:\Ai_Agent\BNES\BnesBrowser\_gngen.ec' -Encoding UTF8
