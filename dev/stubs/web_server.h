#pragma once

#include <functional>
#include <memory>
#include <string>

#include "src/pure/arduino_compat.h"

enum HTTPMethod { HTTP_ANY, HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_PATCH, HTTP_DELETE, HTTP_OPTIONS };

using THandlerFunction = std::function<void()>;

// Forward declaration — implementation detail in web_server.cpp.
struct WebServerImpl;

// Drop-in stub for Arduino's WebServer, backed by cpp-httplib.
// httplib.h is intentionally NOT included here to avoid macro conflicts.
class WebServerStub {
public:
  WebServerStub();
  ~WebServerStub();

  // Route registration.
  void on(const char* uri, int method, THandlerFunction handler);
  // Streaming-upload variant — returns 501 in the emulator.
  void on(const char* uri, int method, THandlerFunction done, THandlerFunction stream);

  // Response helpers — callable only from within a handler.
  void send(int code, const char* contentType, const String& body);
  void send_P(int code, const char* contentType, const char* progmemStr);
  void sendHeader(const char* name, const String& value);

  // Request helpers — callable only from within a handler.
  String arg(const String& name);
  bool   hasArg(const String& name);

  // No-ops called by firmware during normal startup/loop.
  void begin() {}
  void handleClient() {}

  // Start the blocking HTTP server loop.
  void run(int port = 8080);

  // Stop the server (unblocks run()). Safe to call from another thread.
  void stop();

private:
  std::unique_ptr<WebServerImpl> impl_;
};
