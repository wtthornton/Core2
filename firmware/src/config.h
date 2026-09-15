#pragma once

#if __has_include("secrets.h")
#include "secrets.h"
#endif

// Desk HMI firmware revision (docs / Linear / flash notes).
#ifndef CORE2_FW_VERSION
#define CORE2_FW_VERSION "0.3.5"
#endif

// POSIX TZ for NTP + quiet hours (desk is Pacific).
#ifndef CORE2_TZ
#define CORE2_TZ "PST8PDT,M3.2.0/2,M11.1.0/2"
#endif

#ifndef CORE2_WIFI_SSID
#define CORE2_WIFI_SSID ""
#endif
#ifndef CORE2_WIFI_PASS
#define CORE2_WIFI_PASS ""
#endif

// Handoff contract (AgentForge mint paste): AF_BASE_URL + AF_PROJECT_SLUG + AF_API_KEY (afp_…).
#ifndef AF_BASE_URL
#ifdef AF_URL
#define AF_BASE_URL AF_URL
#else
#define AF_BASE_URL "http://192.168.1.207:8010"
#endif
#endif
#ifndef AF_URL
#define AF_URL AF_BASE_URL
#endif

#ifndef AF_PROJECT_SLUG
#define AF_PROJECT_SLUG "core2"
#endif

#ifndef AF_API_KEY
#define AF_API_KEY ""
#endif
