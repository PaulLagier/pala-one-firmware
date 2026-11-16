#include "src/hal/companion_bookmarks.h"
#ifdef PALA_COMPANION
#include "src/state.h"
#include "src/storage/book_metadata.h"
#include "src/storage/library.h"
#include "src/ui/text.h"

namespace {
String cachedKey, cachedText;
uint32_t cachedOffset = 0;
struct BookmarksForPath {
  String key;
  uint16_t pages[MAX_BOOKMARKS]{};
  uint32_t offsets[MAX_BOOKMARKS]{};
  uint8_t count = 0;
  explicit BookmarksForPath(const String &path) : key(prefKeyForBook(path)) {
    count = loadBookmarksForKey(key, pages, offsets);
  }
};
int findMark(const BookmarksForPath &marks, JsonDocument &in) {
  if (!in["page"].is<uint16_t>())
    return -1;
  const auto page = in["page"].as<uint32_t>();
  for (int i = 0; i < marks.count; i++) {
    if (marks.pages[i] != page)
      continue;
    if (marks.offsets[i] == kOffsetUnset
            ? in["offset"].isNull()
            : in["offset"].is<uint32_t>() && in["offset"].as<uint32_t>() == marks.offsets[i])
      return i;
  }
  return -1;
}
} // namespace
void companionBookmarksReset() {
  cachedKey = "";
  cachedText = "";
  cachedOffset = 0;
}
const char *companionBookmarksCommand(const String &op, JsonDocument &in, JsonDocument &out) {
  if (op == "bookmarks.list") {
    if (!in["cursor"].isNull() && !in["cursor"].is<int>())
      return "invalid-cursor";
    const int cursor = in["cursor"] | 0;
    if (cursor < 0 || cursor > MAX_BOOKS * MAX_BOOKMARKS)
      return "invalid-cursor";
    auto list = out["bookmarks"].to<JsonArray>();
    for (int book = cursor / MAX_BOOKMARKS; book < g_library.bookCount; book++) {
      const String path = g_library.books[book].path;
      BookmarksForPath marks(path);
      const int first = book == cursor / MAX_BOOKMARKS ? cursor % MAX_BOOKMARKS : 0;
      if (first >= marks.count)
        continue;
      auto item = list.add<JsonObject>();
      item["path"] = path;
      item["page"] = marks.pages[first];
      if (marks.offsets[first] == kOffsetUnset)
        item["offset"] = nullptr;
      else
        item["offset"] = marks.offsets[first];
      const int next = book * MAX_BOOKMARKS + first + 1;
      if (next < g_library.bookCount * MAX_BOOKMARKS)
        out["next"] = next;
      else
        out["next"] = nullptr;
      return nullptr;
    }
    out["next"] = nullptr;
    return nullptr;
  }
  const String path = in["path"] | "";
  if (bookIndexForPath(path) < 0)
    return "book-not-found";
  BookmarksForPath marks(path);
  const int index = findMark(marks, in);
  if (index < 0)
    return "bookmark-changed";
  if (op == "bookmarks.delete") {
    for (int i = index + 1; i < marks.count; i++) {
      marks.pages[i - 1] = marks.pages[i];
      marks.offsets[i - 1] = marks.offsets[i];
    }
    marks.count--;
    saveBookmarksForKey(marks.key, marks.pages, marks.offsets, marks.count);
    BookmarksForPath verified(path);
    if (verified.count != marks.count)
      return "storage-error";
    for (int i = 0; i < marks.count; i++)
      if (verified.pages[i] != marks.pages[i] || verified.offsets[i] != marks.offsets[i])
        return "storage-error";
    companionBookmarksReset();
    return nullptr;
  }
  if (op != "bookmarks.text")
    return "unknown-command";
  const String key = path + ":" + String(marks.pages[index]) + ":" + String(marks.offsets[index]);
  if (cachedKey != key) {
    companionBookmarksReset();
    cachedOffset = resolveBookmarkOffset(path, marks.pages[index], marks.offsets[index]);
    File file = FS.open(path, "r");
    if (!file)
      return "storage-error";
    (void)extractPageText(file, cachedOffset, cachedText);
    file.close();
    if (cachedText.length() > 8192) {
      companionBookmarksReset();
      return "bookmark-text-too-large";
    }
    for (unsigned i = 0; i < cachedText.length(); i++)
      if (uint8_t(cachedText[i]) < 32 && cachedText[i] != '\n' && cachedText[i] != '\t')
        cachedText.setCharAt(i, ' ');
    cachedKey = key;
  }
  if (!in["cursor"].isNull() && !in["cursor"].is<int>())
    return "invalid-cursor";
  const int cursor = in["cursor"] | 0;
  const int length = int(cachedText.length());
  if (cursor < 0 || cursor > length ||
      (cursor < length && (uint8_t(cachedText[cursor]) & 0xc0) == 0x80))
    return "invalid-cursor";
  int end = min(cursor + 160, length);
  while (end < length && (uint8_t(cachedText[end]) & 0xc0) == 0x80)
    end--;
  out["text"] = cachedText.substring(cursor, end);
  out["resolvedOffset"] = cachedOffset;
  if (end < length)
    out["next"] = end;
  else
    out["next"] = nullptr;
  return nullptr;
}
#endif
