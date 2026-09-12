# tapps-mcp-hook-version: 3.12.43
# tapps-mcp-hook-content-sha: 4c1e991f
# TappsMCP PreToolUse hook — Linear cache-first read gate (TAP-1224)
$mode = 'warn'
$stdin = [Console]::In.ReadToEnd()
$tool = ''
try {
    $d = $stdin | ConvertFrom-Json
    if ($d.tool_name) { $tool = [string]$d.tool_name }
    elseif ($d.toolName) { $tool = [string]$d.toolName }
} catch { exit 0 }
if ($tool -ne 'mcp__plugin_linear_linear__list_issues' -and $tool -ne 'list_issues') {
    exit 0
}
$inp = $null
if ($d.PSObject.Properties.Name -contains 'tool_input') { $inp = $d.tool_input }
elseif ($d.PSObject.Properties.Name -contains 'toolInput') { $inp = $d.toolInput }
$team = ''; $project = ''; $state = ''; $label = ''; $limit = 50
if ($inp) {
    if ($inp.PSObject.Properties.Name -contains 'team' -and $inp.team) { $team = [string]$inp.team }
    if ($inp.PSObject.Properties.Name -contains 'project' -and $inp.project) { $project = [string]$inp.project }
    if ($inp.PSObject.Properties.Name -contains 'state' -and $inp.state) { $state = [string]$inp.state }
    if ($inp.PSObject.Properties.Name -contains 'label' -and $inp.label) { $label = [string]$inp.label }
    if ($inp.PSObject.Properties.Name -contains 'limit' -and $inp.limit) {
        try { $limit = [int]$inp.limit } catch { $limit = 50 }
    }
}
$key = ''
if ($team -and $project) {
    $filtObj = [ordered]@{}
    if ($state) { $filtObj['state'] = $state }
    if ($label) { $filtObj['label'] = $label }
    if ($limit) { $filtObj['limit'] = $limit }
    $payload = ($filtObj | ConvertTo-Json -Compress)
    $sha = [System.Security.Cryptography.SHA256]::Create()
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($payload)
    $hash = [BitConverter]::ToString($sha.ComputeHash($bytes)).Replace('-', '').ToLower().Substring(0, 16)
    $teamPart = if ($team) { $team.Replace('/', '_') } else { '_' }
    $projPart = if ($project) { $project.Replace('/', '_') } else { '_' }
    $statePart = if ($state) { $state.Replace('/', '_') } else { 'any' }
    $key = "${teamPart}__${projPart}__${statePart}__${hash}"
}
if (-not $key) { exit 0 }
$root = if ($env:CLAUDE_PROJECT_DIR) { $env:CLAUDE_PROJECT_DIR } else { $PWD.Path }
$dir = Join-Path $root '.tapps-mcp'
if ($env:TAPPS_LINEAR_SKIP_CACHE_GATE -eq '1') {
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    $entry = @{ ts = (Get-Date -Format 'o'); bypass = 'TAPPS_LINEAR_SKIP_CACHE_GATE'; key = $key } | ConvertTo-Json -Compress
    Add-Content -Path (Join-Path $dir '.bypass-log.jsonl') -Value $entry
    exit 0
}
$sentinel = Join-Path $dir ".linear-snapshot-sentinel-${key}"
if (Test-Path $sentinel) {
    $now = [int64]([DateTimeOffset]::Now.ToUnixTimeSeconds())
    $sent = 0
    try { $sent = [int64](Get-Content $sentinel -Raw).Trim() } catch {}
    if ($sent -gt 0 -and ($now - $sent) -le 300) { exit 0 }
}
if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
$violation = @{ ts = (Get-Date -Format 'o'); key = $key; mode = $mode } | ConvertTo-Json -Compress
Add-Content -Path (Join-Path $dir '.cache-gate-violations.jsonl') -Value $violation
if ($mode -eq 'warn') {
    [Console]::Error.WriteLine("[TappsMCP refusal layer=hook-only/defense-in-depth] Primary gate is the tapps_linear_list_issues server tool (TAP-2008 Agent Gateway). This hook is the fallback layer - it fired because the raw Linear plugin was called directly instead of through the wrapper.")
    [Console]::Error.WriteLine("TappsMCP: Linear cache-first read rule (TAP-1224, warn mode) - no recent tapps_linear_snapshot_get for this slice.")
    [Console]::Error.WriteLine("Route reads through the linear-read skill. Allowed (warn) but logged to .tapps-mcp/.cache-gate-violations.jsonl.")
    [Console]::Error.WriteLine("See .claude/rules/linear-standards.md.")
    exit 0
}
[Console]::Error.WriteLine("[TappsMCP refusal layer=hook-only/defense-in-depth] Primary gate is the tapps_linear_list_issues server tool (TAP-2008 Agent Gateway). This hook is the fallback layer - it fired because the raw Linear plugin was called directly instead of through the wrapper.")
[Console]::Error.WriteLine("TappsMCP: Blocked mcp__plugin_linear_linear__list_issues - no recent tapps_linear_snapshot_get for this slice.")
[Console]::Error.WriteLine("Route reads through the linear-read skill (TAP-1260): tapps_linear_snapshot_get -> filter on hit, or list_issues + snapshot_put on miss.")
[Console]::Error.WriteLine("For a single-issue lookup, use get_issue. Bypass: TAPPS_LINEAR_SKIP_CACHE_GATE=1 (logged).")
[Console]::Error.WriteLine("See .claude/rules/linear-standards.md.")
exit 2
