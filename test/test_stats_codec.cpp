// Tests for src/pure/stats_codec.cpp.
//
// The stats wire format is shared between the firmware (storage/stats.cpp)
// and the Stats example app (examples/stats/app.c). The codec is what
// gates a cold-boot reload — if a future schema change ships, decodeStats
// must reject the old version so the firmware reinitialises from zero
// rather than reading garbage. These tests pin that.

#include <cstring>

#include "test_framework.h"
#include "pure/stats_codec.h"

static StatsFile sample() {
  StatsFile s{};
  s.version       = STATS_SCHEMA;
  s.firstRtcSec   = 0xDEADBEEF;
  s.pagesRead     = 12345;
  s.buttonPresses = 67890;
  return s;
}

TEST_CASE("encode + decode round-trips StatsFile exactly") {
  StatsFile s = sample();
  uint8_t buf[STATS_ENCODED_SIZE];
  size_t n = encodeStats(s, buf);
  CHECK_EQ(n, STATS_ENCODED_SIZE);

  StatsFile out{};
  REQUIRE(decodeStats(buf, n, out));
  CHECK_EQ(out.version,       s.version);
  CHECK_EQ(out.firstRtcSec,   s.firstRtcSec);
  CHECK_EQ(out.pagesRead,     s.pagesRead);
  CHECK_EQ(out.buttonPresses, s.buttonPresses);
}

TEST_CASE("decodeStats rejects a wrong schema version") {
  StatsFile s = sample();
  s.version = 99;
  uint8_t buf[STATS_ENCODED_SIZE];
  encodeStats(s, buf);

  StatsFile out{};
  CHECK(!decodeStats(buf, STATS_ENCODED_SIZE, out));
}

TEST_CASE("decodeStats rejects a short buffer") {
  StatsFile s = sample();
  uint8_t buf[STATS_ENCODED_SIZE];
  encodeStats(s, buf);

  StatsFile out{};
  CHECK(!decodeStats(buf, STATS_ENCODED_SIZE - 1, out));
  CHECK(!decodeStats(buf, 0, out));
}

TEST_CASE("decodeStats rejects an oversized buffer") {
  // A longer-than-expected blob would mean the on-disk format grew; the
  // firmware should treat it as foreign and reinitialise, not slice off
  // the first STATS_ENCODED_SIZE bytes and hope they're still valid.
  StatsFile s = sample();
  uint8_t buf[STATS_ENCODED_SIZE + 16];
  std::memset(buf, 0, sizeof(buf));
  encodeStats(s, buf);

  StatsFile out{};
  CHECK(!decodeStats(buf, STATS_ENCODED_SIZE + 16, out));
}

TEST_CASE("STATS_ENCODED_SIZE matches the struct size") {
  // The example app reads/writes sizeof(StatsFile) directly. If this ever
  // drifts (added padding, layout change), the wire format breaks silently.
  CHECK_EQ(STATS_ENCODED_SIZE, sizeof(StatsFile));
}
