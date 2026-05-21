#include "src/pure/stats_codec.h"

#include <cstring>  // memcpy

size_t encodeStats(const StatsFile& s, uint8_t* buf) {
  std::memcpy(buf, &s, sizeof(s));
  return sizeof(s);
}

bool decodeStats(const uint8_t* buf, size_t len, StatsFile& out) {
  if (len != STATS_ENCODED_SIZE) return false;
  std::memcpy(&out, buf, sizeof(out));
  if (out.version != STATS_SCHEMA) return false;
  return true;
}
