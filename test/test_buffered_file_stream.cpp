#include "test_framework.h"
#include "src/hal/file_stream.h"
#include "src/pure/paginator.h"

static std::string patternedBytes(size_t count) {
  std::string out;
  for (size_t i = 0; i < count; ++i) out += char('a' + i % 26);
  return out;
}

TEST_CASE("HAL buffered stream reads sequential blocks without filesystem seeks") {
  File file{patternedBytes(FILE_STREAM_BUF_BYTES * 5 + 17)};
  BufferedFileReadStream stream(file);
  for (size_t i = 0; i < file.bytes.size(); ++i) {
    CHECK_EQ(stream.position(), i);
    CHECK_EQ(stream.read(), int(file.bytes[i]));
  }
  CHECK_EQ(file.reads, 6u);
  CHECK_EQ(file.seeks, 0u);
  CHECK_EQ(stream.read(), -1);
}

TEST_CASE("HAL buffered lookahead retains the physical refill cursor") {
  File file{patternedBytes(FILE_STREAM_BUF_BYTES * 3)};
  BufferedFileReadStream stream(file);
  CHECK_EQ(stream.read(), int(file.bytes[0]));
  REQUIRE(stream.seek(FILE_STREAM_BUF_BYTES - 1));
  CHECK_EQ(stream.read(), int(file.bytes[FILE_STREAM_BUF_BYTES - 1]));
  REQUIRE(stream.seek(0));
  for (size_t i = 0; i <= FILE_STREAM_BUF_BYTES; ++i)
    CHECK_EQ(stream.read(), int(file.bytes[i]));
  CHECK_EQ(file.seeks, 0u);
  CHECK_EQ(file.reads, 2u);
  REQUIRE(stream.seek(0)); // Older window: this time an actual seek is needed.
  CHECK_EQ(stream.read(), int(file.bytes[0]));
  CHECK_EQ(file.seeks, 1u);
}

TEST_CASE("HAL buffered stream handles nonzero starts short reads and EOF") {
  File file{patternedBytes(2000)};
  file.cursor = 37;
  file.shortLimit = 11;
  BufferedFileReadStream stream(file);
  for (size_t i = 37; i < file.bytes.size(); ++i)
    CHECK_EQ(stream.read(), int(file.bytes[i]));
  CHECK_EQ(file.seeks, 0u);
  REQUIRE(stream.seek(file.bytes.size()));
  CHECK_EQ(stream.read(), -1);
  CHECK(!stream.seek(file.bytes.size() + 1));
  CHECK_EQ(stream.position(), file.bytes.size());
}

TEST_CASE("HAL buffered stream recovers from failed seeks and zero-byte reads") {
  File file{patternedBytes(FILE_STREAM_BUF_BYTES * 4)};
  BufferedFileReadStream stream(file);
  CHECK_EQ(stream.read(), int(file.bytes[0]));
  file.failSeek = true;
  REQUIRE(stream.seek(FILE_STREAM_BUF_BYTES * 2));
  CHECK_EQ(stream.read(), -1);
  file.failSeek = false;
  CHECK_EQ(stream.read(), int(file.bytes[FILE_STREAM_BUF_BYTES * 2]));
  REQUIRE(stream.seek(0));
  file.failRead = true;
  CHECK_EQ(stream.read(), -1);
  file.failRead = false;
  // The failed read followed a real seek. The old window end is no longer
  // the physical cursor, even though that old cached window is still valid.
  REQUIRE(stream.seek(FILE_STREAM_BUF_BYTES * 3));
  CHECK_EQ(stream.read(), int(file.bytes[FILE_STREAM_BUF_BYTES * 3]));
}

TEST_CASE("HAL buffered pagination preserves text and page offsets") {
  std::string text;
  for (int i = 0; i < 1000; ++i) text += "hello café world with wrapping words\n\n";
  File file{text};
  BufferedFileReadStream stream(file);
  StringReadStream reference(text.c_str());
  LayoutMetrics layout{};
  layout.maxWidth = 17;
  layout.maxLines = 4;
  auto measure = [](const char* s) { return int(std::strlen(s)); };
  uint32_t offset = 0;
  while (offset < text.size()) {
    std::string actual, expected;
    const auto next = paginatePage(stream, offset, layout, measure,
        [&](const char* s, size_t n) { actual.append(s, n); actual += '\n'; });
    CHECK_EQ(next, paginatePage(reference, offset, layout, measure,
        [&](const char* s, size_t n) { expected.append(s, n); expected += '\n'; }));
    CHECK(actual == expected);
    REQUIRE(next > offset);
    offset = next;
  }
  CHECK(file.reads <= (text.size() + FILE_STREAM_BUF_BYTES - 1) / FILE_STREAM_BUF_BYTES);
  CHECK_EQ(file.seeks, 0u);
}
