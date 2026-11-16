#pragma once
#include "stream.h"

// Keep file-system calls off the byte-by-byte pagination path. The logical
// cursor is separate from the read-ahead cursor in File.
template <typename Source>
class BufferedReadStream : public IReadStream {
public:
  explicit BufferedReadStream(Source& f) : f_(f), size_(f.size()), pos_(f.position()) {}
  int read() override {
    if (pos_ >= size_) return -1;
    if (used_ == filled_) {
      filled_ = f_.read(buffer_, sizeof(buffer_));
      used_ = 0;
      if (filled_ == 0) return -1;
    }
    ++pos_;
    return buffer_[used_++];
  }
  bool seek(uint32_t pos) override {
    if (pos > size_) return false;
    // Paginator look-ahead often seeks back within the current block. Keep
    // that block and its physical read-ahead cursor rather than re-reading.
    const uint32_t base = pos_ - uint32_t(used_);
    if (filled_ && pos >= base && uint64_t(pos) <= uint64_t(base) + filled_) {
      used_ = pos - base;
      pos_ = pos;
      return true;
    }
    if (!f_.seek(pos)) return false;
    pos_ = pos;
    used_ = filled_ = 0;
    return true;
  }
  uint32_t position() override { return pos_; }
  size_t size() override { return size_; }
  bool available() override { return pos_ < size_; }
private:
  Source& f_;
  const size_t size_;
  uint32_t pos_;
  uint8_t buffer_[1024];
  size_t used_ = 0, filled_ = 0;
};

