#include "src/pure/app_header.h"

#include <string.h>

// Read the header out of `buf` via memcpy so we don't rely on `buf` being
// aligned for uint32_t. The struct layout matches the on-disk byte layout
// (the build's Python post-step writes little-endian uint32s directly into
// the header fields; ESP32 + every host we care about is little-endian).
static void readHeader(const void* buf, PalaAppHeader* out) {
  memcpy(out, buf, sizeof(PalaAppHeader));
}

AppHeaderStatus validateAppHeader(const void* buf, size_t fileSize,
                                  uint32_t* outFileApiVersion) {
  // +4 matches the old loader: a header alone is meaningless; the smallest
  // sensible app has at least one instruction word past it.
  if (fileSize < sizeof(PalaAppHeader) + 4) return AppHeaderStatus::TooSmall;

  PalaAppHeader hdr;
  readHeader(buf, &hdr);

  if (hdr.magic != PALA_APP_MAGIC) return AppHeaderStatus::BadMagic;

  if (hdr.api_version != PALA_API_VERSION) {
    if (outFileApiVersion) *outFileApiVersion = hdr.api_version;
    return AppHeaderStatus::ApiMismatch;
  }

  if (hdr.entry_offset < sizeof(PalaAppHeader) || hdr.entry_offset >= fileSize) {
    return AppHeaderStatus::BadEntryOffset;
  }

  if (hdr.reloc_count > 0) {
    if (hdr.reloc_offset < sizeof(PalaAppHeader)) return AppHeaderStatus::BadRelocTable;
    // Bound reloc_offset against fileSize before the subtraction below, or
    // (fileSize - reloc_offset) underflows when reloc_offset > fileSize and
    // any reloc_count then passes the avail/4 check.
    if (hdr.reloc_offset > fileSize) return AppHeaderStatus::BadRelocTable;
    uint32_t avail = (uint32_t)(fileSize - hdr.reloc_offset);
    if (hdr.reloc_count > avail / 4u) return AppHeaderStatus::BadRelocTable;
  }

  return AppHeaderStatus::Ok;
}

bool validateRelocEntries(const void* buf, size_t fileSize) {
  PalaAppHeader hdr;
  readHeader(buf, &hdr);
  if (hdr.reloc_count == 0) return true;

  // validateAppHeader has been called first (precondition), so the table
  // itself is in range. Walk entries with memcpy to stay alignment-safe.
  const uint8_t* base = reinterpret_cast<const uint8_t*>(buf);
  for (uint32_t i = 0; i < hdr.reloc_count; i++) {
    uint32_t off;
    memcpy(&off, base + hdr.reloc_offset + i * 4u, sizeof(uint32_t));
    // Each entry must target a 4-byte-aligned slot strictly after the
    // header (the loader does *(uint32_t*)(buf+off) += base; targeting
    // header bytes corrupts in-memory state the loader still relies on)
    // and strictly before the reloc table itself (entries inside the
    // table would be self-referential).
    if (off + 4u < off)              return false;  // wraparound guard
    if (off & 3u)                    return false;  // alignment
    if (off < sizeof(PalaAppHeader)) return false;  // lower bound
    if (off + 4u > hdr.reloc_offset) return false;  // upper bound
  }
  (void)fileSize;  // reloc_offset is bounded by fileSize in validateAppHeader
  return true;
}
