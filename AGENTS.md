# Core2 — instructions for AI assistants

This is the **Core2** repo (`wtthornton/Core2`). Keep writes in this tree and in Linear project **Core2**.

## Identity

| Field | Value |
|-------|-------|
| GitHub | https://github.com/wtthornton/Core2 |
| Linear team | `TappsCodingAgents` (`TAP`) |
| Linear project | `Core2` |
| Agent assignee | `Claude Agent` |
| Config | `.tapps-mcp.yaml` |

## Linear

Route Linear through skills, not raw MCP calls:

1. **Writes** (create / update / triage) — `linear-issue` skill. Validate with `nlt-linear-issues` before saving.
2. **Multi-issue reads** — `linear-read` skill (cache-first).
3. **Single issue** — `get_issue(id="TAP-###")`.
4. **Releases** — `linear-release-update` skill.

Linear MCP lives in `.cursor/mcp.json` as server `linear` (`https://mcp.linear.app/mcp`). If Linear tools are missing, authenticate with `mcp_auth` on that server, then retry.

## Sibling projects

`C:\code\TappsMCP`, `C:\code\ReportLab`, `C:\code\nlt-ideas-scout`, and other neighbors are **reference only**. Do not change their files or Linear projects from this workspace.

## Quality

When TappsMCP (`nlt-build`) is connected:

1. Call `tapps_session_start` at the start of a working session.
2. After Python edits, run `tapps_quick_check`.
3. Before declaring multi-file work complete, run `tapps_validate_changed` with explicit `file_paths`.
