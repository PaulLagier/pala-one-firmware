#include "src/storage/navigation_profile.h"
#include "src/state.h"

namespace NavigationProfile {
namespace {
constexpr const char *key = "cfg_read_menu";
bool reading = false;
} // namespace
void loadSettings() { reading = prefs.getBool(key, false); }
bool readingFocused() { return reading; }
bool setReadingFocused(bool enabled) {
  if (enabled == reading)
    return true;
  if (prefs.putBool(key, enabled) != sizeof(bool))
    return false;
  reading = enabled;
  return true;
}
int filterSystemEntries(LibraryEntryType *entries, int count) {
  if (!entries || count <= 0)
    return 0;
  if (!reading)
    return count;
  int used = 0;
  for (int i = 0; i < count; ++i) {
    const auto entry = entries[i];
    if (entry == LIB_ENTRY_BOOKMARKS || entry == LIB_ENTRY_UPLOAD || entry == LIB_ENTRY_ABOUT)
      entries[used++] = entry;
  }
  return used;
}
} // namespace NavigationProfile
