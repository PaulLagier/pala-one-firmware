#include "test_framework.h"
#include "pure/text_util.h"

TEST_CASE("utf8CharLenFromLead recognizes ASCII and multibyte leads") {
  CHECK_EQ(utf8CharLenFromLead('a'), 1);
  CHECK_EQ(utf8CharLenFromLead(0x7F), 1);
  CHECK_EQ(utf8CharLenFromLead(0xC3), 2);  // é lead
  CHECK_EQ(utf8CharLenFromLead(0xE2), 3);  // — lead
  CHECK_EQ(utf8CharLenFromLead(0xF0), 4);  // emoji lead
  CHECK_EQ(utf8CharLenFromLead(0x80), 1);  // continuation byte alone -> 1
}

TEST_CASE("utf8SafeCharLenAt handles valid and malformed UTF-8") {
  String s = "café";  // c, a, f, 0xC3, 0xA9
  CHECK_EQ(utf8SafeCharLenAt(s, 0), 1);
  CHECK_EQ(utf8SafeCharLenAt(s, 3), 2);
  CHECK_EQ(utf8SafeCharLenAt(s, 999), 0);   // out of range
  CHECK_EQ(utf8SafeCharLenAt(s, -1), 0);

  // Truncated multibyte sequence -> fall back to 1
  char truncated[] = {(char)0xC3, 0};
  String t(truncated);
  CHECK_EQ(utf8SafeCharLenAt(t, 0), 1);
}

TEST_CASE("normalizeTypography strips BOM") {
  char bom[] = {(char)0xEF, (char)0xBB, (char)0xBF, 'H', 'i', 0};
  String s(bom);
  CHECK_EQ(normalizeTypography(s), String("Hi"));
}

TEST_CASE("normalizeTypography converts NBSP to space") {
  char nbsp[] = {'a', (char)0xC2, (char)0xA0, 'b', 0};
  String s(nbsp);
  CHECK_EQ(normalizeTypography(s), String("a b"));
}

TEST_CASE("normalizeTypography converts smart single quotes") {
  // U+2018 = 0xE2 0x80 0x98, U+2019 = 0xE2 0x80 0x99
  char in[] = {(char)0xE2, (char)0x80, (char)0x98, 'x', (char)0xE2, (char)0x80, (char)0x99, 0};
  String s(in);
  CHECK_EQ(normalizeTypography(s), String("'x'"));
}

TEST_CASE("normalizeTypography converts smart double quotes") {
  // U+201C = 0xE2 0x80 0x9C, U+201D = 0xE2 0x80 0x9D
  char in[] = {(char)0xE2, (char)0x80, (char)0x9C, 'h', 'i', (char)0xE2, (char)0x80, (char)0x9D, 0};
  String s(in);
  CHECK_EQ(normalizeTypography(s), String("\"hi\""));
}

TEST_CASE("normalizeTypography converts em/en dashes to '-'") {
  // U+2013 en-dash = 0xE2 0x80 0x93, U+2014 em-dash = 0xE2 0x80 0x94
  char in[] = {'a', (char)0xE2, (char)0x80, (char)0x94, 'b', 0};
  String s(in);
  CHECK_EQ(normalizeTypography(s), String("a-b"));
}

TEST_CASE("normalizeTypography converts ellipsis to ...") {
  // U+2026 ellipsis = 0xE2 0x80 0xA6
  char in[] = {'h', 'i', (char)0xE2, (char)0x80, (char)0xA6, 0};
  String s(in);
  CHECK_EQ(normalizeTypography(s), String("hi..."));
}

TEST_CASE("normalizeTypography preserves non-special UTF-8") {
  // "café" should round-trip unchanged
  String s = "café";
  CHECK_EQ(normalizeTypography(s), s);
}

TEST_CASE("compactText collapses spaces and tabs") {
  CHECK_EQ(compactText("a    b\tc"), String("a b c"));
}

TEST_CASE("compactText strips \\r") {
  CHECK_EQ(compactText("a\r\nb"), String("a\nb"));
}

TEST_CASE("compactText limits consecutive newlines to two") {
  CHECK_EQ(compactText("a\n\n\n\nb"), String("a\n\nb"));
  CHECK_EQ(compactText("a\n\nb"), String("a\n\nb"));
  CHECK_EQ(compactText("a\nb"), String("a\nb"));
}

TEST_CASE("compactText trims trailing whitespace and newlines") {
  CHECK_EQ(compactText("hello   "), String("hello"));
  CHECK_EQ(compactText("hello\n\n\n"), String("hello"));
  CHECK_EQ(compactText("hello \n "), String("hello"));
}

TEST_CASE("compactText drops trailing spaces before newline") {
  CHECK_EQ(compactText("a   \nb"), String("a\nb"));
}

TEST_CASE("compactText streaming: space run across chunk boundary collapses") {
  bool lastWasSpace = false;
  int newlineCount = 0;
  String a = compactText("foo  ", &lastWasSpace, &newlineCount, /*trimTail=*/false);
  String b = compactText("  bar", &lastWasSpace, &newlineCount, /*trimTail=*/true);
  CHECK_EQ(a + b, String("foo bar"));
}

TEST_CASE("compactText streaming: newline run across chunk boundary collapses to two") {
  bool lastWasSpace = false;
  int newlineCount = 0;
  String a = compactText("foo\n\n", &lastWasSpace, &newlineCount, /*trimTail=*/false);
  String b = compactText("\n\nbar", &lastWasSpace, &newlineCount, /*trimTail=*/true);
  CHECK_EQ(a + b, String("foo\n\nbar"));
}

TEST_CASE("compactText streaming: trimTail=false preserves trailing whitespace") {
  bool lastWasSpace = false;
  int newlineCount = 0;
  String a = compactText("hello ", &lastWasSpace, &newlineCount, /*trimTail=*/false);
  CHECK_EQ(a, String("hello "));
  CHECK(lastWasSpace);
}

TEST_CASE("compactText streaming: final chunk with trimTail=true strips trailing") {
  bool lastWasSpace = false;
  int newlineCount = 0;
  String a = compactText("hello", &lastWasSpace, &newlineCount, /*trimTail=*/false);
  String b = compactText("   \n\n", &lastWasSpace, &newlineCount, /*trimTail=*/true);
  CHECK_EQ(a + b, String("hello"));
}

TEST_CASE("compactText streaming: no word merging across boundary") {
  // Chunk split mid-word: "fo"|"o bar" should still produce "foo bar"
  // (no whitespace at the split point, so nothing to merge or collapse).
  bool lastWasSpace = false;
  int newlineCount = 0;
  String a = compactText("fo", &lastWasSpace, &newlineCount, /*trimTail=*/false);
  String b = compactText("o bar", &lastWasSpace, &newlineCount, /*trimTail=*/true);
  CHECK_EQ(a + b, String("foo bar"));
}
