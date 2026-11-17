#include "test_framework.h"
#include "map_kv_store.h"
#include "storage/book_metadata.h"

TEST_CASE("loadBookmarks returns empty when key is absent") {
  MapKvStore kv;
  Bookmarks bm = loadBookmarks(kv, "b_deadbeef");
  CHECK_EQ(bm.count, 0);
}

TEST_CASE("saveBookmarks + loadBookmarks round-trip") {
  MapKvStore kv;
  Bookmarks bm;
  bm.count = 2;
  bm.pages[0] = 5;   bm.offsets[0] = 500;
  bm.pages[1] = 42;  bm.offsets[1] = 4200;

  saveBookmarks(kv, "b_test", bm);

  Bookmarks back = loadBookmarks(kv, "b_test");
  CHECK_EQ(back.count, 2);
  CHECK_EQ(back.pages[0], 5);
  CHECK_EQ(back.offsets[0], 500u);
  CHECK_EQ(back.pages[1], 42);
  CHECK_EQ(back.offsets[1], 4200u);
}

TEST_CASE("loadSavedPage defaults to 0") {
  MapKvStore kv;
  CHECK_EQ(loadSavedPage(kv, "b_new"), 0);
}

TEST_CASE("saveSavedPage + loadSavedPage round-trip") {
  MapKvStore kv;
  saveSavedPage(kv, "b_test", 137);
  CHECK_EQ(loadSavedPage(kv, "b_test"), 137);
}

TEST_CASE("loadSavedPage clamps negative stored values to 0") {
  MapKvStore kv;
  saveSavedPage(kv, "b_test", -99);
  CHECK_EQ(loadSavedPage(kv, "b_test"), 0);
}

TEST_CASE("clearBookMetadata removes progress and bookmarks") {
  MapKvStore kv;
  saveSavedPage(kv, "b_test", 42);
  Bookmarks bm;
  bm.count = 1;
  bm.pages[0] = 5;
  bm.offsets[0] = 100;
  saveBookmarks(kv, "b_test", bm);

  CHECK(kv.has("b_test_p"));
  CHECK(kv.has("b_test_bm"));

  clearBookMetadata(kv, "b_test");
  CHECK(!kv.has("b_test_p"));
  CHECK(!kv.has("b_test_bm"));
}

TEST_CASE("renameBookMetadata moves progress and bookmarks") {
  MapKvStore kv;
  saveSavedPage(kv, "b_old", 50);
  Bookmarks bm;
  bm.count = 1;
  bm.pages[0] = 7;
  bm.offsets[0] = 700;
  saveBookmarks(kv, "b_old", bm);

  renameBookMetadata(kv, "b_old", "b_new");

  // Old keys gone
  CHECK(!kv.has("b_old_p"));
  CHECK(!kv.has("b_old_bm"));
  // New keys hold the data
  CHECK_EQ(loadSavedPage(kv, "b_new"), 50);
  Bookmarks moved = loadBookmarks(kv, "b_new");
  CHECK_EQ(moved.count, 1);
  CHECK_EQ(moved.pages[0], 7);
}

TEST_CASE("renameBookMetadata is a no-op when neither key is set") {
  MapKvStore kv;
  size_t before = kv.entryCount();
  renameBookMetadata(kv, "missing_old", "missing_new");
  CHECK_EQ(kv.entryCount(), before);
}

TEST_CASE("saveBookmarks overwrites prior content cleanly") {
  MapKvStore kv;
  Bookmarks first;
  first.count = 3;
  for (int i = 0; i < 3; i++) {
    first.pages[i] = (uint16_t)(i + 1);
    first.offsets[i] = (uint32_t)((i + 1) * 10);
  }
  saveBookmarks(kv, "b_x", first);

  Bookmarks second;
  second.count = 1;
  second.pages[0] = 99;
  second.offsets[0] = 9900;
  saveBookmarks(kv, "b_x", second);

  Bookmarks back = loadBookmarks(kv, "b_x");
  CHECK_EQ(back.count, 1);
  CHECK_EQ(back.pages[0], 99);
}

namespace {
class BookmarkFaultStore : public MapKvStore {
public:
  bool dropWrites = false, failReads = false, failAfterWrite = false;
  unsigned attemptedWrites = 0;
  void putBytes(const char *key, const void *bytes, size_t size) override {
    ++attemptedWrites;
    if (!dropWrites)
      MapKvStore::putBytes(key, bytes, size);
    if (failAfterWrite)
      failReads = true;
  }
  size_t getBytes(const char *key, void *bytes, size_t size) override {
    return failReads ? 0 : MapKvStore::getBytes(key, bytes, size);
  }
};
} // namespace
TEST_CASE("Reader bookmark toggle verifies persistence and keeps all reading progress") {
  MapKvStore kv;
  saveSavedOffset(kv, "b_toggle", 1234);
  saveSavedPage(kv, "b_toggle", 70);
  CHECK_EQ(String(applyReaderBookmarkAction(kv, "b_toggle", 70000, 500, 600, true)),
           String(D_TOAST_BOOKMARK_SAVED));
  auto back = loadBookmarks(kv, "b_toggle");
  CHECK_EQ(back.count, 1);
  CHECK_EQ(back.pages[0], 70000u);
  CHECK_EQ(String(applyReaderBookmarkAction(kv, "b_toggle", 1, 500, 600, true)),
           String(D_TOAST_BOOKMARK_REMOVED));
  CHECK_EQ(loadBookmarks(kv, "b_toggle").count, 0);
  CHECK_EQ(loadSavedOffset(kv, "b_toggle"), 1234u);
  CHECK_EQ(loadSavedPage(kv, "b_toggle"), 70);
  // An acknowledged empty list is a real one-byte record, not an absent read.
  uint8_t encoded[BOOKMARKS_ENCODED_MAX_SIZE];
  CHECK_EQ(kv.getBytes("b_toggle_bm", encoded, sizeof(encoded)), 1u);
  CHECK_EQ(encoded[0], 0);
}
TEST_CASE("Reader bookmark default remains add-only and duplicate does not write") {
  MapKvStore kv;
  applyReaderBookmarkAction(kv, "b_toggle", 1, 100, 100, false);
  const auto writes = kv.putBytesCallCount();
  CHECK_EQ(String(applyReaderBookmarkAction(kv, "b_toggle", 1, 100, 100, false)),
           String(D_TOAST_BOOKMARK_EXISTS));
  CHECK_EQ(loadBookmarks(kv, "b_toggle").count, 1);
  CHECK_EQ(kv.putBytesCallCount(), writes);
}
TEST_CASE("Failed bookmark writes and unreadable records cannot report removal success") {
  BookmarkFaultStore kv;
  applyReaderBookmarkAction(kv, "b_toggle", 1, 100, 200, false);
  kv.dropWrites = true;
  CHECK_EQ(String(applyReaderBookmarkAction(kv, "b_toggle", 1, 100, 200, true)),
           String(D_WEB_ERR_WRITE_FAILED));
  CHECK_EQ(loadBookmarks(kv, "b_toggle").count, 1);
  kv.failReads = true;
  const auto writes = kv.attemptedWrites;
  CHECK_EQ(String(applyReaderBookmarkAction(kv, "b_toggle", 1, 100, 200, true)),
           String(D_WEB_ERR_WRITE_FAILED));
  CHECK_EQ(kv.attemptedWrites, writes);
  kv.failReads = false;
  kv.dropWrites = false;
  kv.failAfterWrite = true;
  CHECK_EQ(String(applyReaderBookmarkAction(kv, "b_toggle", 1, 100, 200, true)),
           String(D_WEB_ERR_WRITE_FAILED));
}
TEST_CASE("Malformed stored bookmark record is preserved rather than overwritten") {
  BookmarkFaultStore kv;
  const uint8_t broken[] = {2, 9};
  kv.putBytes("b_toggle_bm", broken, sizeof(broken));
  const auto writes = kv.attemptedWrites;
  CHECK_EQ(String(applyReaderBookmarkAction(kv, "b_toggle", 1, 100, 200, true)),
           String(D_WEB_ERR_WRITE_FAILED));
  CHECK_EQ(kv.attemptedWrites, writes);
  uint8_t back[2];
  CHECK_EQ(kv.getBytes("b_toggle_bm", back, sizeof(back)), sizeof(broken));
  CHECK_EQ(back[0], 2);
  CHECK_EQ(back[1], 9);
}

TEST_CASE("Oversized bookmark blobs are not silently truncated and rewritten") {
  BookmarkFaultStore kv;
  uint8_t oversized[BOOKMARKS_ENCODED_MAX_SIZE + 1] = {};
  oversized[0] = 0x80 | MAX_BOOKMARKS;
  kv.putBytes("b_toggle_bm", oversized, sizeof(oversized));
  const auto writes = kv.attemptedWrites;
  CHECK_EQ(String(applyReaderBookmarkAction(kv, "b_toggle", 1, 0, 100, true)),
           String(D_WEB_ERR_WRITE_FAILED));
  CHECK_EQ(kv.attemptedWrites, writes);
  CHECK_EQ(kv.bytesSize("b_toggle_bm"), sizeof(oversized));
}
