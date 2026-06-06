#include "src/ui/header_title.h"

#include "src/config.h"
#include "src/state.h"

namespace HeaderTitle {

static constexpr int kMaxLen = 31;
static constexpr const char* kNvsKey = "cfg_hdr_title";

static char s_title[kMaxLen + 1] = "";
static bool s_custom = false;

void loadSettings() {
  if (prefs.isKey(kNvsKey)) {
    s_custom = true;
    String val = prefs.getString(kNvsKey, "");
    strncpy(s_title, val.c_str(), kMaxLen);
    s_title[kMaxLen] = '\0';
  } else {
    s_custom = false;
    strncpy(s_title, LIB_HEADER_TITLE, kMaxLen);
    s_title[kMaxLen] = '\0';
  }
}

const char* current() {
  return s_title;
}

void set(const char* title) {
  if (title == nullptr) {
    resetToDefault();
    return;
  }
  s_custom = true;
  strncpy(s_title, title, kMaxLen);
  s_title[kMaxLen] = '\0';
  prefs.putString(kNvsKey, s_title);
}

void resetToDefault() {
  s_custom = false;
  prefs.remove(kNvsKey);
  strncpy(s_title, LIB_HEADER_TITLE, kMaxLen);
  s_title[kMaxLen] = '\0';
}

}  // namespace HeaderTitle
