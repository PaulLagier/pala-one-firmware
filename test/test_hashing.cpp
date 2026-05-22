#include "test_framework.h"
#include "pure/hashing.h"

TEST_CASE("fnv1a32 known values") {
  // Empty string -> FNV-1a 32-bit offset basis
  CHECK_EQ(fnv1a32(""), 0x811c9dc5u);
  // Reference values (verified against canonical FNV-1a)
  CHECK_EQ(fnv1a32("a"), 0xe40c292cu);
  CHECK_EQ(fnv1a32("foobar"), 0xbf9cf968u);
}

TEST_CASE("hashPath32 matches fnv1a32 of the same bytes") {
  String p = "/books/foo.txt";
  CHECK_EQ(hashPath32(p), fnv1a32(p.c_str()));
}

TEST_CASE("prefKeyForBook is deterministic and well-formed") {
  String k = prefKeyForBook("/books/a.txt");
  CHECK_EQ(k.length(), 10u);  // "b_" + 8 hex digits
  CHECK(k.startsWith("b_"));
  // Determinism
  CHECK_EQ(prefKeyForBook("/books/a.txt"), prefKeyForBook("/books/a.txt"));
  // Different paths produce different keys
  CHECK(prefKeyForBook("/books/a.txt") != prefKeyForBook("/books/b.txt"));
}

TEST_CASE("bmKeyFor appends _bm suffix") {
  CHECK_EQ(bmKeyFor("b_12345678"), String("b_12345678_bm"));
}
