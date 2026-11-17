#include "test_framework.h"
#include "src/state.h"
#include "src/ui/lock.h"
#include "src/ui/statusbar.h"
#include "src/web/display_settings.h"
#include "src/web/reading_settings.h"
#include "src/ui/font.h"
LockPrefs prefs;
DisplayServer server;
DisplayScreen screen;
DisplayScreen *g_currentScreen = &screen;
int invalidations = 0, dirty = 0, fullFrames = 0;
namespace Font {
void invalidateLayoutCache() { ++invalidations; }
} // namespace Font
void markPagesDirtyForLayoutChange() { ++dirty; }
void forceNextMenuFrameFull() { ++fullFrames; }
namespace {
void reset() {
  prefs.values.clear();
  Lock::loadSettings();
  Statusbar::loadSettings();
  server = DisplayServer{};
  screen = DisplayScreen{};
  g_currentScreen = &screen;
  invalidations = dirty = fullFrames = 0;
}
} // namespace
TEST_CASE("Other forms cannot change display settings") {
  reset();
  server.args = {{"statusbar", "2"}, {"font", "14"}};
  CHECK(!handleDisplaySettingsPost());
  CHECK_EQ(server.status, 0);
  CHECK_EQ(Statusbar::mode(), Statusbar::Full);
  CHECK(Lock::iconEnabled());
  CHECK(prefs.values.empty());
}
TEST_CASE("Malformed display form is rejected before any preference or layout change") {
  for (const char *mode : {"", "-1", "3", "2oops", " 1", "1.0", "01", "junk"}) {
    reset();
    server.args = {{"display_form", "1"}, {"statusbar", mode}};
    REQUIRE(handleDisplaySettingsPost());
    CHECK_EQ(server.status, 400);
    CHECK(prefs.values.empty());
    CHECK_EQ(invalidations, 0);
    CHECK_EQ(dirty, 0);
    CHECK_EQ(screen.draws, 0);
    CHECK(Lock::iconEnabled());
  }
}
TEST_CASE("Display modes persist and invalidate layout only when geometry changes") {
  for (int mode = 0; mode < 3; ++mode) {
    reset();
    Lock::engage();
    server.args = {{"display_form", "1"}, {"statusbar", std::to_string(mode)}};
    REQUIRE(handleDisplaySettingsPost());
    CHECK_EQ(server.status, 303);
    CHECK(server.location == "/settings");
    CHECK_EQ(screen.draws, 1);
    CHECK_EQ(fullFrames, 1);
    CHECK_EQ(dirty, mode == 0 ? 0 : 1);
    CHECK_EQ(invalidations, dirty);
    CHECK_EQ(Statusbar::reserveH(), mode == 0 ? 8 : mode == 1 ? 1 : 0);
    CHECK(Lock::isLocked());
    CHECK(!Lock::iconEnabled());
    Statusbar::loadSettings();
    Lock::loadSettings();
    CHECK_EQ(int(Statusbar::mode()), mode);
    CHECK(!Lock::iconEnabled());
    CHECK(Lock::isLocked());
    // No unrelated reading or control preferences were written.
    for (const auto &p : prefs.values)
      CHECK(p.first == "cfg_statusbar" || p.first == "cfg_locked" || p.first == "cfg_lockicon");
    server.args["lock_icon"] = "1";
    g_currentScreen = nullptr;
    REQUIRE(handleDisplaySettingsPost());
    CHECK(Lock::iconEnabled());
    CHECK_EQ(invalidations, dirty);
    CHECK_EQ(dirty, mode == 0 ? 0 : 1);
  }
}

TEST_CASE("Saving gestures or another settings card preserves bionic and paragraph gaps") {
  for (const char *field : {"btnQ", "btnS", "wifi_form", "icons_form", "hdr", "timings_form"}) {
    reset();
    Font::bionic = Font::halfGaps = true;
    Font::writes = 0;
    server.args = {{field, "1"}};
    CHECK(!applyReadingSettingsForm());
    CHECK(Font::bionicEnabled());
    CHECK(Font::halfParagraphGapsEnabled());
    CHECK_EQ(Font::writes, 0);
  }
}
TEST_CASE("Reading form can explicitly clear both unchecked reading checkboxes") {
  reset();
  Font::bionic = Font::halfGaps = true;
  Font::writes = 0;
  server.args = {{"font", "12"}};
  CHECK(applyReadingSettingsForm());
  CHECK(!Font::bionicEnabled());
  CHECK(!Font::halfParagraphGapsEnabled());
  CHECK_EQ(Font::writes, 2);
  CHECK(!applyReadingSettingsForm());
  CHECK_EQ(Font::writes, 2);
}
