#include "test_framework.h"
#include "src/state.h"
#include "src/storage/navigation_profile.h"
#include <vector>
ProfilePrefs prefs;
TEST_CASE("navigation view preserves stored flags and restores the complete custom order") {
  prefs.flags.clear();
  prefs.failWrites = false;
  NavigationProfile::loadSettings();
  CHECK(!NavigationProfile::readingFocused());
  const std::vector<LibraryEntryType> order = {
      LIB_ENTRY_UPLOAD, LIB_ENTRY_APPS,  LIB_ENTRY_SETTINGS, LIB_ENTRY_BOOKMARKS,
      LIB_ENTRY_LIST,   LIB_ENTRY_ABOUT, LIB_ENTRY_UPDATE,   LIB_ENTRY_STATISTICS};
  auto visible = order;
  CHECK_EQ(NavigationProfile::filterSystemEntries(visible.data(), int(visible.size())), 8);
  CHECK(visible == order);
  prefs.flags["unrelated-setting"] = true;
  REQUIRE(NavigationProfile::setReadingFocused(true));
  visible = order;
  CHECK_EQ(NavigationProfile::filterSystemEntries(visible.data(), int(visible.size())), 3);
  CHECK_EQ(visible[0], LIB_ENTRY_UPLOAD);
  CHECK_EQ(visible[1], LIB_ENTRY_BOOKMARKS);
  CHECK_EQ(visible[2], LIB_ENTRY_ABOUT);
  CHECK(prefs.flags["unrelated-setting"]);
  NavigationProfile::loadSettings();
  CHECK(NavigationProfile::readingFocused());
  REQUIRE(NavigationProfile::setReadingFocused(false));
  visible = order;
  CHECK_EQ(NavigationProfile::filterSystemEntries(visible.data(), int(visible.size())), 8);
  CHECK(visible == order);
}
TEST_CASE("navigation persistence failure keeps the active view unchanged") {
  prefs.flags.clear();
  prefs.failWrites = false;
  NavigationProfile::loadSettings();
  prefs.failWrites = true;
  CHECK(!NavigationProfile::setReadingFocused(true));
  CHECK(!NavigationProfile::readingFocused());
  CHECK_EQ(NavigationProfile::filterSystemEntries(nullptr, 8), 0);
  LibraryEntryType entry = LIB_ENTRY_UPLOAD;
  CHECK_EQ(NavigationProfile::filterSystemEntries(&entry, 0), 0);
  prefs.failWrites = false;
}
