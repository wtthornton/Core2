# tapps-mcp-hook-version: 3.12.43
# tapps-mcp-hook-content-sha: 81581245
# TappsMCP PostToolUse hook — Linear gate sentinel writer (TAP-981/TAP-986)
# Writes .tapps-mcp/.linear-validate-sentinel with current Unix epoch seconds
# whenever an agent calls mcp__docs-mcp__docs_validate_linear_issue. Paired
# with tapps-pre-linear-write.ps1 which reads the sentinel to decide whether
# to allow a downstream save_issue.
$stdin = [Console]::In.ReadToEnd()
$tool = ""
try {
    $d = $stdin | ConvertFrom-Json
    if ($d.tool_name) { $tool = [string]$d.tool_name }
    elseif ($d.toolName) { $tool = [string]$d.toolName }
} catch {}
if ($tool -eq 'mcp__docs-mcp__docs_validate_linear_issue' -or $tool -eq 'mcp__nlt-linear-issues__docs_validate_linear_issue' -or $tool -eq 'docs_validate_linear_issue') {
    $root = if ($env:CLAUDE_PROJECT_DIR) { $env:CLAUDE_PROJECT_DIR } else { $PWD.Path }
    $dir = Join-Path $root '.tapps-mcp'
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }
    $ts = [int64]([DateTimeOffset]::Now.ToUnixTimeSeconds())
    Set-Content -Path (Join-Path $dir '.linear-validate-sentinel') -Value $ts -Encoding UTF8
}
exit 0
