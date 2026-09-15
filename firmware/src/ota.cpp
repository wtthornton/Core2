#include "ota.h"

#include <HTTPUpdate.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp32-hal-cpu.h>

void ota_run(const char* url) {
  if (url == nullptr) {
    Serial.println(F("ota need http:// URL"));
    return;
  }
  while (*url == ' ') {
    ++url;
  }
  if (strncmp(url, "http://", 7) != 0) {
    Serial.println(F("ota need http:// URL (no https)"));
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("ota need wifi"));
    return;
  }

  setCpuFrequencyMhz(240);
  Serial.println(F("ota unsigned http LAN; USB flash is recovery"));
  Serial.printf("ota %s\n", url);
  WiFiClient client;
  httpUpdate.rebootOnUpdate(true);
  httpUpdate.onProgress([](int cur, int total) {
    static int last = -1;
    const int pct = total > 0 ? (cur * 100) / total : 0;
    if (pct != last && pct % 10 == 0) {
      last = pct;
      Serial.printf("ota %d%%\n", pct);
    }
  });
  const t_httpUpdate_return ret = httpUpdate.update(client, String(url), "");
  if (ret == HTTP_UPDATE_OK) {
    Serial.println(F("ota ok"));
    return;
  }
  Serial.printf("ota fail %s\n", httpUpdate.getLastErrorString().c_str());
}
