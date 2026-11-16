#include "src/pure/utf8_clusters.h"
#include <string.h>
#include "src/ui/header_title.h"

#include "src/config.h"
#include "src/state.h"

namespace HeaderTitle {

static constexpr int kMaxLen = 31;
static constexpr const char* kNvsKey = "cfg_hdr_title";

static char s_title[kMaxLen + 1] = "";
static bool s_custom = false;
static void copyTitle(const char* title) {
  size_t length=strlen(title), end=0;
  while(end<length) {
    uint32_t cp=0;size_t bytes=PalaUtf8::decode(title,length,end,cp);
    if(!bytes||(static_cast<unsigned char>(title[end])>=0x80&&bytes==1))break;
    size_t cluster=PalaUtf8::clusterBytes(title,length,end);
    if(end+cluster>kMaxLen)break;
    // Reject malformed bytes inside the cluster too.
    end+=cluster;
  }
  memcpy(s_title,title,end);s_title[end]='\0';
}


void loadSettings() {
  if (prefs.isKey(kNvsKey)) {
    s_custom = true;
    String val = prefs.getString(kNvsKey, "");
    copyTitle(val.c_str());
  } else {
    s_custom = false;
    copyTitle(LIB_HEADER_TITLE);
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
  copyTitle(title);
  prefs.putString(kNvsKey, s_title);
}

void resetToDefault() {
  s_custom = false;
  prefs.remove(kNvsKey);
  copyTitle(LIB_HEADER_TITLE);
}

}  // namespace HeaderTitle
