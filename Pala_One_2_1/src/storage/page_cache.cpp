#include "src/storage/page_cache.h"

#include "src/pure/hashing.h"   // prefKeyForBook

// ============================================================================
//  On-disk page-offset cache
//
//  File format (little-endian):
//    uint32 magic            kPageCacheMagic
//    uint32 layoutVersion    encodeLayoutVersion(layout)  — bodySize/lineGap/family/bionic
//    uint32 fileSize         source-book size at save time
//    uint16 count            number of entries that follow
//    uint32 offsets[count]   byte offsets of pages 0..count-1
//
//  Magic history:
//    0x50434F46 — original, no stamp
//    0x50434F47 — added 16-bit `(bodySize, lineGap)` stamp
//    0x50434F48 — widened stamp to 32 bits to also cover font family + bionic
//
//  Old files fail the magic check, get ignored, then overwritten on the next
//  save. No migration code needed.
// ============================================================================

static constexpr uint32_t kPageCacheMagic = 0x50434F48UL;

static constexpr size_t kHeaderBytes =
    sizeof(uint32_t)   // magic
  + sizeof(uint32_t)   // layoutVersion
  + sizeof(uint32_t)   // fileSize
  + sizeof(uint16_t);  // count

// Compact encoding of "what layout were the offsets in this file computed
// under?" — bodySize ∈ {8,10,12,14}, lineGap ∈ [0,4], family ∈ {0,1},
// bionic ∈ {0,1}. Each tucks into one byte, so a stride-by-256 packing is
// unambiguous and trivially injective.
static uint32_t encodeLayoutVersion(const PageCacheLayout& layout) {
  return ((uint32_t)(layout.bodySize & 0xFF))
       | ((uint32_t)(layout.lineGap  & 0xFF) << 8)
       | ((uint32_t)(layout.family   & 0xFF) << 16)
       | ((uint32_t)(layout.bionic   & 0xFF) << 24);
}

static String pageCachePathForBook(const String& path) {
  return String("/pc_") + prefKeyForBook(path) + ".bin";
}

// Open the cache file for `path`, read + validate the header (magic, layout
// stamp, expected source-file size, non-zero entry count), and return it
// positioned just past the header. On false, any opened file is closed and
// `outFile` / `outCount` are left untouched. Both load functions share this
// gate; only the work that follows differs.
static bool openAndValidateCache(const String& path, size_t expectedSize,
                                 const PageCacheLayout& layout,
                                 File& outFile, uint16_t& outCount) {
  File f = FS.open(pageCachePathForBook(path), "r");
  if (!f) return false;

  uint32_t magic = 0;
  uint32_t layoutVersion = 0;
  uint32_t fileSize = 0;
  uint16_t count = 0;

  if (f.read((uint8_t*)&magic, sizeof(magic)) != sizeof(magic))                         { f.close(); return false; }
  if (f.read((uint8_t*)&layoutVersion, sizeof(layoutVersion)) != sizeof(layoutVersion)) { f.close(); return false; }
  if (f.read((uint8_t*)&fileSize, sizeof(fileSize)) != sizeof(fileSize))                { f.close(); return false; }
  if (f.read((uint8_t*)&count, sizeof(count)) != sizeof(count))                         { f.close(); return false; }

  if (magic != kPageCacheMagic
      || layoutVersion != encodeLayoutVersion(layout)
      || fileSize != (uint32_t)expectedSize
      || count == 0) {
    f.close();
    return false;
  }

  outFile = f;
  outCount = count;
  return true;
}

bool loadPageOffsetCacheForBook(const String& path, size_t expectedSize,
                                const PageCacheLayout& layout,
                                PageOffsetTable& out) {
  File f;
  uint16_t count = 0;
  if (!openAndValidateCache(path, expectedSize, layout, f, count)) return false;
  if (count > MAX_PAGES) { f.close(); return false; }

  int loaded = 0;
  for (uint16_t i = 0; i < count; i++) {
    uint32_t off = 0;
    if (f.read((uint8_t*)&off, sizeof(off)) != sizeof(off)) break;
    out.offsets[i] = off;
    loaded++;
  }
  f.close();

  if (loaded == 0) return false;
  out.count = loaded;
  return true;
}

void savePageOffsetCacheForBook(const String& path, size_t fileSize,
                                const PageCacheLayout& layout,
                                const PageOffsetTable& in) {
  if (in.count <= 1) return;

  File f = FS.open(pageCachePathForBook(path), "w");
  if (!f) return;

  uint32_t magic = kPageCacheMagic;
  uint32_t layoutVersion = encodeLayoutVersion(layout);
  uint32_t size32 = (uint32_t)fileSize;
  uint16_t count16 = (uint16_t)min(in.count, MAX_PAGES);

  f.write((const uint8_t*)&magic, sizeof(magic));
  f.write((const uint8_t*)&layoutVersion, sizeof(layoutVersion));
  f.write((const uint8_t*)&size32, sizeof(size32));
  f.write((const uint8_t*)&count16, sizeof(count16));
  f.write((const uint8_t*)in.offsets, count16 * sizeof(uint32_t));
  f.close();
}

int loadOffsetForPageFromDisk(const String& path, size_t expectedSize,
                              const PageCacheLayout& layout,
                              int maxPage, uint32_t* out) {
  if (maxPage < 0) return -1;

  File f;
  uint16_t count = 0;
  if (!openAndValidateCache(path, expectedSize, layout, f, count)) return -1;

  int targetPage = (maxPage >= (int)count) ? (int)count - 1 : maxPage;
  size_t entryPos = kHeaderBytes + (size_t)targetPage * sizeof(uint32_t);
  if (!f.seek(entryPos)) { f.close(); return -1; }

  uint32_t off = 0;
  if (f.read((uint8_t*)&off, sizeof(off)) != sizeof(off)) { f.close(); return -1; }
  f.close();

  *out = off;
  return targetPage;
}

void deletePageCacheForBook(const String& path) {
  String cachePath = pageCachePathForBook(path);
  if (FS.exists(cachePath)) FS.remove(cachePath);
}

void renamePageCacheForBook(const String& oldPath, const String& newPath) {
  String oldCache = pageCachePathForBook(oldPath);
  if (!FS.exists(oldCache)) return;
  String newCache = pageCachePathForBook(newPath);
  if (FS.exists(newCache)) FS.remove(newCache);
  FS.rename(oldCache, newCache);
}
