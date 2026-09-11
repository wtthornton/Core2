---
name: linear-read
description: Read multi-issue Linear data via cache-first dance. MANDATORY for any list-style Linear read. Routes through tapps_linear_snapshot_get/put before list_issues. Use when listing, filtering, or reviewing Linear issues (backlog review, "what's open", triage, "find issues assigned to X"). Single-issue lookups go straight to get_issue instead.
mcp_tools:
  - tapps_linear_snapshot_get
  - tapps_linear_snapshot_put
  - tapps_linear_list_issues
  - linear_list_issues
  - linear_get_issue
---

Multi-issue Linear reads are cache-first. Invoke ANY time the user asks for a list, batch, or filtered view of Linear issues.

**When to invoke:** "list Linear issues", "what's open in TAP", "find issues assigned to X", "review the backlog". Skip for single-issue lookups (`get_issue(id="TAP-686")`).

Default slice for this repo: team `TappsCodingAgents`, project `Core2`.

**Core flow — every multi-issue read:**

1. `tapps_linear_snapshot_get(team, project, state, label?)` first.
2. On `cached=true`, use `data.issues` and filter in-memory — `list_issues` is NOT called.
3. On `cached=false`, call `tapps_linear_list_issues(team, project, state, label?, limit?)` as a gate check. On `ok=true`, call `linear_list_issues` with NARROW filters. On `ok=false`, follow the `hint` (re-call `snapshot_get` first).
4. Immediately call `tapps_linear_snapshot_put(team, project, issues_json=json.dumps(issues), state, label?, limit?)` with the **same** key dimensions as the get call.

**Anti-patterns — do not do these:**

- `list_issues` without a prior `snapshot_get` for the same key.
- `list_issues({})` or `list_issues({team, limit:250})` (the unfiltered scroll).
- Re-fetching the same narrow query 5-12 times in one turn with no intervening writes.
- Single-issue lookup via `list_issues` filtering — use `get_issue(id)` instead.
