# Core2

Public workspace for **M5Stack Core2** hardware knowledge and engineering
work.

- GitHub: https://github.com/wtthornton/Core2
- Linear: https://linear.app/tappscodingagents/project/core2-7d22b9d8efd1 (team **TappsCodingAgents**, project **Core2**)
- Hardware KB: [docs/kb/README.md](docs/kb/README.md)
- USB flash / load firmware: [docs/kb/usb-flash.md](docs/kb/usb-flash.md)
- Doc index: [docs/INDEX.md](docs/INDEX.md)
- Agent summary: [llms.txt](llms.txt)

Host probe (Windows, after CP210x or CH9102 VCP driver): `pip install -r requirements.txt` then `python scripts/core2_usb.py`.

AgentForge desk monitor (**brick → AF direct**, no PC bridge):
[docs/firmware/VISION.md](docs/firmware/VISION.md).
Plan / cleanup: [docs/firmware/PLAN.md](docs/firmware/PLAN.md).
AF API usage: [docs/firmware/PROTOCOL.md](docs/firmware/PROTOCOL.md).
Build/flash: `python scripts/core2_dev.py upload --port COM4`.

## Cursor

Project rules, skills, and MCP config live under `.cursor/`.

- Linear MCP: `https://mcp.linear.app/mcp` (OAuth on first use)
- Local NLT fleet: `127.0.0.1:8760-8765` with `X-Tapps-Project-Root` pointing at this repo

After cloning, reload the Cursor window if MCP servers do not appear (**Ctrl+Shift+P → Developer: Reload Window**). Enable **linear** in **Settings → Tools & MCP** and complete OAuth when prompted.

## Linear

| Field | Value |
|-------|-------|
| Team | TappsCodingAgents (`TAP`) |
| Project | Core2 |
| Agent assignee | Claude Agent |

See `AGENTS.md` and `.cursor/rules/linear.mdc` for the issue workflow.
