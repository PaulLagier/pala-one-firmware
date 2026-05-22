#include <cstring>
#include <vector>

#include "test_framework.h"
#include "pure/app_header.h"

namespace {

// Build a synthetic app binary as a byte vector. Header at offset 0;
// payload bytes are zeros unless the test rewrites them.
std::vector<uint8_t> makeBin(uint32_t magic, uint32_t apiVersion,
                             uint32_t entryOffset, uint32_t relocOffset,
                             uint32_t relocCount, size_t totalSize) {
  std::vector<uint8_t> v(totalSize, 0);
  PalaAppHeader hdr{};
  hdr.magic        = magic;
  hdr.api_version  = apiVersion;
  std::strcpy(hdr.name, "test");
  hdr.entry_offset = entryOffset;
  hdr.reloc_offset = relocOffset;
  hdr.reloc_count  = relocCount;
  std::memcpy(v.data(), &hdr, sizeof(hdr));
  return v;
}

// Convenience: minimum-size valid binary with entry just past the header.
std::vector<uint8_t> makeValid() {
  return makeBin(PALA_APP_MAGIC, PALA_API_VERSION,
                 /*entry*/ sizeof(PalaAppHeader),
                 /*relocOff*/ 0, /*relocCount*/ 0,
                 /*size*/ sizeof(PalaAppHeader) + 4);
}

}  // namespace

TEST_CASE("validateAppHeader: minimal valid binary") {
  auto bin = makeValid();
  CHECK(validateAppHeader(bin.data(), bin.size()) == AppHeaderStatus::Ok);
}

TEST_CASE("validateAppHeader: too small") {
  // Header + 3 bytes is below the +4-byte payload floor.
  std::vector<uint8_t> v(sizeof(PalaAppHeader) + 3, 0);
  PalaAppHeader hdr{};
  hdr.magic = PALA_APP_MAGIC;
  hdr.api_version = PALA_API_VERSION;
  std::memcpy(v.data(), &hdr, sizeof(hdr));
  CHECK(validateAppHeader(v.data(), v.size()) == AppHeaderStatus::TooSmall);
}

TEST_CASE("validateAppHeader: bad magic") {
  auto bin = makeBin(0xDEADBEEFUL, PALA_API_VERSION,
                     sizeof(PalaAppHeader), 0, 0,
                     sizeof(PalaAppHeader) + 4);
  CHECK(validateAppHeader(bin.data(), bin.size()) == AppHeaderStatus::BadMagic);
}

TEST_CASE("validateAppHeader: api mismatch reports file's version") {
  uint32_t fileVer = PALA_API_VERSION + 99;
  auto bin = makeBin(PALA_APP_MAGIC, fileVer,
                     sizeof(PalaAppHeader), 0, 0,
                     sizeof(PalaAppHeader) + 4);
  uint32_t reportedVer = 0;
  CHECK(validateAppHeader(bin.data(), bin.size(), &reportedVer)
        == AppHeaderStatus::ApiMismatch);
  CHECK_EQ(reportedVer, fileVer);
}

TEST_CASE("validateAppHeader: entry_offset inside header is rejected") {
  auto bin = makeBin(PALA_APP_MAGIC, PALA_API_VERSION,
                     /*entry*/ 0,  // points into the header itself
                     0, 0, sizeof(PalaAppHeader) + 4);
  CHECK(validateAppHeader(bin.data(), bin.size())
        == AppHeaderStatus::BadEntryOffset);
}

TEST_CASE("validateAppHeader: entry_offset at fileSize is rejected") {
  size_t sz = sizeof(PalaAppHeader) + 4;
  auto bin = makeBin(PALA_APP_MAGIC, PALA_API_VERSION,
                     /*entry*/ (uint32_t)sz,  // one past the end
                     0, 0, sz);
  CHECK(validateAppHeader(bin.data(), bin.size())
        == AppHeaderStatus::BadEntryOffset);
}

TEST_CASE("validateAppHeader: reloc_count==0 ignores reloc_offset") {
  // Bogus reloc_offset is harmless when count is zero.
  auto bin = makeBin(PALA_APP_MAGIC, PALA_API_VERSION,
                     sizeof(PalaAppHeader),
                     /*relocOff*/ 0xFFFFFF00u,
                     /*relocCount*/ 0,
                     sizeof(PalaAppHeader) + 4);
  CHECK(validateAppHeader(bin.data(), bin.size()) == AppHeaderStatus::Ok);
}

TEST_CASE("validateAppHeader: reloc table overflowing file is rejected") {
  // Two entries (8 bytes) declared but file only has 4 bytes past the table.
  size_t totalSz = sizeof(PalaAppHeader) + 4 + 4; // header + 4 entry pad + 4 reloc bytes
  uint32_t relocOff = sizeof(PalaAppHeader) + 4;
  auto bin = makeBin(PALA_APP_MAGIC, PALA_API_VERSION,
                     sizeof(PalaAppHeader),
                     relocOff, /*count*/ 2, totalSz);
  CHECK(validateAppHeader(bin.data(), bin.size())
        == AppHeaderStatus::BadRelocTable);
}

TEST_CASE("validateAppHeader: reloc_offset inside header is rejected") {
  size_t totalSz = sizeof(PalaAppHeader) + 16;
  auto bin = makeBin(PALA_APP_MAGIC, PALA_API_VERSION,
                     sizeof(PalaAppHeader),
                     /*relocOff*/ 4,  // points into the header
                     /*count*/ 1, totalSz);
  CHECK(validateAppHeader(bin.data(), bin.size())
        == AppHeaderStatus::BadRelocTable);
}

TEST_CASE("validateAppHeader: reloc_offset past fileSize is rejected") {
  // (fileSize - reloc_offset) used to underflow when reloc_offset > fileSize,
  // letting any reloc_count slip through the avail/4 check.
  size_t totalSz = sizeof(PalaAppHeader) + 4;
  auto bin = makeBin(PALA_APP_MAGIC, PALA_API_VERSION,
                     sizeof(PalaAppHeader),
                     /*relocOff*/ (uint32_t)(totalSz + 100),
                     /*count*/ 1, totalSz);
  CHECK(validateAppHeader(bin.data(), bin.size())
        == AppHeaderStatus::BadRelocTable);
}

TEST_CASE("validateRelocEntries: empty reloc table is trivially ok") {
  auto bin = makeValid();
  CHECK(validateRelocEntries(bin.data(), bin.size()) == true);
}

TEST_CASE("validateRelocEntries: entry pointing before reloc table is ok") {
  size_t totalSz = sizeof(PalaAppHeader) + 16;
  uint32_t relocOff = sizeof(PalaAppHeader) + 4;
  auto bin = makeBin(PALA_APP_MAGIC, PALA_API_VERSION,
                     sizeof(PalaAppHeader),
                     relocOff, /*count*/ 1, totalSz);
  // Patch the single reloc entry to point at the entry slot.
  uint32_t entry = sizeof(PalaAppHeader);
  std::memcpy(bin.data() + relocOff, &entry, sizeof(entry));

  REQUIRE(validateAppHeader(bin.data(), bin.size()) == AppHeaderStatus::Ok);
  CHECK(validateRelocEntries(bin.data(), bin.size()) == true);
}

TEST_CASE("validateRelocEntries: entry pointing into reloc table is rejected") {
  size_t totalSz = sizeof(PalaAppHeader) + 16;
  uint32_t relocOff = sizeof(PalaAppHeader) + 4;
  auto bin = makeBin(PALA_APP_MAGIC, PALA_API_VERSION,
                     sizeof(PalaAppHeader),
                     relocOff, /*count*/ 1, totalSz);
  // Self-referential: points to the start of the reloc table itself.
  uint32_t selfRef = relocOff;
  std::memcpy(bin.data() + relocOff, &selfRef, sizeof(selfRef));

  REQUIRE(validateAppHeader(bin.data(), bin.size()) == AppHeaderStatus::Ok);
  CHECK(validateRelocEntries(bin.data(), bin.size()) == false);
}

TEST_CASE("validateRelocEntries: entry pointing into header is rejected") {
  // The loader does *(uint32_t*)(buf+off) += base; an entry pointing inside
  // the header would corrupt in-memory header state.
  size_t totalSz = sizeof(PalaAppHeader) + 16;
  uint32_t relocOff = sizeof(PalaAppHeader) + 4;
  auto bin = makeBin(PALA_APP_MAGIC, PALA_API_VERSION,
                     sizeof(PalaAppHeader),
                     relocOff, /*count*/ 1, totalSz);
  uint32_t insideHeader = 0;  // start of magic field
  std::memcpy(bin.data() + relocOff, &insideHeader, sizeof(insideHeader));

  REQUIRE(validateAppHeader(bin.data(), bin.size()) == AppHeaderStatus::Ok);
  CHECK(validateRelocEntries(bin.data(), bin.size()) == false);
}

TEST_CASE("validateRelocEntries: unaligned entry offset is rejected") {
  // Loader writes *(uint32_t*)(buf+off); off must be 4-byte aligned.
  size_t totalSz = sizeof(PalaAppHeader) + 16;
  uint32_t relocOff = sizeof(PalaAppHeader) + 4;
  auto bin = makeBin(PALA_APP_MAGIC, PALA_API_VERSION,
                     sizeof(PalaAppHeader),
                     relocOff, /*count*/ 1, totalSz);
  uint32_t unaligned = (uint32_t)sizeof(PalaAppHeader) + 1;
  std::memcpy(bin.data() + relocOff, &unaligned, sizeof(unaligned));

  REQUIRE(validateAppHeader(bin.data(), bin.size()) == AppHeaderStatus::Ok);
  CHECK(validateRelocEntries(bin.data(), bin.size()) == false);
}
