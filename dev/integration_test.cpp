// Integration tests for the /jumptext route and surrounding pipeline.
//
// Starts the web emulator in a background thread, then drives it with a
// real HTTP client. Every test is a full round-trip through the actual web
// handler, storage layer, and stub FS — no mocks are inserted in the path.
//
// Run from the repo root (so "dev/fs_test" resolves):
//   ./dev/build/pala_web_test
// Or set PALA_EMU_FS_ROOT to point at a different fixture directory.

// httplib must be included before any arduino_compat.h header so the
// min/max macros it defines don't clobber httplib's use of std::min/max.
// prelude.h is force-included via -include, so system headers are already
// parsed before this TU touches anything.
#include <httplib.h>

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>

#include "test_framework.h"

#include "src/pure/find_text.h"
#include "src/storage/book_metadata.h"
#include "src/ui/text.h"
#include "src/storage/preferences_store.h"
#include "src/storage/library.h"
#include "src/storage/app_catalog.h"
#include "src/state.h"
#include "stubs/fs_stub.h"
#include "stubs/web_server.h"
#include "src/web/web.h"

extern void loadListItems();

static constexpr int kPort = 18080;

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

static httplib::Client* gCli = nullptr;

static std::string locationOf(const httplib::Response& res) {
  auto it = res.headers.find("Location");
  return it != res.headers.end() ? it->second : "";
}

static httplib::Result post(const char* path, httplib::Params params) {
  return gCli->Post(path, params);
}

// Read the full contents of a virtual-FS file into a String.
static String readFile(const char* path) {
  File f = FS.open(path, "r");
  String out;
  if (!f) return out;
  uint8_t buf[256];
  size_t n;
  while ((n = f.read(buf, sizeof(buf))) > 0)
    out.concat((const char*)buf, (unsigned)n);
  f.close();
  return out;
}

// ---------------------------------------------------------------------------
//  /files page
// ---------------------------------------------------------------------------

TEST_CASE("/files renders the Find-in-book form") {
  auto res = gCli->Get("/files");
  REQUIRE(res);
  CHECK_EQ(res->status, 200);
  CHECK(res->body.find("jumptext") != std::string::npos);
  CHECK(res->body.find("Find in book") != std::string::npos);
}

// ---------------------------------------------------------------------------
//  Entry point — starts the server, runs tests, then shuts down cleanly.
// ---------------------------------------------------------------------------

int main() {
  const char* env = std::getenv("PALA_EMU_FS_ROOT");
  std::string fsRoot = (env && env[0]) ? env : "dev/fs_test";
  LittleFS.setRoot(fsRoot);
  loadBooks();
  loadApps();
  loadListItems();
  registerWebRoutes();

  std::thread srv([]() { server.run(kPort); });
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  httplib::Client cli("localhost", kPort);
  cli.set_follow_location(false);
  gCli = &cli;

  int passed = 0, failed = 0;
  for (auto& tc : tf_allTests()) {
    tf_currentFailures() = 0;
    std::printf("[ RUN  ] %s\n", tc.name);
    tc.fn();
    if (tf_currentFailures() == 0) {
      std::printf("[  OK  ] %s\n", tc.name);
      ++passed;
    } else {
      std::printf("[ FAIL ] %s (%d failures)\n", tc.name, tf_currentFailures());
      tf_totalFailures() += tf_currentFailures();
      ++failed;
    }
  }
  std::printf("\n%zu tests, %d checks, %d passed, %d failed\n",
              tf_allTests().size(), tf_totalChecks(), passed, failed);

  server.stop();
  srv.join();
  return failed == 0 ? 0 : 1;
}
