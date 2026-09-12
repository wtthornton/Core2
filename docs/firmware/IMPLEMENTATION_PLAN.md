# Implementation plan: vision reset (brick → AF direct)

**Locked vision:** [VISION.md](VISION.md)  
**Date:** 2026-09-12  
**Owner:** Core2 only (firmware + Core2 Linear). AF API gaps → AgentForge Platform Linear.

This plan is the ordered work to **align docs, Linear, code, and verification** with the vision, then **implement** the direct AF client.

---

## Linear IDs (2026-09-12)

| ID | Project | Role |
|----|---------|------|
| TAP-7499 | Core2 | Epic |
| TAP-7500 | Core2 | Delete middleware |
| TAP-7501 | Core2 | Firmware AF client |
| TAP-7502 | Core2 | Poll-alert stopgap |
| TAP-7503 | AgentForge Platform | Fleet monitor SSE |
| TAP-7504 | AgentForge Platform | Device key docs (Core2 uses afp_ only) |
| TAP-7484 | Core2 | Canceled |

---

## 0. Review findings (current drift)

### Aligned (keep)

| Artifact | Status |
|----------|--------|
| `docs/firmware/VISION.md` | Source of truth |
| `docs/firmware/PLAN.md` | Reset + cleanup outline |
| `docs/firmware/PROTOCOL.md` | Direct AF contract (partial) |
| `.cursor/rules/core2-af-direct.mdc` | Agent hard rule |
| `.cursor/rules/core2.mdc` | Points at vision |
| `docs/INDEX.md` / `README.md` | Point at VISION |

### Drift / wrong (must fix)

| Artifact | Problem |
|----------|---------|
| `scripts/af_bridge.py` | Host middleware — **delete** |
| `scripts/af_protocol.py` | Bridge helpers — **delete** (or replace with firmware-only tests later) |
| `tests/test_af_bridge.py` | Tests the middleware — **delete** |
| `firmware/src/net.cpp` | Polls `…/v1/state` on bridge host — **rewrite → AF** |
| `firmware/src/config.h` + `secrets.h.example` | `CORE2_BRIDGE_*` — **replace with `AF_BASE_URL` / `AF_PROJECT_SLUG` / `AF_API_KEY` (`afp_`)** |
| `firmware/src/protocol.cpp` | Serial NDJSON ingest as product path — **rewrite** |
| `.env.example` / `.env` | Bridge + host key story — **trim** |
| `docs/kb/README.md` | Links PLAN/PROTOCOL without VISION |
| `llms.txt` / handoff | May still describe bridge |
| TAP-7471–7474 (Done) | Documented **obsolete** architecture |
| TAP-7484 (Backlog) | Still about `af_bridge` + host keys — **cancel/rewrite** |

### AF capability (consumer facts)

| Need | AF today | Action |
|------|----------|--------|
| `/health`, `/ready` | Public OK | Use on device |
| `/projects/{slug}` | Bearer `afp_` | Auth self-check on device |
| `/stats/summary`, `/dashboard`, `/failures` | Bearer `afp_` | Use on device |
| Fleet live alerts SSE | **Missing** (project SSE only) | **AF Platform Linear** |
| Desk ack API | Missing / unclear | Optional AF Linear; local mute OK |

---

## 1. Documentation (update + verify)

### 1.1 Update

1. Keep VISION.md as top of stack; any conflict → VISION wins.  
2. PLAN.md — replace “phase checklist” with this file’s phases; mark B/C/D status.  
3. PROTOCOL.md — full device→AF request/response field map (no NDJSON host section except “deleted”).  
4. `docs/kb/README.md` — link VISION first.  
5. `llms.txt` — one paragraph: brick→AF direct.  
6. `.env.example` — only flash/dev helpers if any; **no** `CORE2_BRIDGE_*`, no required `AF_*` for a PC daemon (device secrets are `secrets.h`).  
7. Session handoff — rewrite under new vision.  
8. Memory (when brain up): save key `core2-af-direct-vision` pointing at VISION.md.

### 1.2 Verify

- Ripgrep repo for: `af_bridge`, `/v1/state`, `CORE2_BRIDGE`, `host bridge`, `NDJSON v1` product path.  
- Expect hits only in VISION/PLAN “rejected / cleanup” sections or git history.  
- Manual read of README, INDEX, kb README, PROTOCOL, PLAN vs VISION.

---

## 2. Linear

### 2.1 Core2 project

| Action | Issue | Notes |
|--------|-------|-------|
| Comment + leave Done | TAP-7471–7474 | “Superseded by VISION.md; host-bridge architecture retired” |
| Cancel or rewrite | TAP-7484 | Cancel as obsolete host-bridge ask **or** retitle to firmware-direct + blocked on AF fleet SSE |
| **Create** | Epic: Core2 direct AF monitor | Parent for cleanup + firmware stories |
| **Create** | Story: delete host middleware | Phase B |
| **Create** | Story: firmware AF HTTP client | Phase C poll path |
| **Create** | Story: firmware AF alert SSE | Phase C blocked by AF |
| **Create** | Story: secrets.h AF_BASE_URL + slug + afp_ + Wi-Fi | Phase C |

Assignee: `Claude Agent`. Project: **Core2**.

### 2.2 AgentForge Platform (API gaps only — no AF code from Core2)

| **Create** | Title (draft) | Ask |
|------------|---------------|-----|
| AF | Fleet monitor SSE for desk HMI | `GET /events/monitor` (name TBD) with platform Bearer; emit invocation/run failures across all projects; ADR-015 compliant (AF pushes SSE, Core2 consumes) |
| AF | Mint `afp_` project key for desk client | Operator paste into `secrets.h`; never put `af_*`/admin on device |

Link AF issues from Core2 epic as blockers.

### 2.3 Verify

- No open Core2 issue still requiring `af_bridge`.  
- AF issues visible on AgentForge Platform backlog for review.

---

## 3. Code — Phase B (cleanup / delete)

**Do first** so nobody keeps running the bridge.

1. Stop any running `af_bridge` process.  
2. Delete:
   - `scripts/af_bridge.py`
   - `scripts/af_protocol.py`
   - `tests/test_af_bridge.py`  
3. Trim `requirements.txt` if httpx was only for bridge (keep if firmware tests need it; likely drop httpx/pytest bridge-only — keep pytest + flash tooling).  
4. Update `secrets.h.example` → Wi-Fi + `AF_BASE_URL` + `AF_PROJECT_SLUG` + `AF_API_KEY` (`afp_`).  
5. Stub or rewrite `net.cpp` so it **does not** call `/v1/state` (even if AF client incomplete: fail closed / “NO AF” UI).

**Verify:** `rg af_bridge` empty in `scripts/` and `tests/`; firmware builds.

---

## 4. Code — Phase C (implement direct AF client)

### 4.1 Firmware modules (proposed)

```text
firmware/src/
  config.h / secrets.h     CORE2_FW_VERSION, AF_BASE_URL, AF_PROJECT_SLUG, AF_API_KEY (afp_), WIFI_*
  af_client.h/.cpp         HTTP GET health/ready + /projects/{slug}/… (not fleet /stats/*)
  protocol.h/.cpp          AppState + UI mapping only (no host NDJSON)
  net.h/.cpp               Wi-Fi STA + call af_client
  ui.*                     ops-cube faces Hub/Pulse/Heat/Trail/Beam (320×240)
  main.cpp                 buttons, haptic, loop
```

### 4.2 Poll loop (unblocked now)

Every N seconds (e.g. 5s):

1. `GET {AF_BASE_URL}/health` → reachable, version, degraded  
2. `GET {AF_BASE_URL}/ready` → ready  
3. `GET {AF_BASE_URL}/projects/{slug}` + Bearer `afp_` → auth ok  
4. `GET {AF_BASE_URL}/projects/{slug}/stats` → inv, err, cost, agent  
5. `GET {AF_BASE_URL}/projects/{slug}/activity-series?days=7` → Trail series  
6. `GET {AF_BASE_URL}/projects/{slug}/dual-meters` + `invocations?limit=5` → Heat  

Map into existing `SnapState` / `FailState` / `SeriesState`.  
On 401: Hub shows Need key (honest), not fake zeros.  
Do **not** call fleet `/stats/*` with `afp_` (403).

### 4.3 Alerts (blocked on AF)

- Until AF fleet SSE exists: optional **poll-derived** alert when failures `n24` rises (on-device), documented as stopgap.  
- When AF ships SSE: `af_client` SSE task → haptic path (reuse UI banner).

### 4.4 Ack / mute

- Mute: local only (existing).  
- Ack: clear local banner; call AF only if AF adds an ack endpoint (separate AF issue).

### 4.5 Verify

1. `python scripts/core2_dev.py build` (and `upload --port COM4`).  
2. `secrets.h` with Wi-Fi + `AF_BASE_URL` + `AF_PROJECT_SLUG` + mint `afp_` key.  
3. Brick Status shows READY + version **without** any PC Python bridge.  
4. Process list: no `af_bridge`.  
5. With key: non-zero stats when AF has traffic.  
6. Without key: DEGRADED / need key, not DOWN if `/health` works.

---

## 5. Phase D (later — after C useful)

- NVS Wi-Fi + key setup UI  
- Mute schedule  
- OTA  

Do **not** start D until C is desk-useful.

---

## 6. Execution order (do in sequence)

| Step | Work | Exit criteria |
|------|------|----------------|
| **S0** | Publish this plan; link from PLAN.md + INDEX | File exists; VISION still wins |
| **S1** | Linear: Core2 epic/stories; AF Platform API issues; fix TAP-7484 | IDs recorded in PLAN |
| **S2** | Docs pass (kb README, llms, .env.example, handoff, memory) | rg clean except intentional |
| **S3** | Phase B delete middleware + secrets example + net stub | Build green; no af_bridge |
| **S4** | Phase C AF HTTP poll client + flash COM4 | Live Status from AF, no PC bridge |
| **S5** | Poll-alert stopgap (optional) | Failures rise → haptic |
| **S6** | Hand off; AF SSE story waits on AF issue | Handoff + Linear blockers |

---

## 7. Out of scope (hard)

- Implementing AgentForge endpoints in `C:\code\AgentForge` from this repo  
- Recreating host NDJSON / `/v1/state` bridge  
- Phase D polish before C works  

---

## 8. Success (end state)

Matches VISION.md “Success looks like”:

1. Brick on Wi‑Fi, no `af_bridge`.  
2. Status from `AF_BASE_URL` health/ready/projects/stats.  
3. Alerts from AF API (or documented stopgap + AF Linear).  
4. Docs/Linear/code all tell the same story.
