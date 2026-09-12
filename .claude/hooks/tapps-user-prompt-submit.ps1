# tapps-mcp-hook-version: 3.12.43
# tapps-mcp-hook-content-sha: cbd326d5
# TappsMCP UserPromptSubmit hook (TAP-975 / TAP-2000)
# Re-surfaces pipeline state per user turn so long sessions don't drift.
# Reads one sidecar:
#   .tapps-mcp/.session-start-marker   — Unix epoch of last tapps_session_start
# Checklist outcomes live in brain (checklist_outcome events via TAP-2000);
# call tapps_checklist or /tapps-finish-task — hooks cannot query brain.
# Stays SILENT when session_start was within 30 min.
$null = $input | Out-Null
$projDir = $env:CLAUDE_PROJECT_DIR
if (-not $projDir) { $projDir = "." }
$ssMarker = Join-Path $projDir '.tapps-mcp/.session-start-marker'
$now = [int64]([DateTimeOffset]::Now.ToUnixTimeSeconds())
$needSs = $false
if (-not (Test-Path $ssMarker)) {
    $needSs = $true
} else {
    $raw = ''
    try { $raw = (Get-Content -Path $ssMarker -Raw -ErrorAction Stop).Trim() } catch {}
    if ($raw -notmatch '^[0-9]+$') {
        $needSs = $true
    } else {
        $age = $now - [int64]$raw
        if ($age -gt 1800) { $needSs = $true }
    }
}
if (-not $needSs) {
    exit 0
}
[Console]::Error.WriteLine('[TappsMCP] Pipeline-state reminder:')
[Console]::Error.WriteLine('  - tapps_session_start was not called within the last 30 min - call it before edits to refresh project context.')
exit 0
