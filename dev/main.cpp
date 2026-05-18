#include <cstdlib>
#include <cstring>
#include "stubs/web_server.h"
#include "src/web/web.h"

extern void loadMockData();
extern WebServerStub server;

int main(int argc, char* argv[]) {
  int port = 8080;
  for (int i = 1; i < argc; i++) {
    if ((std::strcmp(argv[i], "--port") == 0 || std::strcmp(argv[i], "-p") == 0) && i + 1 < argc) {
      port = std::atoi(argv[++i]);
    } else if (std::strncmp(argv[i], "--port=", 7) == 0) {
      port = std::atoi(argv[i] + 7);
    }
  }
  loadMockData();
  registerWebRoutes();
  server.run(port);
  return 0;
}
