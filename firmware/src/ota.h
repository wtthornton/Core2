#pragma once

// HTTP OTA from USB `cfg ota http://host/firmware.bin`. http only (LAN).
void ota_run(const char* url);
