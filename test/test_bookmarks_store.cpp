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
