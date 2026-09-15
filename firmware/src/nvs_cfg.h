#pragma once

#include "protocol.h"

// Compile-time secrets.h values are factory defaults. NVS wins when present.
void nvs_cfg_load(NetCfg& net);
bool nvs_cfg_save(const NetCfg& net);
bool nvs_cfg_save_quiet(const NetCfg& net);
void nvs_cfg_wipe(NetCfg& net);
