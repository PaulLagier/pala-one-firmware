#include "src/hal/companion_rpc.h"
#include "src/hal/companion_language.h"
#include "src/storage/navigation_profile.h"
#include "src/hal/packed_file_stream.h"
#ifdef PALA_COMPANION
#include <ArduinoJson.h>
#include "src/hal/companion_bookmarks.h"
#include "src/hal/companion_ble.h"
#include <esp_random.h>
#include <mbedtls/sha256.h>
#include "src/state.h"
#include "src/pure/paths.h"
#include "src/storage/library.h"
#include "src/storage/book_metadata.h"
#include "src/storage/fs_util.h"
#include "src/ui/font.h"
#include "src/hal/input.h"
#include "src/storage/click_timings.h"
#include "src/ui/screen_settings.h"
#include "src/hal/display.h"
#include "src/ui/statusbar.h"
#include "src/ui/lock.h"
#include "src/ui/bookmark_settings.h"
#include "src/ui/sleep.h"
#include "src/ui/header_title.h"
#include "src/ui/reader_actions.h"

namespace {
File upload;
String temporary, destination;
uint32_t uploadId = 0, expected = 0, received = 0;
mbedtls_sha256_context digest;
bool hashing = false;
uint32_t read32(const uint8_t *b) {
  return uint32_t(b[0]) | uint32_t(b[1]) << 8 | uint32_t(b[2]) << 16 | uint32_t(b[3]) << 24;
}
// Leave 22 bytes for the session field added by the BLE transport.
String response(JsonDocument &doc) {
  String out;
  serializeJson(doc, out);
  if (out.length() > 490) {
    const uint32_t id = doc["id"] | 0U;
    doc.clear();
    doc["id"] = id;
    doc["ok"] = false;
    doc["error"] = "response-too-large";
    out = "";
    serializeJson(doc, out);
  }
  return out;
}
String fail(uint32_t id, const char *reason) {
  JsonDocument d;
  d["id"] = id;
  d["ok"] = false;
  d["error"] = reason;
  return response(d);
}
// Protocol v3 holds use 0:none, 1:bookmark, 2:lock, 3:menu. Public firmware
// action IDs also contain navigation/rotation; never cast between schemas.
int wireAction(ButtonAction action) {
  switch (action) {
  case ACTION_NONE:
    return 0;
  case ACTION_BOOKMARK:
    return 1;
  case ACTION_LOCK:
    return 2;
  case ACTION_MENU:
    return 3;
  default:
    return -1; // Existing public binding outside the legacy app schema.
  }
}
ButtonAction firmwareAction(int action) {
  static const ButtonAction actions[] = {ACTION_NONE, ACTION_BOOKMARK, ACTION_LOCK, ACTION_MENU};
  return actions[action];
}
void addSettings(JsonObject o) {
  o["language"] = CompanionLanguage::currentCode();
  auto languages = o["availableLanguages"].to<JsonArray>();
  for (size_t i = 0; i < CompanionLanguage::count(); ++i)
    languages.add(CompanionLanguage::codeAt(i));
  o["title"] = HeaderTitle::current();
  o["fontSize"] = Font::currentBodySize();
  o["fontFamily"] = Font::currentFamily() == Font::Family::OpenDyslexic ? "dys" : "helv";
  o["lineGap"] = Font::currentLineGap();
  o["bionic"] = Font::bionicEnabled();
  o["sleepSeconds"] = Sleep::idleTimeoutSecs();
  o["noScreensaver"] = Sleep::noScreensaver();
  o["lockOnSleep"] = Sleep::lockOnSleep();
  o["longAction"] = wireAction(Gestures::actionLong());
  o["extraLongAction"] = wireAction(Gestures::actionExtraLong());
  o["clickHoldAction"] = wireAction(Gestures::actionClickHold());
}
bool containsNul(JsonVariantConst value) {
  if (!value.is<const char *>())
    return false;
  const JsonString string = value.as<JsonString>();
  return string.size() && std::memchr(string.c_str(), 0, string.size()) != nullptr;
}
bool setSetting(const String &key, JsonVariant v) {
  if (containsNul(v))
    return false;
  if (key == "language" && v.is<const char *>())
    return CompanionLanguage::setLanguage(v.as<const char *>());
  if (key == "title" && v.is<const char *>()) {
    const JsonString raw = v.as<JsonString>();
    for (size_t i = 0; i < raw.size(); ++i) {
      const uint8_t byte = uint8_t(raw.c_str()[i]);
      if (byte < 32 || byte == 127)
        return false;
    }
    String t = v.as<String>();
    t.trim();
    if (t.length() > 31)
      return false;
    HeaderTitle::set(t.c_str());
    return true;
  }
  if (key == "fontFamily" && v.is<const char *>()) {
    String f = v.as<String>();
    if (f != "helv" && f != "dys")
      return false;
    Font::setFamily(f == "dys" ? Font::Family::OpenDyslexic : Font::Family::Helvetica);
    return true;
  }
  if (key == "readingMenu" && v.is<bool>())
    return NavigationProfile::setReadingFocused(v.as<bool>());
  if (key == "halfParagraphGaps" && v.is<bool>()) {
    Font::setHalfParagraphGaps(v.as<bool>());
    return true;
  }
  if (key == "flipped" && v.is<bool>()) {
    ScreenSettings::setScreenRotation(v.as<bool>());
    return true;
  }
  if (key == "bionic" && v.is<bool>()) {
    Font::setBionic(v.as<bool>());
    return true;
  }
  if (key == "noScreensaver" && v.is<bool>()) {
    Sleep::setNoScreensaver(v.as<bool>());
    return true;
  }
  if (key == "lockOnSleep" && v.is<bool>()) {
    Sleep::setLockOnSleep(v.as<bool>());
    return true;
  }
  if (key == "bookmarkToggle" && v.is<bool>()) {
    return BookmarkSettings::setToggleEnabled(v.as<bool>());
  }
  if (key == "lockIcon" && v.is<bool>()) {
    Lock::setIconEnabled(v.as<bool>());
    return true;
  }
  if (!v.is<int>())
    return false;
  const int n = v.as<int>();
  if (key == "clickSequenceMs" || key == "holdMs" || key == "veryLongMs" || key == "debounceMs" ||
      key == "clickGapMs") {
    if (n < 0)
      return false;
    const int index = key == "clickGapMs"        ? 0
                      : key == "clickSequenceMs" ? 1
                      : key == "holdMs"          ? 2
                      : key == "veryLongMs"      ? 3
                                                 : 4;
    if (index == 0 && uint32_t(n) > ClickTimings::maxClickSequenceMs())
      return false;
    if (index == 2 && uint32_t(n) >= ClickTimings::veryLongMs())
      return false;
    const auto &setting = ClickTimings::timingSettings()[index];
    if (uint32_t(n) < setting.minValue() || uint32_t(n) > setting.maxValue())
      return false;
    setting.set(uint32_t(n));
    return true;
  }
  if (key == "quadAction" && n >= ACTION_NONE && n <= ACTION_ROTATE) {
    Gestures::setActionQuad(ButtonAction(n));
    return true;
  }
  if (key == "statusbar" && n >= 0 && n <= 2) {
    Statusbar::setMode(Statusbar::Mode(n));
    return true;
  }
  if (key == "fontSize" && (n == 8 || n == 10 || n == 12 || n == 14)) {
    Font::setBodySize(n);
    return true;
  }
  if (key == "lineGap" && n >= 0 && n <= 4) {
    Font::setLineGap(n);
    return true;
  }
  if (key == "sleepSeconds" && (n >= 10 && n <= 3600)) {
    Sleep::setIdleTimeout(n);
    return true;
  }
  if (n >= 0 && n <= 3) {
    if (key == "longAction") {
      Gestures::setActionLong(firmwareAction(n));
      return true;
    }
    if (key == "extraLongAction") {
      Gestures::setActionExtraLong(firmwareAction(n));
      return true;
    }
    if (key == "clickHoldAction") {
      Gestures::setActionClickHold(firmwareAction(n));
      return true;
    }
  }
  return false;
}
} // namespace
void companionRpcReset() {
  companionBookmarksReset();
  if (upload)
    upload.close();
  // Reserved transfer files never enter the catalogue. Recover after power loss.
  static bool recovered = false;
  if (!recovered) {
    recovered = true;
    File dir = FS.open("/books");
    if (dir && dir.isDirectory()) {
      File entry = dir.openNextFile();
      while (entry) {
        String path = entry.path();
        const bool stale =
            !entry.isDirectory() && path.startsWith("/books/.pala-ble-") && path.endsWith(".tmp");
        entry.close();
        if (stale)
          FS.remove(path);
        entry = dir.openNextFile();
      }
      dir.close();
    }
  }
  if (temporary.length() && FS.exists(temporary))
    FS.remove(temporary);
  if (hashing) {
    mbedtls_sha256_free(&digest);
    hashing = false;
  }
  temporary = "";
  destination = "";
  uploadId = 0;
  expected = 0;
  received = 0;
}
String companionRpcControl(const uint8_t *bytes, size_t length) {
  if (!bytes || !length || length > 512)
    return fail(0, "invalid-request");
  JsonDocument in;
  if (deserializeJson(in, bytes, length))
    return fail(0, "invalid-json");
  // Never let an embedded NUL turn a distinct JSON path/op into an alias
  // after conversion to Arduino String / C strings.
  for (const char *field : {"op", "path", "name", "key", "sha256"}) {
    const auto value = in[field];
    if ((!value.isNull() && !value.is<const char *>()) || containsNul(value))
      return fail(0, "invalid-request");
  }
  const uint32_t id = in["id"] | 0U;
  const String op = in["op"] | "";
  if (!id || !in["id"].is<uint32_t>() || !in["op"].is<const char *>())
    return fail(0, "invalid-request");
  JsonDocument out;
  out["id"] = id;
  out["ok"] = true;
  if (op == "ping") {
    const size_t total = fsTotalBytesSafe(), used = fsUsedBytesSafe();
    out["totalBytes"] = total;
    out["usedBytes"] = used;
    out["freeBytes"] = used <= total ? total - used : 0;
    out["uploading"] = bool(upload);
    out["bookFormats"].to<JsonArray>().add("pbt1");
  } else if (op == "books.list") {
    if (upload)
      return fail(id, "transfer-busy");
    if (!in["cursor"].isNull() && !in["cursor"].is<int>())
      return fail(id, "invalid-cursor");
    int cursor = in["cursor"] | 0;
    if (cursor < 0 || cursor > g_library.bookCount)
      return fail(id, "invalid-cursor");
    auto books = out["books"].to<JsonArray>();
    // One bounded entry per page fits even the longest UTF-8 path.
    if (cursor < g_library.bookCount) {
      const auto &b = g_library.books[cursor];
      auto item = books.add<JsonObject>();
      // Catalog display names have a fixed byte buffer. Build from the full
      // canonical path so its truncation cannot split a UTF-8 character.
      item["name"] = prettyRelativeLabel(String(b.path).substring(7));
      item["path"] = b.path;
      item["size"] = b.size;
      cursor++;
    }
    if (cursor < g_library.bookCount)
      out["next"] = cursor;
    else
      out["next"] = nullptr;
    out["total"] = g_library.bookCount;
  } else if (op == "books.delete") {
    if (upload)
      return fail(id, "transfer-busy");
    String path = in["path"] | "";
    int index = bookIndexForPath(path);
    if (index < 0)
      return fail(id, "book-not-found");
    if (!FS.remove(path))
      return fail(id, "storage-error");
    deleteBookMetadata(path);
    loadBooks();
    companionBleRefreshIdentity();
  } else if (op.startsWith("bookmarks.")) {
    if (upload)
      return fail(id, "transfer-busy");
    const char *error = companionBookmarksCommand(op, in, out);
    if (error)
      return fail(id, error);
  } else if (op == "settings.advanced.get") {
    auto o = out["settings"].to<JsonObject>();
    o["readingMenu"] = NavigationProfile::readingFocused();
    o["halfParagraphGaps"] = Font::halfParagraphGapsEnabled();
    o["flipped"] = ScreenSettings::isScreenFlipped();
    o["statusbar"] = int(Statusbar::mode());
    o["lockIcon"] = Lock::iconEnabled();
    o["bookmarkToggle"] = BookmarkSettings::toggleEnabled();
    o["clickGapMs"] = ClickTimings::maxClickGapMs();
    o["clickSequenceMs"] = ClickTimings::maxClickSequenceMs();
    o["legacyControls"] = Gestures::legacyControlsOn();
    auto actions = o["rawActions"].to<JsonObject>();
    actions["short"] = int(Gestures::actionShort());
    actions["double"] = int(Gestures::actionDouble());
    actions["triple"] = int(Gestures::actionTriple());
    actions["quad"] = int(Gestures::actionQuad());
    actions["long"] = int(Gestures::actionLong());
    actions["veryLong"] = int(Gestures::actionExtraLong());
    actions["clickHold"] = int(Gestures::actionClickHold());
    o["holdMs"] = ClickTimings::longMs();
    o["veryLongMs"] = ClickTimings::veryLongMs();
    o["debounceMs"] = ClickTimings::debounceMs();
  } else if (op == "settings.get") {
    addSettings(out["settings"].to<JsonObject>());
  } else if (op == "settings.set") {
    if (!setSetting(in["key"] | "", in["value"]))
      return fail(id, "invalid-setting");
    companionBookmarksReset();
    companionBleRefreshIdentity();
    addSettings(out["settings"].to<JsonObject>());
  } else if (op == "upload.begin") {
    if (upload)
      return fail(id, "transfer-busy");
    if (g_library.bookCount >= MAX_BOOKS)
      return fail(id, "library-full");
    String name = in["name"] | "";
    String lower = name;
    lower.toLowerCase();
    if (!in["size"].is<uint32_t>())
      return fail(id, "invalid-request");
    uint32_t size = in["size"].as<uint32_t>();
    const uint32_t maximum =
        lower.endsWith(".pbt") ? PalaPacked::MaximumPhysicalSize : PalaPacked::MaximumSize;
    if (!size || size > maximum || fsFreeBytesSafe() < size + 8192)
      return fail(id, "storage-full");
    if ((!lower.endsWith(".txt") && !lower.endsWith(".pbt")) || name.length() <= 4 ||
        name.startsWith(".") || name.length() >= MAX_UPLOAD_FILE_NAME || name.indexOf('/') >= 0 ||
        name.indexOf('\\') >= 0)
      return fail(id, "invalid-filename");
    name = name.substring(0, name.length() - 4) + (lower.endsWith(".pbt") ? ".pbt" : ".txt");
    String clean = sanitizeUploadedFilename(name.substring(0, name.length() - 4) + ".txt");
    if (lower.endsWith(".pbt"))
      clean = clean.substring(0, clean.length() - 4) + ".pbt";
    if (clean.length() <= 4 || clean.length() >= MAX_UPLOAD_FILE_NAME || clean.startsWith("."))
      return fail(id, "invalid-filename");
    destination = "/books/" + clean;
    if (FS.exists(destination)) {
      destination = "";
      return fail(id, "already-exists");
    }
    uploadId = esp_random();
    if (!uploadId)
      uploadId = 1;
    char temporaryName[40];
    snprintf(temporaryName, sizeof(temporaryName), "/books/.pala-ble-%08lx.tmp",
             static_cast<unsigned long>(uploadId));
    temporary = temporaryName;
    upload = FS.open(temporary, "w");
    if (!upload) {
      companionRpcReset();
      return fail(id, "storage-error");
    }
    expected = size;
    received = 0;
    mbedtls_sha256_init(&digest);
    hashing = true;
    if (mbedtls_sha256_starts(&digest, 0)) {
      companionRpcReset();
      return fail(id, "hash-error");
    }
    out["uploadId"] = uploadId;
    out["offset"] = received;
  } else if (op == "upload.cancel") {
    companionRpcReset();
  } else if (op == "upload.finish") {
    if (!upload || (in["uploadId"] | 0U) != uploadId)
      return fail(id, "invalid-upload");
    if (received != expected) {
      companionRpcReset();
      return fail(id, "incomplete-upload");
    }
    uint8_t hash[32];
    if (mbedtls_sha256_finish(&digest, hash)) {
      companionRpcReset();
      return fail(id, "hash-error");
    }
    String actual;
    for (uint8_t b : hash) {
      char h[3];
      snprintf(h, sizeof(h), "%02x", b);
      actual += h;
    }
    String wanted = in["sha256"] | "";
    wanted.toLowerCase();
    if (wanted.length() != 64 || actual != wanted) {
      companionRpcReset();
      return fail(id, "checksum-mismatch");
    }
    upload.flush();
    upload.close();
    if (destination.endsWith(".pbt")) {
      File candidate = FS.open(temporary, "r");
      // The transfer's temporary suffix is not a book type; validate explicitly.
      PalaPacked::Stream<File> book(candidate, inflateBookBlock);
      const size_t size = book.size();
      size_t actualBytes = 0;
      while (book.read() >= 0) {
        ++actualBytes;
        if ((actualBytes & 4095) == 0)
          yield();
      }
      candidate.close();
      if (!size || actualBytes != size) {
        companionRpcReset();
        return fail(id, "invalid-book");
      }
    }
    if (FS.exists(destination)) {
      companionRpcReset();
      return fail(id, "already-exists");
    }
    if (!FS.rename(temporary, destination)) {
      companionRpcReset();
      return fail(id, "storage-error");
    }
    out["path"] = destination;
    out["size"] = received;
    out["sha256"] = actual;
    temporary = "";
    companionRpcReset();
    loadBooks();
    companionBleRefreshIdentity();
  } else
    return fail(id, "unknown-command");
  return response(out);
}
String companionRpcChunk(const uint8_t *b, size_t length) {
  if (!b || length < 13 || length > 512)
    return fail(0, "invalid-chunk");
  uint32_t id = read32(b), transfer = read32(b + 4), offset = read32(b + 8);
  size_t count = length - 12;
  if (!id)
    return fail(0, "invalid-request");
  if (!upload || transfer != uploadId)
    return fail(id, "invalid-upload");
  if (offset != received || count > expected - received)
    return fail(id, "invalid-offset");
  if (upload.write(b + 12, count) != count || mbedtls_sha256_update(&digest, b + 12, count)) {
    companionRpcReset();
    return fail(id, "storage-error");
  }
  received += count;
  JsonDocument out;
  out["id"] = id;
  out["ok"] = true;
  out["offset"] = received;
  return response(out);
}
#else
void companionRpcReset() {}
String companionRpcControl(const uint8_t *, size_t) { return ""; }
String companionRpcChunk(const uint8_t *, size_t) { return ""; }
#endif
