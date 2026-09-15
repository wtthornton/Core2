# TOOLS.md — Core2 desk HMI

## Flash / build

```text
python scripts/core2_dev.py build
python scripts/core2_dev.py upload --port COM4
```

USB serial 115200 is debug only. Config persist:

```text
cfg show
cfg ssid … / cfg pass … / cfg url … / cfg slug … / cfg key afp_…
cfg save
cfg wipe
```

## AgentForge consumer

Project-scoped `afp_` routes only: `/health`, `/ready`, `/projects/{slug}`,
`/stats`, `activity-series`, `dual-meters`, `invocations`, `/events`.

## Linear

Team `TappsCodingAgents`, project **Core2**. Writes go through the linear-issue
skill. AF API gaps belong on **AgentForge Platform**.
