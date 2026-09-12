# Plan: Core2 AgentForge monitor HMI

Status: **ARCHITECTURE RESET (2026-09-12).** Product path is
**Core2 → AgentForge directly**. Host middleware (`af_bridge`) is **deprecated
and scheduled for deletion**. See [Cleanup plan](#cleanup-plan) below.

**Vision (source of truth):** [VISION.md](VISION.md)  
**Full execution plan:** [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)  
Hard rule: `.cursor/rules/core2-af-direct.mdc`.

Hardware: original Core2, CP2104 **COM4**, AXP192, MPU6886, 16 MB flash.

## Goal

Desk-side HMI **on the brick**. Firmware (C++ / M5Unified + M5GFX) polls and
subscribes to **AgentForge HTTP/SSE** on the LAN (or Tailscale). No always-on
PC process is required for ops data.

## Decision (locked — reset)

| Layer | Choice | Why |
|-------|--------|-----|
| On-device HAL | **M5Unified + M5GFX** | Display, touch, IMU, RTC, speaker, PMU |
| JSON | **ArduinoJson 7** | Parse AF JSON / NDJSON-sized payloads |
| Build / flash | **PlatformIO CLI** + esptool | `board = m5stack-core2` |
| Network | **Device STA Wi-Fi → AF** | Brick is the AF client |
| AF surface | AF public + authenticated APIs | `/health` `/ready` `/stats/*` + **fleet** alert stream (needs AF) |
| Auth | Bearer on device (`secrets.h` / NVS) | No host key vault for the product path |
| USB serial | Flash + debug logs only | **Not** the ops data bus |

### Explicitly rejected

- Host Python bridge that polls AF and pushes NDJSON over USB/LAN
- “API keys stay on the PC; brick only talks to localhost bridge”
- Building AgentForge features inside the Core2 repo

## Product screens (unchanged intent)

1. **Status** — AF ready/down, version, invocations, error rate, cost, last agent
2. **Issues** — 24h failure count + last five agents
3. **Trend** — 7-day invocation / error sparklines

BtnA ack alert, BtnB mute 15 min, BtnC cycle screens. Unacked alerts vibe + tone.

## Target device config

```text
firmware/src/secrets.h   (gitignored for Wi-Fi)
  CORE2_WIFI_SSID / PASS
  AF_BASE_URL            e.g. http://192.168.1.207:8010
  AF_PROJECT_SLUG        core2
  AF_API_KEY             afp_… project key (mint paste from AF host)
```

## AF API contract (Core2 consumer)

| Need | AF today (v4.60.0 / fec001e6) | Core2 action |
|------|------------------------------|--------------|
| Liveness / version | `GET /health`, `GET /ready` (public) | Use directly |
| Auth self-check | `GET /projects/{slug}` + Bearer `afp_` | Required; `/health` is not enough |
| Fleet stats | `GET /stats/*` needs platform `af_` | **Do not use with afp_** |
| Project stats | `GET /projects/{slug}/stats`, `activity-series`, `dual-meters` + `afp_` | **Firmware uses these** |
| Live fleet alerts | **Missing** — only `GET /projects/{slug}/events` + project key | **AF Linear issue** — do not build host SSE proxy |
| Ack / mute | No AF standard desk-ack API | Optional AF issue or local-only mute |

## Cleanup plan

### Phase A — lock direction (this session)

- [x] Hard rule `core2-af-direct.mdc`
- [x] Rewrite this PLAN + PROTOCOL
- [x] Linear Core2 + AgentForge Platform issues
- [x] Stop running `af_bridge` processes
- [x] Session handoff / memory note

### Phase B — delete host middleware (Core2 code)

- [x] Delete `scripts/af_bridge.py`
- [x] Delete `scripts/af_protocol.py`
- [x] Delete `tests/test_af_bridge.py`
- [x] Remove README / INDEX / `.env.example` bridge knobs (`CORE2_BRIDGE_*`)
- [x] Firmware polls AF (`af_client.cpp`), not `:8766/v1/state`
- [x] Serial NDJSON ops path removed (debug drain only)
- [x] TAP-7471–7474 commented; TAP-7484 canceled

### Phase C — implement direct AF client (Core2 firmware)

- [x] `secrets.h`: `AF_BASE_URL` + `AF_PROJECT_SLUG` + `AF_API_KEY` (`afp_`) + Wi-Fi
- [x] HTTP client polls `/health`, `/ready`, `/projects/{slug}`, `/stats/*`
- [x] Map JSON → UI state
- [ ] SSE client when AF ships TAP-7503
- [x] Local mute/ack; poll-alert stopgap (TAP-7502)

### Phase D — operator polish (after C works)

- NVS Wi-Fi + key setup screen, mute schedule, OTA

## Flash (during transition)

1. `pip install -r requirements.txt`
2. `python scripts/core2_dev.py upload --port COM4`
3. Do **not** start `af_bridge` for product use

## Linear

| ID | Project | Role |
|----|---------|------|
| TAP-7499 | Core2 | Epic: direct AF monitor |
| TAP-7500 | Core2 | Delete host middleware |
| TAP-7501 | Core2 | Firmware AF HTTP poll client |
| TAP-7502 | Core2 | Poll-alert stopgap (blocked by TAP-7503) |
| TAP-7503 | AgentForge Platform | Fleet monitor SSE |
| TAP-7504 | AgentForge Platform | Docs: device keys (superseded for Core2 by afp_-only policy) |
| TAP-7484 | Core2 | Canceled (obsolete bridge framing) |
| TAP-7471–7474 | Core2 | Done historical; commented superseded |
