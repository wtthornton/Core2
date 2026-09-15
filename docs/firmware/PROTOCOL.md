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
| GET | `/projects/{slug}/events` | Bearer | live SSE **ops alerts** (`consumer_id=core2-desk`) — not chat |
| POST | `/projects/{slug}/voice/turns` | Bearer | PTT audio (WAV/PCM). **TAP-7548 / TAP-7550** — 404 until AF ships |
| GET | `/projects/{slug}/voice/turns/{id}` | Bearer | poll `transcript`, `reply_text`, `status` (TAP-7550) |
| GET | `audio_url` from voice-turn | Bearer | AF TTS (mono PCM/WAV) — **TAP-7554**; else spoken-blocked |
| GET | project stream (`afp_`) | Bearer | Talk tokens — **TAP-7555**; not `/events`, not TAP-7503 |

**Do not** call fleet `/stats/*` with an `afp_` key — AF returns 403
(`stats_scope_required` / `cross-project-denied`). Fleet monitor needs a
platform key or TAP-7503; Core2 firmware stays on project routes only.

Poll interval ~5s while Wi-Fi connected. Voice POST/poll is a **separate
client** from the ops GET poller (longer timeout, larger body).

### Talk / Jarvis (PTT)

- Capture on device → POST audio to AF. AF owns STT (and TTS when TAP-7554 ships).
- Auth: device **`afp_` only**. Never `ws_auth_token` or platform `af_*`.
- `GET /projects/{slug}/events` is ops alerts, not assistant tokens. Token
  stream waits on TAP-7555; until then Talk stays poll-only with honest
  **no stream** copy.
- **Blocked UI** when voice-turn is missing (404/unimplemented): show
  `blocked: no AF voice API (TAP-7550)` — do not invent a local reply or a
  PC STT/TTS proxy.
- Spoken playback waits on TAP-7554 (`audio` / `audio_url`). Without it,
  show text reply plus `spoken blocked (TAP-7554)`. Device **status** lines
  (blocked API, spoken-blocked, mic missing, need key, too short) also play
  canned WAVs on the NS4168 when Quiet is off. That is not local Jarvis TTS.

### Auth

- Prefer gated `GET /projects/{slug}` — `/health` alone proves nothing.
- HTTP 401/403 on project routes → Status **Need key**.
- Device holds **`afp_` only** — never platform `af_*` or `AF_PLUGIN_ADMIN_KEY`.

### Poll-alert stopgap (TAP-7502)

When dual-meters `error_count` rises vs previous poll, firmware raises a local
alert. Project SSE (`GET /projects/{slug}/events`) raises on
`invocation.terminal` / `run.completed` with `is_error`. Fleet-wide SSE still
waits on **TAP-7503**.

Zeros on Hub with **Live** + **auth ok** are real empty project stats, not a
parse bug. `GET /projects/core2/stats` returns `total_invocations: 0` until
that slug has traffic.

## NVS setup (Phase D)

Compile-time `secrets.h` is the factory default. Beam → tap cube opens setup.
Save writes NVS and reconnects Wi-Fi. USB debug (115200):

```text
cfg show
cfg ssid MyNet
cfg pass secret
cfg url http://192.168.1.207:8010
cfg slug core2
cfg key afp_…
cfg save
cfg wipe
cfg quiet 2200 0700
cfg quiet off
cfg time
cfg ota http://192.168.1.207:8000/firmware.bin
```

Quiet hours persist in NVS (Pacific `CORE2_TZ`). Until NTP locks, hours do not mute. Soft-key Quiet is still 15 minutes.

OTA pulls `firmware.bin` over **unsigned http** on the LAN (`cfg ota http://host/firmware.bin`). There is no auto-update and no https. Partition table already has app0/app1 (`default_16MB.csv`). USB flash remains the recovery path.

`GET /health` and `GET /ready` are public. Opening `/projects/core2` in a browser returns `Missing or invalid Authorization header` — that route needs `Authorization: Bearer afp_…`.

Opening a serial monitor can pulse CP210x DTR and reboot the brick. `platformio.ini` sets `monitor_dtr = 0` / `monitor_rts = 0`; flash still uses auto-reset.

## Buttons / faces

Ops-cube on **320×240**: Hub · Pulse · Heat · Trail · Beam · **Talk**.
Top face strip **46 px** (large touch tabs, compact labels). Soft keys **40 px**:
Prev / Quiet / Next on ops faces; on Talk the middle key is **hold-to-talk**.
Swipe or tap chips. BtnA/B/C map to the same (BtnB = PTT on Talk, Quiet elsewhere).

## Deleted

Host NDJSON v1 and `scripts/af_bridge.py` are removed from the product path.
