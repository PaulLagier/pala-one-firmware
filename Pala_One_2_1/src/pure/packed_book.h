#pragma once
#include <stdint.h>
#include <stddef.h>
#include <memory>
#include <new>
#include <algorithm>
#include "stream.h"

namespace PalaPacked {
constexpr uint32_t BlockSize = 4096;
constexpr uint32_t MaximumSize = 2 * 1024 * 1024;
constexpr uint32_t MaximumPhysicalSize = 2103316;

inline uint32_t read32(const uint8_t* bytes) {
  return uint32_t(bytes[0]) | uint32_t(bytes[1]) << 8 |
         uint32_t(bytes[2]) << 16 | uint32_t(bytes[3]) << 24;
}

inline uint32_t crc32(const uint8_t* data, size_t length) {
  uint32_t crc = 0xffffffffU;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (unsigned bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320U : 0U);
  }
  return ~crc;
}

struct Header {
  uint32_t size = 0;
  uint32_t blocks = 0;
};

// Allocation-free inspection, reading the offset table sequentially rather
// than issuing a separate filesystem seek for every four-byte entry.
template<class Source> bool inspect(Source& source, Header& result) {
  result = Header{};
  uint8_t header[16];
  if (!source.seek(0) || source.read(header, sizeof(header)) != sizeof(header) ||
      header[0] != 'P' || header[1] != 'B' || header[2] != 'T' || header[3] != '1')
    return false;
  const uint32_t size = read32(header + 4);
  const uint32_t count = read32(header + 12);
  if (!size || size > MaximumSize || read32(header + 8) != BlockSize ||
      count != (size + BlockSize - 1) / BlockSize)
    return false;

  const size_t physicalSize = source.size();
  const uint32_t tableEnd = 16 + 4 * (count + 1);
  uint32_t previous = tableEnd;
  uint32_t index = 0;
  uint8_t table[256];
  while (index <= count) {
    const uint32_t entries = (std::min)(uint32_t(sizeof(table) / 4), count + 1 - index);
    if (source.read(table, entries * 4) != entries * 4) return false;
    for (uint32_t i = 0; i < entries; ++i, ++index) {
      const uint32_t next = read32(table + 4 * i);
      if (index == 0) {
        if (next != tableEnd) return false;
      } else if (next <= previous + 8 || next - previous > BlockSize + 8 || next > physicalSize) {
        return false;
      }
      previous = next;
    }
  }
  if (previous != physicalSize) return false;
  result.size = size;
  result.blocks = count;
  return true;
}

// Source supplies read/seek/size like Arduino File. The bounded inflate
// callback allows this same container reader to run under host sanitizers.
// Each instance owns its source's cursor and caches one decoded block.
template<class Source> class Stream : public IReadStream {
public:
  using Inflate = bool (*)(const uint8_t*, size_t, uint8_t*, size_t);

  Stream(Source& source, Inflate inflate) : source_(source), inflate_(inflate) {
    Header header;
    if (!inspect(source_, header)) return;
    size_ = header.size;
    count_ = header.blocks;
    output_.reset(new(std::nothrow) uint8_t[BlockSize]);
    input_.reset(new(std::nothrow) uint8_t[BlockSize]);
    valid_ = bool(output_) && bool(input_);
  }

  bool valid() const { return valid_; }
  int read() override {
    if (!valid_ || pos_ >= size_) return -1;
    const uint32_t block = pos_ / BlockSize;
    if (cached_ != block && !load(block)) {
      valid_ = false;
      return -1;
    }
    return output_[pos_++ % BlockSize];
  }
  bool seek(uint32_t pos) override {
    if (!valid_ || pos > size_) return false;
    pos_ = pos;
    return true;
  }
  uint32_t position() override { return pos_; }
  size_t size() override { return valid_ ? size_ : 0; }
  bool available() override { return valid_ && pos_ < size_; }

private:
  bool exact(uint8_t* data, size_t length) {
    return source_.read(data, length) == length;
  }
  bool offset(uint32_t block, uint32_t& value) {
    uint8_t bytes[4];
    if (!source_.seek(16 + 4 * block) || !exact(bytes, sizeof(bytes))) return false;
    value = read32(bytes);
    return true;
  }
  bool load(uint32_t block) {
    uint32_t start, end;
    uint8_t header[8];
    if (!offset(block, start) || !offset(block + 1, end)) return false;
    // Recheck bounds at use as well: a source can fail/change after inspection.
    const uint32_t tableEnd = 16 + 4 * (count_ + 1);
    if (start < tableEnd || end <= start || end > source_.size() || end - start > BlockSize + 8)
      return false;
    if (!source_.seek(start) || !exact(header, sizeof(header))) return false;
    const uint32_t stored = read32(header);
    const uint32_t length = stored & 0x7fffffffU;
    const size_t expected = (std::min)(BlockSize, size_ - block * BlockSize);
    if (!length || length > BlockSize || end - start != length + 8 || !exact(input_.get(), length))
      return false;
    if (stored & 0x80000000U) {
      if (length != expected) return false;
      std::copy_n(input_.get(), length, output_.get());
    } else if (!inflate_ || !inflate_(input_.get(), length, output_.get(), expected)) {
      return false;
    }
    if (crc32(output_.get(), expected) != read32(header + 4)) return false;
    cached_ = block;
    return true;
  }

  Source& source_;
  Inflate inflate_;
  std::unique_ptr<uint8_t[]> output_, input_;
  uint32_t size_ = 0, count_ = 0, pos_ = 0, cached_ = 0xffffffffU;
  bool valid_ = false;
};
}  // namespace PalaPacked
