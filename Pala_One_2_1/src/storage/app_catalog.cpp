#include "src/storage/app_catalog.h"

#include "pala_app.h"          // PalaAppHeader, PALA_APP_MAGIC
#include "src/pure/paths.h"        // lastPathComponent
#include "src/state.h"             // FS macro

AppCatalog g_apps;

// Display label fallback when the binary has no valid header: take the
// filename stem (strip "/apps/" prefix and ".bin" extension) and turn '_'
// into ' ' for readability. Mirrors how prettyRelativeLabel handles books.
static void labelFromFilename(const String& absPath, char* out) {
  String stem = lastPathComponent(absPath);
  if (stem.endsWith(".bin")) stem = stem.substring(0, stem.length() - 4);
  stem.replace('_', ' ');
  strncpy(out, stem.c_str(), MAX_APP_NAME);
  out[MAX_APP_NAME] = '\0';
}

// Try to read the display name out of the binary's header. Returns true
// on success and writes a NUL-terminated string into `out` (capped at
// MAX_APP_NAME). On any I/O error or bad magic, returns false and leaves
// `out` untouched — caller falls back to the filename stem.
static bool nameFromHeader(const String& absPath, char* out) {
  File hf = FS.open(absPath, "r");
  if (!hf) return false;
  bool ok = false;
  if (hf.size() >= sizeof(PalaAppHeader)) {
    PalaAppHeader hdr;
    if (hf.read(reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr)) == static_cast<int>(sizeof(hdr)) &&
        hdr.magic == PALA_APP_MAGIC) {
      // hdr.name is 32 bytes — may not be NUL-terminated; clamp.
      char tmp[MAX_APP_NAME + 1];
      memcpy(tmp, hdr.name, MAX_APP_NAME);
      tmp[MAX_APP_NAME] = '\0';
      if (tmp[0] != '\0') {
        strncpy(out, tmp, MAX_APP_NAME);
        out[MAX_APP_NAME] = '\0';
        ok = true;
      }
    }
  }
  hf.close();
  return ok;
}

static void sortByName() {
  for (int i = 0; i < g_apps.count - 1; i++) {
    for (int j = i + 1; j < g_apps.count; j++) {
      if (strcmp(g_apps.entries[j].name, g_apps.entries[i].name) < 0) {
        AppEntry tmp = g_apps.entries[i];
        g_apps.entries[i] = g_apps.entries[j];
        g_apps.entries[j] = tmp;
      }
    }
  }
}

void loadApps() {
  g_apps.count = 0;

  if (!FS.exists("/apps")) {
    FS.mkdir("/apps");
    return;  // nothing to scan in a freshly-created dir
  }

  File dir = FS.open("/apps");
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    return;
  }

  File f = dir.openNextFile();
  while (f && g_apps.count < MAX_APPS) {
    String entryName = String(f.name());
    f.close();

    // Only .bin files are apps. Skip .dat (per-app storage), .tmp
    // (in-flight uploads), and anything else.
    if (!entryName.endsWith(".bin")) {
      f = dir.openNextFile();
      continue;
    }

    // f.name() returns a basename on this LittleFS port; normalize to
    // the absolute path the loader expects.
    String absPath = entryName.startsWith("/")
                         ? entryName
                         : (String("/apps/") + entryName);

    if ((int)absPath.length() > MAX_APP_PATH) {
      // Path won't fit in our fixed buffer — skip rather than truncate
      // to something the loader can't reopen.
      f = dir.openNextFile();
      continue;
    }

    AppEntry& e = g_apps.entries[g_apps.count];

    if (!nameFromHeader(absPath, e.name)) {
      labelFromFilename(absPath, e.name);
    }
    strncpy(e.path, absPath.c_str(), MAX_APP_PATH);
    e.path[MAX_APP_PATH] = '\0';

    g_apps.count++;
    f = dir.openNextFile();
  }
  if (f) f.close();
  dir.close();

  sortByName();
}

const char* appPath(int idx) {
  if (idx < 0 || idx >= g_apps.count) return nullptr;
  return g_apps.entries[idx].path;
}
