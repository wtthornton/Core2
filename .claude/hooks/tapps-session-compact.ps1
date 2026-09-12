# tapps-mcp-hook-version: 3.12.43
# tapps-mcp-hook-content-sha: 5930b681
# TappsMCP SessionStart hook (compact)
# Re-injects TappsMCP context after context compaction.
$null = $input | Out-Null
Write-Output "[TappsMCP] Context was compacted - re-injecting TappsMCP awareness."
Write-Output "Remember: use tapps_quick_check after editing Python files."
Write-Output "Run tapps_validate_changed before declaring work complete."
$proj = if ($env:TAPPS_PROJECT_ROOT) { $env:TAPPS_PROJECT_ROOT } elseif ($env:CLAUDE_PROJECT_DIR) { $env:CLAUDE_PROJECT_DIR } else { "." }
if (Get-Command tapps-mcp -ErrorAction SilentlyContinue) {
  $hint = & tapps-mcp usage-gaps-hint --project-root $proj 2>$null
  if ($hint) { Write-Output "TappsMCP prior-session reminder: $hint" }
}
exit 0
