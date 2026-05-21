#include <vector>

#include "test_framework.h"
#include "pure/paginator.h"

namespace {

// Fixed-width measurer: every byte is 1 unit wide. Multi-byte UTF-8 chars are
// therefore "wider" than one column. That's fine for tests focused on
// wrapping behavior — the engine only cares about ordering of widths.
auto byteWidth = [](const char* s) -> int {
  return s ? (int)std::strlen(s) : 0;
};

LayoutMetrics m(int width, int lines) {
  LayoutMetrics lm;
  lm.maxWidth = width;
  lm.maxLines = lines;
  lm.ascent = 8;
  lm.descent = -2;
  lm.lineH = 12;
  return lm;
}

std::vector<String> linesOf(const String& text, const LayoutMetrics& metrics, uint32_t* outNext = nullptr) {
  StringReadStream in(text);
  std::vector<String> lines;
  uint32_t next = paginatePage(in, 0, metrics, byteWidth,
                               [&](const char* buf, size_t /*len*/) {
                                 lines.push_back(String(buf));
                               });
  if (outNext) *outNext = next;
  return lines;
}

}  // namespace

TEST_CASE("paginator wraps at spaces between words") {
  // Width 10 (chars), 3 lines max. "the quick brown fox jumps" — words split at spaces.
  auto lines = linesOf("the quick brown fox", m(10, 3));
  REQUIRE(lines.size() >= 1u);
  CHECK_EQ(lines[0], String("the quick"));  // 9 chars fits
}

TEST_CASE("paginator preserves explicit newlines as line breaks") {
  auto lines = linesOf("alpha\nbeta\ngamma", m(50, 5));
  REQUIRE(lines.size() == 3u);
  CHECK_EQ(lines[0], String("alpha"));
  CHECK_EQ(lines[1], String("beta"));
  CHECK_EQ(lines[2], String("gamma"));
}

TEST_CASE("paginator stops at maxLines and returns next offset") {
  String text = "one\ntwo\nthree\nfour\nfive";
  uint32_t next = 0;
  auto lines = linesOf(text, m(50, 2), &next);
  CHECK_EQ(lines.size(), 2u);
  CHECK_EQ(lines[0], String("one"));
  CHECK_EQ(lines[1], String("two"));
  CHECK(next > 0u);
  // Next offset should point past "one\ntwo\n"
  CHECK_EQ(next, (uint32_t)8);
}

TEST_CASE("paginator hard-breaks a single oversized word") {
  // Width 5, "abcdefghij" doesn't fit anywhere — must hard-break.
  auto lines = linesOf("abcdefghij", m(5, 5));
  REQUIRE(lines.size() == 2u);
  CHECK_EQ(lines[0], String("abcde"));
  CHECK_EQ(lines[1], String("fghij"));
}

TEST_CASE("paginator does not emit blank lines from leading whitespace") {
  auto lines = linesOf("   hello", m(50, 3));
  REQUIRE(lines.size() == 1u);
  CHECK_EQ(lines[0], String("hello"));
}

TEST_CASE("paginator trims trailing spaces on emitted lines") {
  auto lines = linesOf("hello   \nworld", m(50, 3));
  REQUIRE(lines.size() == 2u);
  CHECK_EQ(lines[0], String("hello"));
  CHECK_EQ(lines[1], String("world"));
}

TEST_CASE("paginator splits after punctuation when line would overflow") {
  // "abc,def" — width 4. After "abc," the line is 4 chars, then "def" added would overflow.
  auto lines = linesOf("abc,def", m(4, 3));
  REQUIRE(lines.size() == 2u);
  CHECK_EQ(lines[0], String("abc,"));
  CHECK_EQ(lines[1], String("def"));
}

TEST_CASE("paginator next-offset advances even on empty page") {
  StringReadStream in("");
  uint32_t next = paginatePage(in, 0, m(50, 3), byteWidth, nullptr);
  // safeReturn guarantees next > startPos
  CHECK(next >= 1u);
}

TEST_CASE("paginator handles UTF-8 multibyte characters without splitting them") {
  // "café" — 'é' is 2 bytes (0xC3 0xA9). Width 100 fits all; check single line.
  auto lines = linesOf("café", m(100, 3));
  REQUIRE(lines.size() == 1u);
  CHECK_EQ(lines[0], String("café"));
}

TEST_CASE("paginator merges oversized-token tail chunk with following text when it fits") {
  // "abcdefghij" is 10 chars on a width-8 line — must be split into a full
  // "abcdefgh" chunk plus a 2-char "ij" tail. The DP should then combine
  // "ij" + " " + "ok" onto the next line (5 chars, fits) instead of
  // emitting "ij" and "ok" as separate lines.
  auto lines = linesOf("abcdefghij ok", m(8, 5));
  REQUIRE(lines.size() == 2u);
  CHECK_EQ(lines[0], String("abcdefgh"));
  CHECK_EQ(lines[1], String("ij ok"));
}

TEST_CASE("paginator chunks paragraphs longer than the DP token-window cap") {
  // ~2100 single-letter "words" — well over kParagraphHardCap (2048). The
  // DP processes this in cap-sized windows across consecutive paginatePage
  // calls. Verify we paginate the whole thing without looping, with strict
  // forward progress and full coverage.
  String text;
  for (int i = 0; i < 2100; i++) text += "a ";

  StringReadStream in(text);
  uint32_t pos = 0;
  int pages = 0;
  int totalLines = 0;
  while (pos < (uint32_t)text.length() && pages < 2000) {
    int linesThisPage = 0;
    uint32_t newPos = paginatePage(in, pos, m(20, 5), byteWidth,
                                   [&](const char*, size_t) { linesThisPage++; });
    CHECK(newPos > pos);  // strict forward progress
    pos = newPos;
    totalLines += linesThisPage;
    pages++;
  }
  CHECK(pages > 1);                          // multi-page traversal
  CHECK(pos >= (uint32_t)text.length() - 1); // consumed (modulo trailing space)
  CHECK(totalLines >= 100);                  // lots of lines emitted
}

TEST_CASE("paginator line-plan cache produces identical output across runs") {
  // A long paragraph guarantees the DP stops mid-paragraph on the first
  // page and populates the cache. Re-paginating from offset 0 must produce
  // byte-identical lines, line-by-line, exercising the cache-hit fast path
  // on every page beyond the first.
  String text;
  for (int i = 0; i < 200; i++) text += "alpha beta gamma delta ";

  auto paginateAll = [&]() {
    std::vector<String> all;
    StringReadStream in(text);
    uint32_t pos = 0;
    while (pos < (uint32_t)text.length()) {
      uint32_t newPos = paginatePage(in, pos, m(40, 5), byteWidth,
                                     [&](const char* buf, size_t) {
                                       all.push_back(String(buf));
                                     });
      if (newPos <= pos) break;
      pos = newPos;
    }
    return all;
  };

  auto first = paginateAll();
  auto second = paginateAll();
  REQUIRE(first.size() == second.size());
  for (size_t i = 0; i < first.size(); i++) {
    CHECK_EQ(first[i], second[i]);
  }
}
