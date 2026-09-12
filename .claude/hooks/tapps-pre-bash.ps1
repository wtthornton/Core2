# tapps-mcp-hook-version: 3.12.43
# tapps-mcp-hook-content-sha: b7f23fad
# TappsMCP PreToolUse hook (Bash) - destructive command guard (opt-in)
# Blocks commands containing rm -rf, format c:, etc. Exit 2 = block, 0 = allow.
$raw = @($input) -join "`n"
$cmd = ""
try {
    $data = $raw | ConvertFrom-Json
    $ti = $data.tool_input
    if ($ti.command) { $cmd = $ti.command }
    elseif ($ti.args) { $cmd = ($ti.args | ForEach-Object { $_ }) -join " " }
} catch {}
$block = $false
if ($cmd -match 'rm\s+-[rf]+' -and $cmd -match '/') { $block = $true }
if ($cmd -match 'format\s+[cC]:') { $block = $true }
if ($cmd -match 'del\s+/[fs]*\s*/[sq]*' -or $cmd -match 'rd\s+/s\s+/q') { $block = $true }
if ($block) {
    Write-Error "TappsMCP: Blocked potentially destructive command."
    exit 2
}
exit 0
