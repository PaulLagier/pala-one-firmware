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

TEST_CASE("GET /files?error=not-found renders the warning banner") {
  auto res = gCli->Get("/files?error=not-found");
  REQUIRE(res);
  CHECK_EQ(res->status, 200);
  CHECK(res->body.find("Text not found in book") != std::string::npos);
}

TEST_CASE("GET /files without error flag has no warning banner") {
  auto res = gCli->Get("/files");
  REQUIRE(res);
  CHECK_EQ(res->status, 200);
  CHECK(res->body.find("Text not found in book") == std::string::npos);
}

// ---------------------------------------------------------------------------
//  /jumptext — validation
// ---------------------------------------------------------------------------

TEST_CASE("POST /jumptext missing id returns 400") {
  auto res = post("/jumptext", {{"query", "detective"}});
  REQUIRE(res);
  CHECK_EQ(res->status, 400);
}

TEST_CASE("POST /jumptext missing query returns 400") {
  auto res = post("/jumptext", {{"id", "0"}});
  REQUIRE(res);
  CHECK_EQ(res->status, 400);
}

TEST_CASE("POST /jumptext empty query returns 400") {
  auto res = post("/jumptext", {{"id", "0"}, {"query", ""}});
  REQUIRE(res);
  CHECK_EQ(res->status, 400);
}

TEST_CASE("POST /jumptext out-of-range id returns 400") {
  auto res = post("/jumptext", {{"id", "999"}, {"query", "detective"}});
  REQUIRE(res);
  CHECK_EQ(res->status, 400);
}

// ---------------------------------------------------------------------------
//  /jumptext — happy path and not-found
// ---------------------------------------------------------------------------

TEST_CASE("POST /jumptext text found redirects to /files") {
  auto res = post("/jumptext", {{"id", "0"}, {"query", "detective"}});
  REQUIRE(res);
  CHECK_EQ(res->status, 302);
  CHECK_EQ(locationOf(*res), "/files");
}

TEST_CASE("POST /jumptext text not found redirects to /files?error=not-found") {
  auto res = post("/jumptext", {{"id", "0"}, {"query", "elephant"}});
  REQUIRE(res);
  CHECK_EQ(res->status, 302);
  CHECK_EQ(locationOf(*res), "/files?error=not-found");
}

// ---------------------------------------------------------------------------
//  End-to-end: saved offset matches find_text on the raw file
// ---------------------------------------------------------------------------

// Walk pages in `f` to find the start offset of the page containing `matchOffset`.
static uint32_t pageStartFor(File& f, uint32_t matchOffset) {
  uint32_t pageStart = 0;
  while (true) {
    uint32_t next = nextPageOffset(f, pageStart);
    if (next <= pageStart || next > matchOffset) break;
    pageStart = next;
  }
  return pageStart;
}

TEST_CASE("POST /jumptext saves the correct byte offset") {
  const String query = "lantern";

  auto res = post("/jumptext", {{"id", "0"}, {"query", query.c_str()}});
  REQUIRE(res);
  CHECK_EQ(res->status, 302);
  CHECK_EQ(locationOf(*res), "/files");

  // Expected: start of the page containing the raw match.
  String content = readFile("/books/hound.txt");
  REQUIRE(content.length() > 0);
  uint32_t matchOffset = find_text(content, query);
  REQUIRE(matchOffset != 0xFFFFFFFFu);
  File f = FS.open("/books/hound.txt", "r");
  REQUIRE(f);
  uint32_t expected = pageStartFor(f, matchOffset);
  f.close();

  String key = prefKeyForBook(String("/books/hound.txt"));
  PreferencesStore kv(prefs);
  CHECK_EQ(loadSavedOffset(kv, key), expected);
}

TEST_CASE("POST /jumptext not-found does not overwrite a previously saved offset") {
  const String goodQuery = "butler";
  auto r1 = post("/jumptext", {{"id", "0"}, {"query", goodQuery.c_str()}});
  REQUIRE(r1);
  REQUIRE(r1->status == 302);

  String content = readFile("/books/hound.txt");
  uint32_t matchOffset = find_text(content, goodQuery);
  REQUIRE(matchOffset != 0xFFFFFFFFu);
  File f = FS.open("/books/hound.txt", "r");
  REQUIRE(f);
  uint32_t expected = pageStartFor(f, matchOffset);
  f.close();

  // A failed search must not clobber the stored offset.
  auto r2 = post("/jumptext", {{"id", "0"}, {"query", "elephant"}});
  REQUIRE(r2);
  REQUIRE(r2->status == 302);

  String key = prefKeyForBook(String("/books/hound.txt"));
  PreferencesStore kv(prefs);
  CHECK_EQ(loadSavedOffset(kv, key), expected);
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
