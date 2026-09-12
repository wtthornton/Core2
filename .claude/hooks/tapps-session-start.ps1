# tapps-mcp-hook-version: 3.12.43
# tapps-mcp-hook-content-sha: 2f2bc6d2
# TappsMCP SessionStart hook (startup/resume)
# Directs the agent to call tapps_session_start as the first MCP action.
$null = $input | Out-Null
Write-Output "REQUIRED: Call tapps_session_start() NOW as your first action."
Write-Output "This initializes project context for all TappsMCP quality tools."
Write-Output "Tools called without session_start will have degraded accuracy."
$proj = if ($env:TAPPS_PROJECT_ROOT) { $env:TAPPS_PROJECT_ROOT } elseif ($env:CLAUDE_PROJECT_DIR) { $env:CLAUDE_PROJECT_DIR } else { "." }
if (Get-Command tapps-mcp -ErrorAction SilentlyContinue) {
  $hint = & tapps-mcp usage-gaps-hint --project-root $proj 2>$null
  if ($hint) { Write-Output "TappsMCP prior-session reminder: $hint" }
}
exit 0
