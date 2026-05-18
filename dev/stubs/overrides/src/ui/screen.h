#pragma once

// Minimal Screen stub — list.cpp checks g_currentScreen == &g_listScreen
// after a POST to decide whether to redirect the device display.
// In the emulator this check always evaluates to false; no navigation occurs.

struct Screen {
  Screen* nextScreen = nullptr;
};

extern Screen* g_currentScreen;
