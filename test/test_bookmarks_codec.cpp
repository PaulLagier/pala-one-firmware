#include <cstring>
#include <vector>

#include "test_framework.h"
#include "pure/bookmarks_codec.h"

TEST_CASE("bookmarks codec round-trip preserves pages and offsets") {
  Bookmarks bm;
  bm.count = 3;
  bm.pages[0] = 1;   bm.offsets[0] = 100;
  bm.pages[1] = 42;  bm.offsets[1] = 4200;
  bm.pages[2] = 999; bm.offsets[2] = 0xDEADBEEF;

  uint8_t buf[BOOKMARKS_ENCODED_MAX_SIZE];
  size_t n = encodeBookmarks(bm, buf);
  CHECK_EQ(n, 1u + 3u * 6u);

  Bookmarks back;
  uint8_t got = decodeBookmarks(buf, n, back);
  CHECK_EQ(got, 3);
  CHECK_EQ(back.pages[0], 1u);
  CHECK_EQ(back.offsets[0], 100u);
  CHECK_EQ(back.pages[2], 999u);
  CHECK_EQ(back.offsets[2], 0xDEADBEEFu);
}

TEST_CASE("bookmarks codec handles empty input") {
  Bookmarks bm;
  CHECK_EQ(decodeBookmarks(nullptr, 0, bm), 0);
  CHECK_EQ(bm.count, 0);
}

TEST_CASE("bookmarks codec decodes legacy v1 (page-only) entries") {
  // v1 format: 1 byte count, then 2 bytes per page (no offsets).
  uint8_t buf[1 + 3 * 2] = {3, 5, 0, 10, 0, 200, 0};
  Bookmarks bm;
  CHECK_EQ(decodeBookmarks(buf, sizeof(buf), bm), 3);
  CHECK_EQ(bm.pages[0], 5u);
  CHECK_EQ(bm.pages[1], 10u);
  CHECK_EQ(bm.pages[2], 200u);
  // Legacy entries get the "recompute" placeholder.
  CHECK_EQ(bm.offsets[0], kOffsetUnset);
  CHECK_EQ(bm.offsets[1], kOffsetUnset);
}

TEST_CASE("bookmarks codec clamps overflow count to MAX_BOOKMARKS") {
  uint8_t buf[BOOKMARKS_ENCODED_MAX_SIZE] = {0};
  buf[0] = 200;  // way over MAX_BOOKMARKS
  Bookmarks bm;
  decodeBookmarks(buf, sizeof(buf), bm);
  CHECK(bm.count <= MAX_BOOKMARKS);
}

TEST_CASE("addBookmark adds a new bookmark and keeps sorted by page") {
  Bookmarks bm;
  CHECK(addBookmark(bm, 10, 1000).added);
  CHECK(addBookmark(bm, 3,  300).added);
  CHECK(addBookmark(bm, 7,  700).added);
  CHECK_EQ(bm.count, 3);
  CHECK_EQ(bm.pages[0], 3u);
  CHECK_EQ(bm.pages[1], 7u);
  CHECK_EQ(bm.pages[2], 10u);
  CHECK_EQ(bm.offsets[1], 700u);
}

TEST_CASE("addBookmark refuses duplicate page") {
  Bookmarks bm;
  addBookmark(bm, 5, 500);
  BookmarkAddResult r = addBookmark(bm, 5, 555);
  CHECK(!r.added);
  // Compare against the language macro, not the English literal, so the
  // test still passes if someone builds the host tests with -DLANG_ES_LA.
  // Host default (no LANG_* flag) falls back to LANG_EN via src/lang/lang.h.
  CHECK_EQ(String(r.message), String(D_TOAST_BOOKMARK_EXISTS));
#ifdef LANG_EN
  // Under the default English build, also pin the literal so a key swap
  // (e.g. {added:false} returning the wrong toast) gets caught.
  CHECK_EQ(String(r.message), String("Bookmark exists"));
#endif
  CHECK_EQ(bm.count, 1);
}

TEST_CASE("addBookmark evicts oldest when full") {
  Bookmarks bm;
  // Add MAX_BOOKMARKS+1 in non-decreasing order
  for (int i = 0; i < MAX_BOOKMARKS; i++) {
    addBookmark(bm, (uint16_t)(i + 1), (uint32_t)((i + 1) * 100));
  }
  CHECK_EQ(bm.count, MAX_BOOKMARKS);
  // Adding one more should evict the oldest (entry 0, which was page 1)
  addBookmark(bm, 999, 99900);
  CHECK_EQ(bm.count, MAX_BOOKMARKS);
  // Page 1 should be gone; page 999 should be present at the end after sort
  bool foundOld = false;
  bool foundNew = false;
  for (int i = 0; i < bm.count; i++) {
    if (bm.pages[i] == 1) foundOld = true;
    if (bm.pages[i] == 999) foundNew = true;
  }
  CHECK(!foundOld);
  CHECK(foundNew);
}

TEST_CASE("large bookmark page numbers never wrap or collide") {
  Bookmarks bm;
  CHECK(addBookmark(bm,0,0).added);CHECK(addBookmark(bm,65536,900000).added);
  CHECK(addBookmark(bm,1000000,2000000).added);CHECK(addBookmark(bm,65535,899990).added);
  uint8_t bytes[BOOKMARKS_ENCODED_MAX_SIZE];const auto size=encodeBookmarks(bm,bytes);
  CHECK_EQ(size,1U+4U*8U);CHECK((bytes[0]&0x80)!=0);
  Bookmarks restored;CHECK_EQ(decodeBookmarks(bytes,size,restored),4);
  for(int i=0;i<4;++i){CHECK_EQ(restored.pages[i],bm.pages[i]);CHECK_EQ(restored.offsets[i],bm.offsets[i]);}
  CHECK_EQ(restored.pages[2],65536U);CHECK_EQ(restored.pages[3],1000000U);
  CHECK(!addBookmark(restored,65536,900000).added);
}
TEST_CASE("ordinary bookmarks retain the exact v2 encoding") {
  Bookmarks bm;addBookmark(bm,65535,0x12345678);
  uint8_t bytes[BOOKMARKS_ENCODED_MAX_SIZE];CHECK_EQ(encodeBookmarks(bm,bytes),7U);
  const uint8_t expected[]={1,255,255,0x78,0x56,0x34,0x12};
  CHECK(std::memcmp(bytes,expected,sizeof(expected))==0);
}
TEST_CASE("bookmark decoder rejects partial wide records and bounded legacy input") {
  const uint8_t wide[]={0x81,0,0,1,0,0x78,0x56,0x34,0x12};
  for(size_t size=0;size<sizeof(wide);++size){Bookmarks bm;CHECK_EQ(decodeBookmarks(wide,size,bm),0);}
  const uint8_t legacy[]={3,0,0};Bookmarks bm;CHECK_EQ(decodeBookmarks(legacy,sizeof(legacy),bm),0);
  for(size_t size=1;size<=BOOKMARKS_ENCODED_MAX_SIZE;++size){
    std::vector<uint8_t> bytes(size,0xa5);
    for(unsigned count:{1U,12U,13U,127U,128U,129U,140U,200U,255U}){
      bytes[0]=uint8_t(count);decodeBookmarks(bytes.data(),bytes.size(),bm);CHECK(bm.count<=MAX_BOOKMARKS);
    }
  }
}
