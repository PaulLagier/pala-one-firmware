#include "src/storage/library_menu_order.h"

#include <cstdlib>
#include <cstring>

#include "src/state.h"
#include "src/pure/library_nav.h"
namespace LibraryMenuOrder {

static constexpr const char* kNvsKey = "cfg_lib_order";

static const LibraryEntryType kDefaultOrder[kMaxSystemEntries] = {
    LIB_ENTRY_BOOKMARKS,
    LIB_ENTRY_APPS,
    LIB_ENTRY_STATISTICS,
    LIB_ENTRY_LIST,
    LIB_ENTRY_ABOUT,
    LIB_ENTRY_UPLOAD,
    LIB_ENTRY_UPDATE,
};

static LibraryEntryType s_order[kMaxSystemEntries] = {
    LIB_ENTRY_BOOKMARKS,
    LIB_ENTRY_APPS,
    LIB_ENTRY_STATISTICS,
    LIB_ENTRY_LIST,
    LIB_ENTRY_ABOUT,
    LIB_ENTRY_UPLOAD,
    LIB_ENTRY_UPDATE,
};

static int s_count = kMaxSystemEntries;

static void copyDefault() {
  memcpy(s_order, kDefaultOrder, sizeof(kDefaultOrder));
  s_count = kMaxSystemEntries;
}

static void normalizeOrder() {
  LibraryEntryType normalized[kMaxSystemEntries];
  int normalizedCount = 0;
  bool sawUpload = false;

  for (int i = 0; i < s_count && normalizedCount < kMaxSystemEntries; i++) {
    LibraryEntryType type = s_order[i];
    if (type == LIB_ENTRY_UPLOAD) {
      sawUpload = true;
    }

    bool duplicate = false;
    for (int j = 0; j < normalizedCount; j++) {
      if (normalized[j] == type) {
        duplicate = true;
        break;
      }
    }
    if (duplicate) continue;
    normalized[normalizedCount++] = type;
  }

  if (!sawUpload && normalizedCount < kMaxSystemEntries) {
    normalized[normalizedCount++] = LIB_ENTRY_UPLOAD;
  }

  memcpy(s_order, normalized, normalizedCount * sizeof(LibraryEntryType));
  s_count = normalizedCount;
}

static void persist() {
  String encoded;
  for (int i = 0; i < s_count; i++) {
    if (i > 0) encoded += ',';
    encoded += (int)s_order[i];
  }
  prefs.putString(kNvsKey, encoded);
}

static void loadEncoded(const String& raw) {
  s_count = 0;

  int start = 0;
  while (start < raw.length() && s_count < kMaxSystemEntries) {
    int comma = raw.indexOf(',', start);
    String token = (comma < 0) ? raw.substring(start) : raw.substring(start, comma);
    token.trim();
    if (token.length() > 0) {
      long value = token.toInt();
      LibraryEntryType type = (LibraryEntryType)value;
      if (isValidLibEntry(type)) {
        bool duplicate = false;
        for (int i = 0; i < s_count; i++) {
          if (s_order[i] == type) {
            duplicate = true;
            break;
          }
        }
        if (!duplicate) {
          s_order[s_count++] = type;
        }
      }
    }
    if (comma < 0) break;
    start = comma + 1;
  }

  normalizeOrder();
}

void loadSettings() {
  if (!prefs.isKey(kNvsKey)) {
    copyDefault();
    return;
  }
  loadEncoded(prefs.getString(kNvsKey, ""));
}

int count() {
  return s_count;
}

LibraryEntryType entryAt(int index) {
  if (index < 0 || index >= s_count) return LIB_ENTRY_BOOKMARKS;
  return s_order[index];
}

int copyEntries(LibraryEntryType* out, int outCap) {
  if (out == nullptr || outCap <= 0) return 0;
  int n = min(s_count, outCap);
  for (int i = 0; i < n; i++) {
    out[i] = s_order[i];
  }
  return n;
}

void setEntries(const LibraryEntryType* entries, int entryCount) {
  if (entries == nullptr || entryCount <= 0) {
    s_order[0] = LIB_ENTRY_UPLOAD;
    s_count = 1;
    persist();
    return;
  }

  s_count = 0;
  for (int i = 0; i < entryCount && s_count < kMaxSystemEntries; i++) {
    LibraryEntryType type = entries[i];
    if (!isValidLibEntry(type)) continue;

    bool duplicate = false;
    for (int j = 0; j < s_count; j++) {
      if (s_order[j] == type) {
        duplicate = true;
        break;
      }
    }
    if (duplicate) continue;
    s_order[s_count++] = type;
  }
  normalizeOrder();
  persist();
}

void resetToDefaults() {
  prefs.remove(kNvsKey);
  copyDefault();
}

}  // namespace LibraryMenuOrder
