# Core2 ↔ AgentForge protocol (direct)

**HARD RULE:** the brick is an AgentForge **HTTP client**. There is no host
NDJSON ops bus. Vision: [VISION.md](VISION.md). Fleet live SSE: AF TAP-7503.

USB serial (115200) is for **flash and debug logs only**.

## Device config (`secrets.h`)

Paste block from the AF host mint script:

```c
CORE2_WIFI_SSID / CORE2_WIFI_PASS   // private — keep secrets.h gitignored
AF_BASE_URL     // e.g. http://192.168.1.207:8010 (LAN DHCP may change)
AF_PROJECT_SLUG // "core2"
AF_API_KEY      // afp_… project key (desk credential; not af_* / admin)
```

`AF_URL` remains an alias for `AF_BASE_URL`.

## Device → AgentForge (project-scoped `afp_`)

| Method | Path | Auth | Maps to |
|--------|------|------|---------|
| GET | `/health` | none | reachable, version, degraded |
| GET | `/ready` | none | ready |
| GET | `/projects/{slug}` | Bearer `afp_` | auth self-check (200 vs 401) |
| GET | `/projects/{slug}/stats` | Bearer | inv, err, cost, agent |
| GET | `/projects/{slug}/activity-series?days=7` | Bearer | Trail charts |
| GET | `/projects/{slug}/dual-meters` | Bearer | Heat error count |
| GET | `/projects/{slug}/invocations?limit=5` | Bearer | recent rows |
| GET | `/projects/{slug}/events` | Bearer | future SSE (TAP-7503 fleet still separate) |

**Do not** call fleet `/stats/*` with an `afp_` key — AF returns 403
(`stats_scope_required` / `cross-project-denied`). Fleet monitor needs a
platform key or TAP-7503; Core2 firmware stays on project routes only.

Poll interval ~5s while Wi-Fi connected.

### Auth

- Prefer gated `GET /projects/{slug}` — `/health` alone proves nothing.
- HTTP 401/403 on project routes → Status **Need key**.
- Device holds **`afp_` only** — never platform `af_*` or `AF_PLUGIN_ADMIN_KEY`.

### Poll-alert stopgap (TAP-7502)

When dual-meters `error_count` rises vs previous poll, firmware raises a local
alert. Real fleet SSE waits on **TAP-7503**.

## Buttons / faces

Ops-cube on **320×240**: Hub · Pulse · Heat · Trail · Beam.
Top face strip **46 px** (large touch tabs, compact labels). Soft keys **40 px**:
Prev / Quiet / Next. Swipe or tap chips. BtnA/B/C map to the same.

## Deleted

Host NDJSON v1 and `scripts/af_bridge.py` are removed from the product path.
