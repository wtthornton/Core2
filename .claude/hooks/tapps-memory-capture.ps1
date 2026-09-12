# tapps-mcp-hook-version: 3.12.43
# tapps-mcp-hook-content-sha: e528eb2c
# TappsMCP Stop hook - Session Quality Tracker (TAP-1999)
# Session episodic memory migrated to brain-native memory_index_session.
# Hook retained only for the stop_hook_active guard (prevents infinite loops).
# IMPORTANT: Must check stop_hook_active to prevent infinite loops.
$rawInput = @($input) -join "`n"
try {
    $data = $rawInput | ConvertFrom-Json
    $active = $data.stop_hook_active
} catch {
    $active = $false
}
if ($active -eq $true -or $active -eq "true" -or $active -eq "True") {
    exit 0
}
$projDir = $env:CLAUDE_PROJECT_DIR
if (-not $projDir) { $projDir = "." }
$captureDir = "$projDir/.tapps-mcp"
$marker = "$captureDir/.validation-marker"
if (Test-Path $marker) {
    # validation occurred; tapps_session_start handles brain indexing via memory_index_session
}
# Session capture now handled by brain-native memory_index_session (TAP-1999).
exit 0
