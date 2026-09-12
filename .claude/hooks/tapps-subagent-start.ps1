# tapps-mcp-hook-version: 3.12.43
# tapps-mcp-hook-content-sha: d87d0957
# TappsMCP SubagentStart hook
# Injects TappsMCP awareness into spawned subagents.
$null = $input | Out-Null
Write-Output "[TappsMCP] This project uses TappsMCP for code quality."
Write-Output "Tools: tapps_quick_check, tapps_score_file, tapps_validate_changed. Memory: uv run tapps-mcp memory …; tapps_memory on nlt-memory when enabled (TAP-3895)."
exit 0
