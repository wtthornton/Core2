# tapps-mcp-hook-version: 3.12.43
# tapps-mcp-hook-content-sha: 0eed38f2
# TappsMCP beforeMCPExecution hook
# Logs MCP tool invocations and reminds to call session_start.
# stdout must be exactly one JSON object (Cursor blocks the tool otherwise).
$rawInput = @($input) -join "`n"
try {
    $data = $rawInput | ConvertFrom-Json
    $tool = if ($data.tool_name) { $data.tool_name }
             elseif ($data.tool) { $data.tool }
             else { "unknown" }
} catch {
    $tool = "unknown"
}
$payload = '{"permission":"allow"}'
if ($tool -match '^tapps_') {
    $sentinel = "$env:TEMP\.tapps-session-started-$PID"
    if ($tool -eq 'tapps_session_start') {
        $null = New-Item -ItemType File -Path $sentinel -Force
    } elseif (-not (Test-Path $sentinel)) {
        $payload = '{"permission":"allow","agent_message":"REMINDER: Call tapps_session_start() first for best results."}'
    }
}
Write-Output $payload
[Console]::Error.WriteLine("[TappsMCP] MCP tool invoked: $tool")
exit 0
