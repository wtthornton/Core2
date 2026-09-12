# Core2 vision: AgentForge desk monitor

**Status:** locked 2026-09-12. If anything in the repo disagrees with this file, **this file wins**.

## One sentence

The **M5Stack Core2 brick talks directly to AgentForge** and shows ops status on its screen. There is **no** PC middleware in the product path.

## What we are building

A desk-side HMI that runs **on Core2 firmware** (C++ / M5Unified):

1. **Hub** — live orb + isometric cube + metric tiles  
2. **Pulse / Heat / Trail / Beam** — cube faces (invokes, failures, 7-day charts, link)  

Nav: top face chips, swipe, or **Prev / Quiet / Next** (BtnA/B/C). Not a CLI.

The brick joins Wi‑Fi, calls AgentForge over HTTP (and SSE when AF provides it), and renders the result. USB is for **flash and debug logs only**.

## What we are not building

- A host Python “bridge” that polls AF and pushes state to the brick  
- Serial NDJSON as the ops data bus  
- A LAN mini-server on the PC (`/v1/state`) that the brick must depend on  
- Patches to AgentForge (or any sibling repo) from the Core2 workspace  

If AF is missing an API the brick needs, we **file Linear on AgentForge Platform** for design/implementation. We wait or show an honest empty/error state. We do **not** invent Core2-side middleware to paper over the gap.

## Ownership (hard)

| Owns | Does not own |
|------|----------------|
| `C:\code\Core2` code, docs, Core2 Linear | AgentForge code |
| Device UX, firmware, on-device secrets layout | Host daemon required for monitoring |
| Consumer contract written as AF Linear issues | Implementing those AF issues |

## Trust / secrets

- AF base URL + project slug + **`afp_` project key** live in **device** config (`secrets.h` / later NVS).  
- The brick is the AF client. Keys are not “kept on the PC for the product path.”  
- Use **`afp_` only** on device — never platform `af_*` or admin keys.  
- Wi‑Fi stays private (`secrets.h` gitignored). The desk `afp_` key is a project credential, not admin.

## Target runtime

```text
[ AgentForge :8010 ]  <--- HTTPS/HTTP + SSE ---  [ Core2 on Wi-Fi ]
                                                      ^
                                                      | USB only for flash/debug
                                                   [ PC ]
```

Not this (rejected):

```text
[ AgentForge ] <-- PC bridge --> [ Core2 USB/LAN ]
```

## Docs map

| Doc | Role |
|-----|------|
| **This file** | Product / architecture vision (source of truth) |
| [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) | Docs + Linear + code + verify execution plan |
| [PLAN.md](PLAN.md) | Phased work + cleanup of the old bridge approach |
| [PROTOCOL.md](PROTOCOL.md) | Device ↔ AF API usage (direct) |
| `.cursor/rules/core2-af-direct.mdc` | Agent hard rule |

## Success looks like

1. Power the brick on the desk (Wi‑Fi configured).  
2. No `af_bridge.py` (or similar) running on a PC.  
3. Status shows live AF version / ready / stats from `AF_BASE_URL` (auth via `afp_`).  
4. Alerts come from AF’s API surface (fleet stream when AF ships it).  
5. Gaps are tracked as **AgentForge Platform** Linear issues, not new Core2 middleware.
