#include <cstring>
#include <vector>

#include "test_framework.h"
#include "pure/stream.h"
#include "pure/token_reader.h"

namespace {

// Helper: pull every token out of a string. Returns (bytes, end, startPos).
struct ReadOut {
  String bytes;
  TokenEnd end;
  uint32_t startPos;
};

std::vector<ReadOut> readAll(const char* text, size_t bufCap = 64) {
  StringReadStream in(text);
  std::vector<char> buf(bufCap);
  std::vector<ReadOut> out;
  while (true) {
    ScannedToken t;
    if (!readNextToken(in, buf.data(), buf.size(), t)) break;
    out.push_back({String(buf.data()), t.end, t.startPos});
    if (t.end == TokenEnd::Eof) break;
  }
  return out;
}

}  // namespace

TEST_CASE("token reader: single word ends with EOF") {
  auto out = readAll("hello");
  REQUIRE(out.size() == 1u);
  CHECK_EQ(out[0].bytes, String("hello"));
  CHECK(out[0].end == TokenEnd::Eof);
  CHECK_EQ(out[0].startPos, 0u);
}

TEST_CASE("token reader: two words split on space") {
  auto out = readAll("hello world");
  REQUIRE(out.size() == 2u);
  CHECK_EQ(out[0].bytes, String("hello"));
  CHECK(out[0].end == TokenEnd::Space);
  CHECK_EQ(out[0].startPos, 0u);
  CHECK_EQ(out[1].bytes, String("world"));
  CHECK(out[1].end == TokenEnd::Eof);
  CHECK_EQ(out[1].startPos, 6u);
}

TEST_CASE("token reader: newline terminates a token") {
  auto out = readAll("a\nb");
  REQUIRE(out.size() == 2u);
  CHECK_EQ(out[0].bytes, String("a"));
  CHECK(out[0].end == TokenEnd::Newline);
  CHECK_EQ(out[1].bytes, String("b"));
  CHECK(out[1].end == TokenEnd::Eof);
}

TEST_CASE("token reader: punctuation is kept with the token") {
  auto out = readAll("hello, world.");
  // hello, (Punctuation) | "" (Space, from the space between) | world. (Punctuation)
  REQUIRE(out.size() == 3u);
  CHECK_EQ(out[0].bytes, String("hello,"));
  CHECK(out[0].end == TokenEnd::Punctuation);
  CHECK_EQ(out[1].bytes, String(""));
  CHECK(out[1].end == TokenEnd::Space);
  CHECK_EQ(out[2].bytes, String("world."));
  CHECK(out[2].end == TokenEnd::Punctuation);
}

TEST_CASE("token reader: empty token between consecutive separators") {
  auto out = readAll("a  b");
  // a -> Space, "" -> Space, b -> Eof
  REQUIRE(out.size() == 3u);
  CHECK_EQ(out[0].bytes, String("a"));
  CHECK(out[0].end == TokenEnd::Space);
  CHECK_EQ(out[1].bytes, String(""));
  CHECK(out[1].end == TokenEnd::Space);
  CHECK_EQ(out[2].bytes, String("b"));
}

TEST_CASE("token reader: leading separator produces empty token first") {
  auto out = readAll("\nhello");
  REQUIRE(out.size() == 2u);
  CHECK_EQ(out[0].bytes, String(""));
  CHECK(out[0].end == TokenEnd::Newline);
  CHECK_EQ(out[1].bytes, String("hello"));
  CHECK(out[1].end == TokenEnd::Eof);
}

TEST_CASE("token reader: CR is silently stripped") {
  auto out = readAll("a\r\nb");
  REQUIRE(out.size() == 2u);
  CHECK_EQ(out[0].bytes, String("a"));
  CHECK(out[0].end == TokenEnd::Newline);
  CHECK_EQ(out[1].bytes, String("b"));
}

TEST_CASE("token reader: BufferFull splits an oversized token and stream resumes correctly") {
  // bufCap=6 -> 5 usable bytes per call. "abcdefgh" -> "abcde" BufferFull, then "fgh" Eof.
  auto out = readAll("abcdefgh", /*bufCap=*/6);
  REQUIRE(out.size() == 2u);
  CHECK_EQ(out[0].bytes, String("abcde"));
  CHECK(out[0].end == TokenEnd::BufferFull);
  CHECK_EQ(out[0].startPos, 0u);
  CHECK_EQ(out[1].bytes, String("fgh"));
  CHECK(out[1].end == TokenEnd::Eof);
  CHECK_EQ(out[1].startPos, 5u);
}

TEST_CASE("token reader: startPos points at first non-separator byte of the token") {
  auto out = readAll("  hello");
  // "" Space at 0, "" Space at 1, "hello" Eof at 2
  REQUIRE(out.size() == 3u);
  CHECK(out[0].end == TokenEnd::Space);
  CHECK_EQ(out[0].startPos, 0u);
  CHECK(out[1].end == TokenEnd::Space);
  CHECK_EQ(out[1].startPos, 1u);
  CHECK_EQ(out[2].bytes, String("hello"));
  CHECK_EQ(out[2].startPos, 2u);
}

TEST_CASE("token reader: consecutive punctuation glues into one token") {
  // `?!`, `:-)`, `...`, `(hi)?` etc. should stay together so none of the
  // individual punct chars can be orphaned at line-end.
  {
    auto out = readAll("really?! Are");
    REQUIRE(out.size() == 3u);
    CHECK_EQ(out[0].bytes, String("really?!"));
    CHECK(out[0].end == TokenEnd::Punctuation);
  }
  {
    auto out = readAll("hi :-) there");
    REQUIRE(out.size() == 4u);
    CHECK_EQ(out[1].bytes, String(":-)"));
    CHECK(out[1].end == TokenEnd::Punctuation);
  }
}

TEST_CASE("token reader: hyphenated words still split (single hyphen is breakable)") {
  // Sanity: glue rule only kicks in once we're already in a punct token,
  // and only consumes *trailing* punct. A leading letter token still ends
  // cleanly at the hyphen.
  auto out = readAll("Rabbit-Hole");
  REQUIRE(out.size() == 2u);
  CHECK_EQ(out[0].bytes, String("Rabbit-"));
  CHECK(out[0].end == TokenEnd::Punctuation);
  CHECK_EQ(out[1].bytes, String("Hole"));
}

TEST_CASE("token reader: trailing closing quote glues to the punctuation token") {
  auto out = readAll("said \"last?\" next");
  // said (Space), "last?"  (Punctuation, with the trailing " glued on),
  // "" (Space, between the close quote and the next word), next (Eof).
  REQUIRE(out.size() == 4u);
  CHECK_EQ(out[0].bytes, String("said"));
  CHECK(out[0].end == TokenEnd::Space);
  CHECK_EQ(out[1].bytes, String("\"last?\""));
  CHECK(out[1].end == TokenEnd::Punctuation);
  CHECK_EQ(out[2].bytes, String(""));
  CHECK(out[2].end == TokenEnd::Space);
  CHECK_EQ(out[3].bytes, String("next"));
}

TEST_CASE("token reader: at EOF with empty stream returns false") {
  StringReadStream in("");
  char buf[16];
  ScannedToken t;
  CHECK(readNextToken(in, buf, sizeof(buf), t) == false);
  CHECK(t.end == TokenEnd::Eof);
  CHECK_EQ(t.len, 0u);
}
