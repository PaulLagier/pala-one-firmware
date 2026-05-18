#include "stubs/web_server.h"
#include "stubs/fs_stub.h"
#include "Preferences.h"
#include "src/ui/screen.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/screens/list_screen.h"

WebServerStub server;
Preferences   prefs;

char        AP_SSID[24] = "PalaOneEmu";
const char* AP_PASS     = "palaone";

// Screen stubs for list.cpp's post-save navigation check.
Screen  g_libraryScreen;
Screen  g_listScreen;
Screen* g_currentScreen = nullptr;
