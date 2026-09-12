# tapps-mcp-hook-version: 3.12.43
# tapps-mcp-hook-content-sha: 9d7d62a4
# TappsMCP PostToolUse hook — Linear cache-gate sentinel writer (TAP-1224)
$stdin = [Console]::In.ReadToEnd()
$tool = ''
try {
    $d = $stdin | ConvertFrom-Json
    if ($d.tool_name) { $tool = [string]$d.tool_name }
    elseif ($d.toolName) { $tool = [string]$d.toolName }
} catch { exit 0 }
if ($tool -ne 'mcp__tapps-mcp__tapps_linear_snapshot_get' -and $tool -ne 'mcp__nlt-linear-issues__tapps_linear_snapshot_get' -and $tool -ne 'tapps_linear_snapshot_get') {
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
if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
$ts = [int64]([DateTimeOffset]::Now.ToUnixTimeSeconds())
Set-Content -Path (Join-Path $dir ".linear-snapshot-sentinel-${key}") -Value $ts -Encoding UTF8
exit 0
