# tapps-mcp-hook-version: 3.12.43
# tapps-mcp-hook-content-sha: fc3ccba4
# TappsMCP PreCompact hook (TAP-2017)
# Indexes pre-compaction session state in brain for post-compact rehydration.
# Set TAPPS_MCP_COMPACTION_REHYDRATE=false to disable.
$rawInput = @($input) -join "`n"
$projDir = $env:CLAUDE_PROJECT_DIR
$backupDir = if ($projDir) { "$projDir/.tapps-mcp" } else { ".tapps-mcp" }
if (-not (Test-Path $backupDir)) {
    New-Item -ItemType Directory -Path $backupDir -Force | Out-Null
}
$outFile = "$backupDir/pre-compact-context.json"
$rawInput | Set-Content -Path $outFile -Encoding UTF8
# Index in brain and write rehydration marker via tapps-mcp CLI.
if (Get-Command tapps-mcp -ErrorAction SilentlyContinue) {
    $rawInput | tapps-mcp compact-index --project-root ($projDir ?? ".") 2>$null
}
Write-Output "[TappsMCP] Pre-compact session indexed for rehydration."
exit 0
