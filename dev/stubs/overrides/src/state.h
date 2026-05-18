#pragma once

// Host-build replacement for Pala_One_2_1/src/state.h.
// Declares the same globals but backed by emulator stubs.

#include "src/config.h"
#include "stubs/web_server.h"
#include "stubs/fs_stub.h"
#include "Preferences.h"

extern WebServerStub server;
extern Preferences   prefs;

// FS macro mirrors the real state.h convention.
#define FS LittleFS

// WiFi credential placeholders (some web handlers reference AP_SSID).
extern char AP_SSID[24];
extern const char* AP_PASS;

// Arduino built-in stubs.
inline void delay(unsigned long) {}
inline unsigned long millis() { return 0; }
