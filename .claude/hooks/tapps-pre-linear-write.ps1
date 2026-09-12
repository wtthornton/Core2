# tapps-mcp-hook-version: 3.12.43
# tapps-mcp-hook-content-sha: 35dfef5f
# TappsMCP PreToolUse hook — Linear write gate (TAP-981/TAP-986)
# Blocks mcp__plugin_linear_linear__save_issue if no recent
# docs_validate_linear_issue sentinel (within 30 minutes). Bypass with
# TAPPS_LINEAR_SKIP_VALIDATE=1 (logged to .tapps-mcp/.bypass-log.jsonl).
$stdin = [Console]::In.ReadToEnd()
$tool = ""
$updateOnly = $false
try {
    $d = $stdin | ConvertFrom-Json
    if ($d.tool_name) { $tool = [string]$d.tool_name }
    elseif ($d.toolName) { $tool = [string]$d.toolName }
    $inp = $null
    if ($d.PSObject.Properties.Name -contains 'tool_input') { $inp = $d.tool_input }
    elseif ($d.PSObject.Properties.Name -contains 'toolInput') { $inp = $d.toolInput }
    if ($inp) {
        $hasId = [bool]($inp.PSObject.Properties.Name -contains 'id' -and $inp.id)
        $hasTemplate = [bool](
            ($inp.PSObject.Properties.Name -contains 'title' -and $inp.title) -or
            ($inp.PSObject.Properties.Name -contains 'description' -and $inp.description)
        )
        if ($hasId -and -not $hasTemplate) { $updateOnly = $true }
    }
} catch {}
if ($tool -ne 'mcp__plugin_linear_linear__save_issue' -and $tool -ne 'save_issue') {
    exit 0
}
# Update-only allow-list (TAP-981 FP reduction): metadata-only updates skip the sentinel.
if ($updateOnly) {
    exit 0
}
$root = if ($env:CLAUDE_PROJECT_DIR) { $env:CLAUDE_PROJECT_DIR } else { $PWD.Path }
$dir = Join-Path $root '.tapps-mcp'
if ($env:TAPPS_LINEAR_SKIP_VALIDATE -eq '1') {
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }
    $nowIso = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    $entry = @{ ts = $nowIso; bypass = 'TAPPS_LINEAR_SKIP_VALIDATE' } | ConvertTo-Json -Compress
    Add-Content -Path (Join-Path $dir '.bypass-log.jsonl') -Value $entry
    exit 0
}
$sentinel = Join-Path $dir '.linear-validate-sentinel'
if (-not (Test-Path $sentinel)) {
    [Console]::Error.WriteLine("[TappsMCP refusal layer=hook-only/defense-in-depth] Primary gate is the docs_save_linear_issue server tool (TAP-2008 Agent Gateway). This hook is the fallback layer - it fired because the raw Linear plugin was called directly instead of through the wrapper.")
    [Console]::Error.WriteLine("TappsMCP: Blocked mcp__plugin_linear_linear__save_issue - no recent docs_validate_linear_issue call.")
    [Console]::Error.WriteLine("Route Linear writes through the linear-issue skill:")
    [Console]::Error.WriteLine("  1. docs_generate_story (or docs_generate_epic)")
    [Console]::Error.WriteLine("  2. docs_validate_linear_issue")
    [Console]::Error.WriteLine("  3. plugin save_issue")
    [Console]::Error.WriteLine("  4. tapps_linear_snapshot_invalidate")
    [Console]::Error.WriteLine("Or set TAPPS_LINEAR_SKIP_VALIDATE=1 for emergency bypass (logged).")
    [Console]::Error.WriteLine("See .claude/rules/linear-standards.md.")
    exit 2
}
$now = [int64]([DateTimeOffset]::Now.ToUnixTimeSeconds())
$raw = ""
try {
    $raw = (Get-Content -Path $sentinel -Raw -ErrorAction Stop).Trim()
} catch {}
if ($raw -notmatch '^[0-9]+$') {
    $sent = [int64]0
} else {
    $sent = [int64]$raw
}
$age = $now - $sent
if ($age -le 1800) {
    exit 0
}
[Console]::Error.WriteLine("[TappsMCP refusal layer=hook-only/defense-in-depth] Primary gate is the docs_save_linear_issue server tool (TAP-2008 Agent Gateway). This hook is the fallback layer - it fired because the raw Linear plugin was called directly instead of through the wrapper.")
[Console]::Error.WriteLine("TappsMCP: Blocked mcp__plugin_linear_linear__save_issue - last docs_validate_linear_issue was ${age}s ago (> 1800s freshness window).")
[Console]::Error.WriteLine("Re-validate before push: docs_validate_linear_issue(title=..., description=..., ...)")
[Console]::Error.WriteLine("Or set TAPPS_LINEAR_SKIP_VALIDATE=1 for emergency bypass (logged).")
[Console]::Error.WriteLine("See .claude/rules/linear-standards.md.")
exit 2
