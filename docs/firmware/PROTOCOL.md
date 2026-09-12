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

## Device → AgentForge

| Method | Path | Auth | Maps to |
|--------|------|------|---------|
| GET | `/health` | none | reachable, version, degraded |
| GET | `/ready` | none | ready |
| GET | `/projects/{slug}` | Bearer `afp_` | auth self-check (200 vs 401) |
| GET | `/stats/summary` | Bearer | inv, err, cost, agent |
| GET | `/stats/dashboard?days=7` | Bearer | Trend sparklines |
| GET | `/stats/failures?limit=5` | Bearer | Issues + poll-alert stopgap |
| GET | fleet SSE (TBD TAP-7503) | Bearer | live haptic alerts |

Poll interval ~5s while Wi-Fi connected.

### Auth

- Prefer gated `GET /projects/{slug}` with the project key — `/health` alone proves nothing.
- HTTP 401/403 on project or `/stats/*` → Status degraded, agent `need afp_ key`.
- Device holds **`afp_` only** — never platform `af_*` or `AF_PLUGIN_ADMIN_KEY`.

### Poll-alert stopgap (TAP-7502)

When `last_24h_count` rises vs previous poll, firmware raises a local alert
(`src=stats.failures`). Real fleet SSE waits on **TAP-7503**.

## Buttons

- **A** — ack local alert
- **B** — mute haptics 15 minutes (local)
- **C** — cycle Status → Issues → Trend

## Deleted

Host NDJSON v1 and `scripts/af_bridge.py` are removed from the product path.
