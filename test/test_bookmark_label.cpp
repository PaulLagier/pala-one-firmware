#include "test_framework.h"
#include "pure/bookmark_label.h"

TEST_CASE("bookmark label reads first 5 words and appends page number") {
  StringReadStream in("It was the best of times, it was the worst of times");
  String label = readBookmarkLabelAtOffset(in, 0, 41);  // page 41 -> "p. 42"
  CHECK_EQ(label, String("It was the best of - p. 42"));
}

TEST_CASE("bookmark label falls back when stream is empty") {
  StringReadStream in("");
  // Empty stream — seek(0) succeeds, but no words found.
  String label = readBookmarkLabelAtOffset(in, 0, 0);
  CHECK_EQ(label, String("Page - p. 1"));
}

TEST_CASE("bookmark label honors seek offset") {
  StringReadStream in("chapter one\nchapter two");
  String label = readBookmarkLabelAtOffset(in, 12, 0);  // start at "chapter two"
  CHECK_EQ(label, String("chapter two - p. 1"));
}

TEST_CASE("bookmark label fails gracefully when seek out of range") {
  StringReadStream in("short");
  String label = readBookmarkLabelAtOffset(in, 9999, 0);
  CHECK_EQ(label, String("p. 1"));
}

TEST_CASE("bookmark label collapses newlines and tabs into single spaces") {
  StringReadStream in("alpha\n\nbeta\tgamma");
  String label = readBookmarkLabelAtOffset(in, 0, 0);
  CHECK_EQ(label, String("alpha beta gamma - p. 1"));
}
