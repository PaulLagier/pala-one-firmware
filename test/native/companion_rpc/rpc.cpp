#include "src/storage/navigation_profile.h"
#include <Arduino.h>
#include "test_framework.h"
#include "map_kv_store.h"
#include <ArduinoJson.h>
#include "src/state.h"
#include "src/storage/library.h"
#include "src/storage/book_metadata.h"
#include "src/storage/click_timings.h"
#include "src/ui/reader_actions.h"
#include "src/ui/font.h"
#include "src/ui/header_title.h"
#include "src/hal/companion_rpc.h"
#include "src/lang/locale_core.h"
#include "src/hal/companion_bookmarks.h"
#include "src/hal/packed_file_stream.h"
#include <openssl/evp.h>

MemoryFS FS;
Catalog g_library;
MapKvStore kv;
unsigned identityRefreshes = 0, metadataDeletes = 0;
bool failBookmarkSave = false;
namespace Gestures {
ButtonAction holds[3] = {ACTION_MENU, ACTION_LOCK, ACTION_BOOKMARK};
ButtonAction actionShort() { return ACTION_NEXT; }
ButtonAction actionDouble() { return ACTION_PREV; }
ButtonAction actionTriple() { return ACTION_HOME; }
ButtonAction actionLong() { return holds[0]; }
ButtonAction actionExtraLong() { return holds[1]; }
ButtonAction actionClickHold() { return holds[2]; }
void setActionLong(ButtonAction value) { holds[0] = value; }
void setActionExtraLong(ButtonAction value) { holds[1] = value; }
void setActionClickHold(ButtonAction value) { holds[2] = value; }
bool legacyControlsOn() { return false; }
} // namespace Gestures
void companionBleRefreshIdentity() { ++identityRefreshes; }
void loadBooks() {
  g_library.bookCount = 0;
  for (const auto &entry : FS.files) {
    String path = entry.first;
    if (!path.endsWith(".txt") && !path.endsWith(".pbt"))
      continue;
    if (g_library.bookCount >= MAX_BOOKS)
      break;
    auto &book = g_library.books[g_library.bookCount++];
    std::snprintf(book.path, sizeof(book.path), "%s", entry.first.c_str());
    std::snprintf(book.name, sizeof(book.name), "%s", entry.first.substr(7).c_str());
    book.size = entry.second->size();
  }
}
int bookIndexForPath(const String &path) {
  for (int i = 0; i < g_library.bookCount; ++i)
    if (path == g_library.books[i].path)
      return i;
  return -1;
}
uint8_t loadBookmarksForKey(const String &key, uint16_t pages[MAX_BOOKMARKS],
                            uint32_t offsets[MAX_BOOKMARKS]) {
  const auto marks = loadBookmarks(kv, key);
  std::copy_n(marks.pages, marks.count, pages);
  std::copy_n(marks.offsets, marks.count, offsets);
  return marks.count;
}
void saveBookmarksForKey(const String &key, const uint16_t pages[MAX_BOOKMARKS],
                         const uint32_t offsets[MAX_BOOKMARKS], uint8_t count) {
  if (failBookmarkSave)
    return;
  Bookmarks marks{};
  marks.count = count;
  std::copy_n(pages, count, marks.pages);
  std::copy_n(offsets, count, marks.offsets);
  saveBookmarks(kv, key, marks);
}
void deleteBookMetadata(const String &path) {
  ++metadataDeletes;
  clearBookMetadata(kv, prefKeyForBook(path));
}
uint32_t resolveBookmarkOffset(const String &, uint16_t page, uint32_t stored) {
  return stored == kOffsetUnset ? uint32_t(page) * 10 : stored;
}
uint32_t extractPageText(File &file, uint32_t offset, String &out) {
  BookReadStream stream(file);
  if (!stream.seek(offset))
    return offset;
  for (int byte; (byte = stream.read()) >= 0 && out.length() <= 8192;)
    out += char(byte);
  return stream.position();
}
namespace {
void reset() {
  companionRpcReset();
  REQUIRE(Language::setLanguage("en"));
  FS = MemoryFS{};
  kv.clear();
  loadBooks();
  identityRefreshes = metadataDeletes = 0;
  failBookmarkSave = false;
  HeaderTitle::title = "Pala";
  Font::size = 10;
  Font::gap = 0;
  Gestures::holds[0] = ACTION_MENU;
  Gestures::holds[1] = ACTION_LOCK;
  Gestures::holds[2] = ACTION_BOOKMARK;
  ClickTimings::resetToDefaults();
}
JsonDocument call(const std::string &json) {
  String text = companionRpcControl(reinterpret_cast<const uint8_t *>(json.data()), json.size());
  CHECK(text.length() <= 490);
  JsonDocument result;
  CHECK(!deserializeJson(result, text.c_str()));
  return result;
}
JsonDocument chunk(uint32_t uploadId, uint32_t offset, const std::vector<uint8_t> &payload,
                   uint32_t id = 100) {
  std::vector<uint8_t> bytes(12);
  uint32_t fields[3] = {id, uploadId, offset};
  for (unsigned f = 0; f < 3; ++f)
    for (unsigned i = 0; i < 4; ++i)
      bytes[f * 4 + i] = uint8_t(fields[f] >> (i * 8));
  bytes.insert(bytes.end(), payload.begin(), payload.end());
  auto text = companionRpcChunk(bytes.data(), bytes.size());
  CHECK(text.length() <= 490);
  JsonDocument result;
  CHECK(!deserializeJson(result, text.c_str()));
  return result;
}
std::string hash(const std::vector<uint8_t> &bytes) {
  uint8_t digest[32];
  unsigned size = 0;
  if (!EVP_Digest(bytes.data(), bytes.size(), digest, &size, EVP_sha256(), nullptr) || size != 32)
    throw std::runtime_error("SHA256 fixture failed");
  std::string result;
  for (auto byte : digest) {
    char word[3];
    std::snprintf(word, sizeof(word), "%02x", byte);
    result += word;
  }
  return result;
}
std::vector<uint8_t> fixture(const char *name = "book.pbt") {
  std::ifstream input(std::string(PACKED_FIXTURES) + "/" + name, std::ios::binary);
  return {std::istreambuf_iterator<char>(input), {}};
}
JsonDocument begin(const char *name, size_t size) {
  return call("{\"id\":1,\"op\":\"upload.begin\",\"name\":\"" + std::string(name) +
              "\",\"size\":" + std::to_string(size) + "}");
}
JsonDocument finish(uint32_t id, const std::string &digest) {
  return call("{\"id\":2,\"op\":\"upload.finish\",\"uploadId\":" + std::to_string(id) +
              ",\"sha256\":\"" + digest + "\"}");
}
void seed(const char *path, const std::string &bytes) {
  auto file = FS.open(path, "w");
  file.print(bytes.c_str());
  loadBooks();
}
void mark(const char *path, uint16_t page = 2, uint32_t offset = 0) {
  Bookmarks marks{};
  marks.count = 1;
  marks.pages[0] = page;
  marks.offsets[0] = offset;
  saveBookmarks(kv, prefKeyForBook(path), marks);
}
} // namespace
TEST_CASE("Companion startup removes only reserved stale transfer files") {
  seed("/books/.pala-ble-dead.tmp", "stale");
  seed("/books/other.tmp", "keep");
  seed("/books/book.txt", "keep");
  companionRpcReset();
  CHECK(!FS.exists("/books/.pala-ble-dead.tmp"));
  CHECK(FS.exists("/books/other.tmp"));
  CHECK(FS.exists("/books/book.txt"));
}
TEST_CASE("Companion rejects invalid requests and bounds every response") {
  reset();
  for (const char *input : {"", "[]", "{", "{}", "{\"id\":0,\"op\":\"ping\"}",
                            "{\"id\":-1,\"op\":\"ping\"}", "{\"id\":1.5,\"op\":\"ping\"}"}) {
    auto result = call(input);
    CHECK(!result["ok"].as<bool>());
  }
  auto result = call("{\"id\":9,\"op\":\"ping\"}");
  CHECK(result["ok"].as<bool>());
  CHECK_EQ(result["id"].as<int>(), 9);
  CHECK(!call(std::string(513, 'x'))["ok"].as<bool>());
  result = call("{\"id\":1,\"op\":\"missing\"}");
  CHECK(result["error"] == "unknown-command");
  auto raw = companionRpcControl(nullptr, 1);
  CHECK(raw.length() <= 512);
}
TEST_CASE("Companion keeps the wire hold-action schema separate from public bindings") {
  reset();
  auto result = call("{\"id\":1,\"op\":\"settings.get\"}");
  CHECK_EQ(result["settings"]["longAction"].as<int>(), 3);
  CHECK_EQ(result["settings"]["extraLongAction"].as<int>(), 2);
  CHECK_EQ(result["settings"]["clickHoldAction"].as<int>(), 1);
  result = call("{\"id\":1,\"op\":\"settings.set\",\"key\":\"longAction\",\"value\":1}");
  REQUIRE(result["ok"].as<bool>());
  CHECK_EQ(Gestures::actionLong(), ACTION_BOOKMARK);
  Gestures::holds[0] = ACTION_ROTATE;
  result = call("{\"id\":1,\"op\":\"settings.get\"}");
  CHECK_EQ(result["settings"]["longAction"].as<int>(), -1);
  CHECK_EQ(Gestures::actionLong(), ACTION_ROTATE);
  result = call("{\"id\":1,\"op\":\"settings.advanced.get\"}");
  REQUIRE(result["ok"].as<bool>());
  CHECK_EQ(result["settings"]["rawActions"]["long"].as<int>(), ACTION_ROTATE);
}
TEST_CASE("Companion settings reject invalid types and cross-field timing changes") {
  reset();
  for (const char *value : {"-1", "99", "true", "null", "\"12\""}) {
    auto result =
        call(std::string("{\"id\":1,\"op\":\"settings.set\",\"key\":\"fontSize\",\"value\":") +
             value + "}");
    CHECK(!result["ok"].as<bool>());
    CHECK_EQ(Font::currentBodySize(), 10);
  }
  auto result = call("{\"id\":1,\"op\":\"settings.set\",\"key\":\"clickGapMs\",\"value\":1000}");
  CHECK(!result["ok"].as<bool>());
  CHECK_EQ(ClickTimings::maxClickGapMs(), 175u);
  result = call("{\"id\":1,\"op\":\"settings.set\",\"key\":\"holdMs\",\"value\":2500}");
  CHECK(!result["ok"].as<bool>());
  CHECK_EQ(ClickTimings::longMs(), 850u);
  result = call("{\"id\":1,\"op\":\"settings.set\",\"key\":\"clickGapMs\",\"value\":100}");
  CHECK(result["ok"].as<bool>());
  CHECK_EQ(ClickTimings::maxClickGapMs(), 100u);
  result = call("{\"id\":1,\"op\":\"settings.set\",\"key\":\"fontSize\",\"value\":12}");
  CHECK(result["ok"].as<bool>());
  CHECK_EQ(Font::currentBodySize(), 12);
  CHECK(identityRefreshes > 0);
}
TEST_CASE("Companion transfers exact binary bytes with ordered chunks and SHA256") {
  for (const char *format : {"book.pbt", "raw.pbt"}) {
    reset();
    auto bytes = fixture(format);
    REQUIRE(!bytes.empty());
    auto result = begin("book.pbt", bytes.size());
    REQUIRE(result["ok"].as<bool>());
    const auto transfer = result["uploadId"].as<uint32_t>();
    for (size_t offset = 0; offset < bytes.size(); offset += 200) {
      const size_t end = (std::min)(offset + 200, bytes.size());
      result = chunk(transfer, uint32_t(offset), {bytes.begin() + offset, bytes.begin() + end});
      REQUIRE(result["ok"].as<bool>());
      CHECK_EQ(result["offset"].as<size_t>(), end);
    }
    result = finish(transfer, hash(bytes));
    REQUIRE(result["ok"].as<bool>());
    REQUIRE(FS.exists("/books/book.pbt"));
    CHECK(*FS.files.at("/books/book.pbt") == bytes);
    CHECK_EQ(g_library.bookCount, 1);
    CHECK_EQ(FS.files.size(), 1u);
  }
}
TEST_CASE("Companion failed validation short writes and cancellation retain existing books") {
  for (int failure = 0; failure < 5; ++failure) {
    reset();
    seed("/books/existing.txt", "keep");
    auto bytes = fixture();
    if (failure == 1)
      bytes.back() ^= 0x40;
    auto result = begin("book.pbt", bytes.size());
    REQUIRE(result["ok"].as<bool>());
    const auto transfer = result["uploadId"].as<uint32_t>();
    if (failure == 2)
      FS.shortWrite = true;
    if (failure == 3)
      FS.failRename = true;
    for (size_t offset = 0; offset < bytes.size(); offset += 200) {
      const size_t end = (std::min)(offset + 200, bytes.size());
      result = chunk(transfer, uint32_t(offset), {bytes.begin() + offset, bytes.begin() + end});
      if (!result["ok"].as<bool>())
        break;
    }
    if (failure == 4)
      result = call("{\"id\":1,\"op\":\"upload.cancel\"}");
    else
      result = finish(transfer, failure == 0 ? std::string(64, '0') : hash(bytes));
    if (failure != 4)
      CHECK(!result["ok"].as<bool>());
    CHECK(FS.exists("/books/existing.txt"));
    CHECK(!FS.exists("/books/book.pbt"));
    CHECK_EQ(FS.files.size(), 1u);
  }
}
TEST_CASE("Companion refuses overwrite bad paths offsets and transfer races") {
  reset();
  seed("/books/existing.txt", "keep");
  CHECK(!begin("existing.txt", 4)["ok"].as<bool>());
  for (const char *name : {"../x.txt", "folder/x.txt", "x.bin", ".txt"})
    CHECK(!begin(name, 4)["ok"].as<bool>());
  CHECK(!begin("large.txt", 2097153)["ok"].as<bool>());
  auto result = begin("allowed.pbt", PalaPacked::MaximumPhysicalSize);
  REQUIRE(result["ok"].as<bool>());
  const auto transfer = result["uploadId"].as<uint32_t>();
  CHECK(!begin("second.txt", 4)["ok"].as<bool>());
  CHECK(!chunk(transfer, 1, {1, 2})["ok"].as<bool>());
  CHECK(!chunk(transfer + 1, 0, {1, 2})["ok"].as<bool>());
  CHECK(!call("{\"id\":1,\"op\":\"books.delete\",\"path\":\"/books/existing.txt\"}")["ok"]
             .as<bool>());
  CHECK(!finish(transfer, std::string(64, '0'))["ok"].as<bool>());
  CHECK(FS.exists("/books/existing.txt"));
  CHECK_EQ(FS.files.size(), 1u);
}
TEST_CASE("Companion bookmark pagination and deletion preserve the uint16 storage format") {
  reset();
  seed("/books/book.txt", "some text");
  mark("/books/book.txt", 65535, 0);
  auto result = call("{\"id\":1,\"op\":\"bookmarks.list\"}");
  REQUIRE(result["ok"].as<bool>());
  CHECK_EQ(result["bookmarks"][0]["page"].as<int>(), 65535);
  result = call("{\"id\":1,\"op\":\"bookmarks.delete\",\"path\":\"/books/"
                "book.txt\",\"page\":65536,\"offset\":0}");
  CHECK(!result["ok"].as<bool>());
  CHECK_EQ(loadBookmarks(kv, prefKeyForBook("/books/book.txt")).count, 1);
  failBookmarkSave = true;
  result = call("{\"id\":1,\"op\":\"bookmarks.delete\",\"path\":\"/books/"
                "book.txt\",\"page\":65535,\"offset\":0}");
  CHECK(result["error"] == "storage-error");
  failBookmarkSave = false;
  result = call("{\"id\":1,\"op\":\"bookmarks.delete\",\"path\":\"/books/"
                "book.txt\",\"page\":65535,\"offset\":0}");
  CHECK(result["ok"].as<bool>());
  CHECK_EQ(loadBookmarks(kv, prefKeyForBook("/books/book.txt")).count, 0);
}
TEST_CASE("Companion bookmark text chunks retain UTF8 boundaries and reject stale bookmarks") {
  reset();
  std::string text(159, 'a');
  for (int i = 0; i < 100; ++i)
    text += u8"é";
  seed("/books/book.txt", text);
  mark("/books/book.txt");
  std::string restored;
  int cursor = 0;
  do {
    auto result = call("{\"id\":1,\"op\":\"bookmarks.text\",\"path\":\"/books/"
                       "book.txt\",\"page\":2,\"offset\":0,\"cursor\":" +
                       std::to_string(cursor) + "}");
    REQUIRE(result["ok"].as<bool>());
    restored += result["text"].as<std::string>();
    if (result["next"].isNull())
      break;
    const int next = result["next"].as<int>();
    REQUIRE(next > cursor);
    cursor = next;
  } while (true);
  CHECK(restored == text);
  auto result = call("{\"id\":1,\"op\":\"bookmarks.text\",\"path\":\"/books/"
                     "book.txt\",\"page\":2,\"offset\":0,\"cursor\":160}");
  CHECK(result["error"] == "invalid-cursor");
  result = call(
      "{\"id\":1,\"op\":\"bookmarks.text\",\"path\":\"/books/book.txt\",\"page\":3,\"offset\":0}");
  CHECK(result["error"] == "bookmark-changed");
}

TEST_CASE("Companion catalog cursors and deletion use existing metadata lifecycle") {
  reset();
  seed("/books/a.txt", "first");
  seed("/books/b.txt", "second");
  mark("/books/a.txt");
  saveSavedOffset(kv, prefKeyForBook("/books/a.txt"), 2);
  auto result = call("{\"id\":1,\"op\":\"books.list\"}");
  REQUIRE(result["ok"].as<bool>());
  CHECK_EQ(result["books"].size(), 1u);
  CHECK_EQ(result["next"].as<int>(), 1);
  result = call("{\"id\":1,\"op\":\"books.list\",\"cursor\":1}");
  CHECK(result["next"].isNull());
  CHECK(result["books"][0]["path"] == "/books/b.txt");
  for (const char *cursor : {"-1", "3", "1.5", "\"0\""}) {
    result = call(std::string("{\"id\":1,\"op\":\"books.list\",\"cursor\":") + cursor + "}");
    CHECK(result["error"] == "invalid-cursor");
  }
  result = call("{\"id\":1,\"op\":\"books.delete\",\"path\":\"/books/a.txt\"}");
  CHECK(result["ok"].as<bool>());
  CHECK(!FS.exists("/books/a.txt"));
  CHECK(FS.exists("/books/b.txt"));
  CHECK_EQ(metadataDeletes, 1u);
  CHECK_EQ(loadBookmarks(kv, prefKeyForBook("/books/a.txt")).count, 0);
  CHECK_EQ(loadSavedOffset(kv, prefKeyForBook("/books/a.txt")), kOffsetUnset);
  CHECK_EQ(g_library.bookCount, 1);
}

TEST_CASE("Companion settings retain bounds with maximally escaped titles") {
  reset();
  JsonDocument request;
  request["id"] = 1;
  request["op"] = "settings.set";
  request["key"] = "title";
  request["value"] = std::string(31, '"');
  std::string json;
  serializeJson(request, json);
  auto result = call(json);
  REQUIRE(result["ok"].as<bool>());
  CHECK_EQ(result["settings"]["title"].as<std::string>().size(), 31u);
  request["value"] = std::string(32, 'a');
  json.clear();
  serializeJson(request, json);
  result = call(json);
  CHECK(result["error"] == "invalid-setting");
  CHECK_EQ(std::strlen(HeaderTitle::current()), 31u);
}

TEST_CASE("Companion rejects embedded-NUL aliases before path or setting mutations") {
  reset();
  seed("/books/book.txt", "keep");
  auto result = call("{\"id\":1,\"op\":\"books.delete\",\"path\":\"/books/book.txt\\u0000other\"}");
  CHECK(!result["ok"].as<bool>());
  CHECK(FS.exists("/books/book.txt"));
  CHECK_EQ(metadataDeletes, 0u);
  result = call("{\"id\":1,\"op\":\"ping\\u0000other\"}");
  CHECK(!result["ok"].as<bool>());
  result =
      call("{\"id\":1,\"op\":\"settings.set\",\"key\":\"title\",\"value\":\"Pala\\u0000other\"}");
  CHECK(result["error"] == "invalid-setting");
  CHECK(std::string(HeaderTitle::current()) == "Pala");
  result =
      call("{\"id\":1,\"op\":\"settings.set\",\"key\":\"title\",\"value\":\"bad\\u0001title\"}");
  CHECK(result["error"] == "invalid-setting");
  CHECK(std::string(HeaderTitle::current()) == "Pala");
}

TEST_CASE("Companion binary frames enforce the exact 500-byte payload boundary") {
  reset();
  std::vector<uint8_t> bytes(501, 'a');
  auto result = begin("book.txt", bytes.size());
  REQUIRE(result["ok"].as<bool>());
  const auto transfer = result["uploadId"].as<uint32_t>();
  result = chunk(transfer, 0, bytes);
  CHECK(result["error"] == "invalid-chunk");
  result = chunk(transfer, 0, {bytes.begin(), bytes.begin() + 500});
  REQUIRE(result["ok"].as<bool>());
  CHECK_EQ(result["offset"].as<int>(), 500);
  result = chunk(transfer, 500, {bytes.back()});
  REQUIRE(result["ok"].as<bool>());
  result = finish(transfer, hash(bytes));
  REQUIRE(result["ok"].as<bool>());
  CHECK(*FS.files.at("/books/book.txt") == bytes);
}

TEST_CASE("Companion book names retain UTF8 beyond the catalog display buffer") {
  reset();
  std::string name;
  for (int i = 0; i < 41; ++i)
    name += u8"é";
  seed(("/books/" + name + ".txt").c_str(), "text");
  auto result = call("{\"id\":1,\"op\":\"books.list\"}");
  REQUIRE(result["ok"].as<bool>());
  CHECK(result["books"][0]["name"].as<std::string>() == name);
}

TEST_CASE("Companion exposes every runtime language within the response limit") {
  reset();
  for (char titleChar : {'x', '\"', '\\'}) {
    HeaderTitle::title = std::string(31, titleChar).c_str();
    for (size_t i = 0; i < Language::count(); ++i) {
      const std::string code = Language::codeAt(i);
      auto changed =
          call("{\"id\":4294967295,\"op\":\"settings.set\",\"key\":\"language\",\"value\":\"" +
               code + "\"}");
      REQUIRE(changed["ok"].as<bool>());
      CHECK(changed["settings"]["language"] == code);
      auto result = call("{\"id\":4294967295,\"op\":\"settings.get\"}");
      REQUIRE(result["ok"].as<bool>());
      CHECK_EQ(result["settings"]["availableLanguages"].size(), 11u);
      CHECK(result["settings"]["language"] == code);
      // Advanced settings are a separate bounded object, not the base schema.
      result = call("{\"id\":4294967295,\"op\":\"settings.advanced.get\"}");
      REQUIRE(result["ok"].as<bool>());
      CHECK(result["settings"]["rawActions"].is<JsonObject>());
    }
  }
  const std::string before = Language::currentCode();
  auto invalid = call("{\"id\":1,\"op\":\"settings.set\",\"key\":\"language\",\"value\":\"zz\"}");
  CHECK(!invalid["ok"].as<bool>());
  CHECK_EQ(std::string(Language::currentCode()), before);
}

ProfilePrefs prefs;

TEST_CASE("Companion reading menu selection is typed and reversible") {
  prefs.flags.clear();
  prefs.failWrites = false;
  CHECK(NavigationProfile::setReadingFocused(false));
  auto result = call(R"({"id":1,"op":"settings.set","key":"readingMenu","value":true})");
  REQUIRE(result["ok"].as<bool>());
  NavigationProfile::loadSettings();
  CHECK(NavigationProfile::readingFocused());
  result = call(R"({"id":1,"op":"settings.advanced.get"})");
  REQUIRE(result["ok"].as<bool>());
  CHECK(result["settings"]["readingMenu"].as<bool>());
  result = call(R"({"id":1,"op":"settings.set","key":"readingMenu","value":1})");
  CHECK(!result["ok"].as<bool>());
  CHECK(NavigationProfile::readingFocused());
  prefs.failWrites = true;
  result = call(R"({"id":1,"op":"settings.set","key":"readingMenu","value":false})");
  CHECK(!result["ok"].as<bool>());
  CHECK(NavigationProfile::readingFocused());
  prefs.failWrites = false;
  result = call(R"({"id":1,"op":"settings.set","key":"readingMenu","value":false})");
  REQUIRE(result["ok"].as<bool>());
  CHECK(!NavigationProfile::readingFocused());
}
