#pragma once
#include "../lock/stubs/src/state.h"
#include "src/pure/arduino_compat.h"
#include <map>
#include <string>
struct DisplayServer {
  std::map<std::string, std::string> args;
  int status = 0;
  std::string location;
  bool hasArg(const char *key) const { return args.count(key) != 0; }
  String arg(const char *key) const {
    auto it = args.find(key);
    return it == args.end() ? String() : String(it->second.c_str());
  }
  void send(int code, const char *, const char *) { status = code; }
  void sendHeader(const char *, const char *value) { location = value; }
};
struct DisplayScreen {
  int draws = 0;
  void draw() { ++draws; }
};
extern DisplayServer server;
extern DisplayScreen *g_currentScreen;
