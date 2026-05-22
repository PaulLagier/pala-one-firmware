#include "test_framework.h"
#include "pure/text_util.h"
#include "pure/find_text.h"

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

TEST_CASE("find_text locates patterns with Boyer-Moore-Horspool") {
  CHECK_EQ(find_text("hello world", "world"), 6u);
  CHECK_EQ(find_text("hello world", "hello"), 0u);
  CHECK_EQ(find_text("hello world", "o w"), 4u);
  CHECK_EQ(find_text("hello world", ""), 0u);
  CHECK_EQ(find_text("hello world", "missing"), 0xFFFFFFFFu);
}

TEST_CASE("find_text handles repeated partial pattern matches and skip arithmetic") {
  // Text contains several prefixes of the pattern before the actual match.
  CHECK_EQ(find_text("abcxabcdabxabcdabcdabcy", "abcdabcy"), 15u);
  CHECK_EQ(find_text("aaaaaaab", "aaab"), 4u);
  CHECK_EQ(find_text("ababababaabababc", "abababc"), 9u);
}

// ============================================================================
//  findByteOffset — chunked streaming search
//
//  All cases use chunkSize=3 so the fixture strings force boundary crossings
//  without needing large buffers.
// ============================================================================

TEST_CASE("findByteOffset finds pattern entirely within the first chunk") {
  StringReadStream s("abcdef");
  CHECK_EQ(findByteOffset(s, String("ab"), 3), 0u);
  CHECK_EQ(findByteOffset(s, String("bc"), 3), 1u);
}

TEST_CASE("findByteOffset finds pattern that spans a chunk boundary") {
  // "cd" straddles the boundary between chunk "abc" and chunk "def".
  StringReadStream s("abcdef");
  CHECK_EQ(findByteOffset(s, String("cd"), 3), 2u);
}

TEST_CASE("findByteOffset finds pattern entirely in a later chunk") {
  StringReadStream s("abcdef");
  CHECK_EQ(findByteOffset(s, String("de"), 3), 3u);
  CHECK_EQ(findByteOffset(s, String("ef"), 3), 4u);
}

TEST_CASE("findByteOffset handles pattern longer than chunkSize") {
  // "cdefg" spans three chunks ("abc","def","ghi") with chunkSize=3.
  StringReadStream s("abcdefghi");
  CHECK_EQ(findByteOffset(s, String("cdefg"), 3), 2u);
}

TEST_CASE("findByteOffset returns 0xFFFFFFFFu when pattern is absent") {
  StringReadStream s("abcdef");
  CHECK_EQ(findByteOffset(s, String("xy"), 3), 0xFFFFFFFFu);
}

TEST_CASE("findByteOffset returns 0 for empty pattern") {
  StringReadStream s("abcdef");
  CHECK_EQ(findByteOffset(s, String(""), 3), 0u);
}

TEST_CASE("findByteOffset finds pattern at the very end of the stream") {
  StringReadStream s("abcdef");
  CHECK_EQ(findByteOffset(s, String("f"), 3), 5u);
}

TEST_CASE("findByteOffset is consistent with find_text on whole-string search") {
  const String text = "the quick brown fox jumps over the lazy dog";
  const String pattern = "lazy";
  StringReadStream s(text);
  CHECK_EQ(findByteOffset(s, pattern, 8), find_text(text, pattern));
}

TEST_CASE("findByteOffset is case-insensitive for ASCII") {
  StringReadStream s("Hello World");
  CHECK_EQ(findByteOffset(s, String("world"),  4), 6u);
  CHECK_EQ(findByteOffset(s, String("HELLO"),  4), 0u);
  CHECK_EQ(findByteOffset(s, String("hElLo"),  4), 0u);
  CHECK_EQ(findByteOffset(s, String("WORLD"),  4), 6u);
}

TEST_CASE("findByteOffset case-insensitive match spans chunk boundary") {
  // "LO W" straddles the chunk boundary between "Hel" and "lo World".
  StringReadStream s("Hello World");
  CHECK_EQ(findByteOffset(s, String("LO W"), 3), 3u);
}
