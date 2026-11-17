#pragma once
#include "Arduino.h"
#include <functional>
#include <map>
#include <string>
constexpr int HTTP_GET = 1, HTTP_POST = 2;
struct WebServer {
  std::map<std::pair<int, std::string>, std::function<void()>> routes;
  std::map<std::string, String> args, headers;
  String body;
  int status = 0;
  bool hasArg(const char *name) { return args.count(name) != 0; }
  String arg(const char *name) {
    auto p = args.find(name);
    return p == args.end() ? String() : p->second;
  }
  void on(const char *path, int method, std::function<void()> handler) {
    routes[{method, path}] = handler;
  }
  void send(int code, const char *, const String &text) {
    status = code;
    body = text;
  }
  void sendHeader(const char *name, const String &value) { headers[name] = value; }
  void request(const char *path, int method, std::map<std::string, String> fields = {}) {
    args = std::move(fields);
    headers.clear();
    body = "";
    status = 0;
    routes.at({method, path})();
  }
};
inline WebServer server;
