#include "src/storage/page_cache.h"
#include "src/pure/hashing.h"
#include "src/pure/packed_book.h"
#include <limits.h>

namespace {
// Legacy prefix caches remain readable. New caches include their absolute
// first page and a CRC over the header and offsets, even for a prefix.
constexpr uint32_t kPrefixMagic = 0x50434F4AUL;
constexpr uint32_t kWindowMagic = 0x50434F4CUL;
constexpr uint32_t kPrivateWindowMagic = 0x50434F4BUL;
struct Header {
  uint32_t first = 0;
  uint16_t count = 0;
  size_t bytes = 14;
};
uint32_t encodeLayoutVersion(const PageCacheLayout &layout) {
  // Bit7 identifies extended glyphs; bit22 identifies adaptive compact page heights.
  // Reject old-font caches while preserving canonical book byte positions.
  return (1UL << 22) | uint32_t((layout.bodySize & 127) | 128) |
         (uint32_t(layout.lineGap & 255) << 8) | (uint32_t(layout.family & 15) << 16) |
         (uint32_t(layout.bionic & 1) << 20) | (uint32_t(layout.halfGaps & 1) << 21) |
         (uint32_t(layout.statusbarReserve) << 24);
}
String pageCachePathForBook(const String &path) {
  return String("/pc_") + prefKeyForBook(path) + ".bin";
}
bool validOffset(uint32_t offset, int local, uint32_t previous, const Header &header, size_t size) {
  if (offset >= size)
    return false;
  if (local)
    return offset > previous;
  return header.first == 0 ? offset == 0 : offset > 0 && offset >= header.first;
}
bool openAndValidateCache(const String &path, size_t size, const PageCacheLayout &layout,
                          File &file, Header &header) {
  file = FS.open(pageCachePathForBook(path), "r");
  if (!file)
    return false;
  uint32_t magic = 0, stamp = 0, storedSize = 0, storedCrc = 0;
  auto read = [&](auto &value) {
    return file.read(reinterpret_cast<uint8_t *>(&value), sizeof(value)) == sizeof(value);
  };
  if (!read(magic) || !read(stamp) || !read(storedSize))
    return false;
  if (magic == kWindowMagic || magic == kPrivateWindowMagic) {
    header.bytes = 22;
    if (!read(header.first))
      return false;
  } else if (magic != kPrefixMagic)
    return false;
  if (!read(header.count))
    return false;
  if ((magic == kWindowMagic || magic == kPrivateWindowMagic) && !read(storedCrc))
    return false;
  if (!size || size > INT_MAX || storedSize != size || stamp != encodeLayoutVersion(layout) ||
      !header.count || header.count > MAX_PAGES || uint64_t(header.first) + header.count > size ||
      file.size() != header.bytes + size_t(header.count) * sizeof(uint32_t))
    return false;
  uint32_t crc = 0;
  auto hash = [&](const auto &value) {
    crc = PalaPacked::crc32(reinterpret_cast<const uint8_t *>(&value), sizeof(value), crc);
  };
  hash(magic);
  hash(stamp);
  hash(storedSize);
  hash(header.first);
  hash(header.count);
  uint32_t previous = 0, block[64];
  for (int base = 0; base < header.count;) {
    const size_t n = min(size_t(64), size_t(header.count - base));
    if (file.read(reinterpret_cast<uint8_t *>(block), n * sizeof(uint32_t)) != n * sizeof(uint32_t))
      return false;
    if (magic == kWindowMagic || magic == kPrivateWindowMagic)
      crc = PalaPacked::crc32(reinterpret_cast<const uint8_t *>(block), n * sizeof(uint32_t), crc);
    for (size_t j = 0; j < n; ++j) {
      if (!validOffset(block[j], base + int(j), previous, header, size))
        return false;
      previous = block[j];
    }
    base += int(n);
  }
  return ((magic != kWindowMagic && magic != kPrivateWindowMagic) || crc == storedCrc) &&
         file.seek(header.bytes);
}
} // namespace

bool loadPageOffsetCacheForBook(const String &path, size_t expectedSize,
                                const PageCacheLayout &layout, PageOffsetTable &out) {
  out.reset();
  File file;
  Header header;
  if (!openAndValidateCache(path, expectedSize, layout, file, header)) {
    file.close();
    return false;
  }
  const size_t bytes = size_t(header.count) * sizeof(uint32_t);
  if (file.read(reinterpret_cast<uint8_t *>(out.offsets), bytes) != bytes) {
    file.close();
    out.reset();
    return false;
  }
  file.close();
  out.firstPage = int(header.first);
  out.count = header.count;
  out.eofReached = false;
  return true;
}
void savePageOffsetCacheForBook(const String &path, size_t fileSize, const PageCacheLayout &layout,
                                const PageOffsetTable &in) {
  if (in.count <= 1 || in.count > MAX_PAGES || in.firstPage < 0 || fileSize > INT_MAX ||
      uint64_t(in.firstPage) + in.count > fileSize)
    return;
  Header header;
  header.first = uint32_t(in.firstPage);
  header.count = uint16_t(in.count);
  uint32_t previous = 0;
  for (int i = 0; i < in.count; ++i) {
    if (!validOffset(in.offsets[i], i, previous, header, fileSize))
      return;
    previous = in.offsets[i];
  }
  const String target = pageCachePathForBook(path), temporary = target + ".tmp";
  File file = FS.open(temporary, "w");
  if (!file)
    return;
  const uint32_t magic = kWindowMagic, stamp = encodeLayoutVersion(layout),
                 size = uint32_t(fileSize);
  auto write = [&](const auto &value) {
    return file.write(reinterpret_cast<const uint8_t *>(&value), sizeof(value)) == sizeof(value);
  };
  uint32_t crc = 0;
  auto hash = [&](const auto &value) {
    crc = PalaPacked::crc32(reinterpret_cast<const uint8_t *>(&value), sizeof(value), crc);
  };
  hash(magic);
  hash(stamp);
  hash(size);
  hash(header.first);
  hash(header.count);
  crc = PalaPacked::crc32(reinterpret_cast<const uint8_t *>(in.offsets),
                          size_t(in.count) * sizeof(uint32_t), crc);
  bool ok = write(magic) && write(stamp) && write(size) && write(header.first) &&
            write(header.count) && write(crc) &&
            file.write(reinterpret_cast<const uint8_t *>(in.offsets),
                       size_t(in.count) * sizeof(uint32_t)) == size_t(in.count) * sizeof(uint32_t);
  file.close();
  // A full storage device or failed rename must retain the previous cache.
  if (!ok || !FS.rename(temporary, target))
    FS.remove(temporary);
}
int loadOffsetForPageFromDisk(const String &path, size_t expectedSize,
                              const PageCacheLayout &layout, int maxPage, uint32_t *out) {
  if (maxPage < 0 || !out)
    return -1;
  File file;
  Header header;
  if (!openAndValidateCache(path, expectedSize, layout, file, header)) {
    file.close();
    return -1;
  }
  if (uint32_t(maxPage) < header.first) {
    file.close();
    return -1;
  }
  const int local = min(maxPage - int(header.first), int(header.count) - 1);
  uint32_t offset = 0;
  const bool ok = file.seek(header.bytes + size_t(local) * sizeof(uint32_t)) &&
                  file.read(reinterpret_cast<uint8_t *>(&offset), sizeof(offset)) == sizeof(offset);
  file.close();
  if (!ok)
    return -1;
  *out = offset;
  return int(header.first) + local;
}
void deletePageCacheForBook(const String &path) {
  const String cache = pageCachePathForBook(path);
  if (FS.exists(cache))
    FS.remove(cache);
}
void renamePageCacheForBook(const String &oldPath, const String &newPath) {
  const String oldCache = pageCachePathForBook(oldPath);
  if (!FS.exists(oldCache))
    return;
  const String newCache = pageCachePathForBook(newPath);
  if (FS.exists(newCache))
    FS.remove(newCache);
  FS.rename(oldCache, newCache);
}
