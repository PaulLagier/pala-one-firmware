#ifndef PALA_HAL_FILE_STREAM_H
#define PALA_HAL_FILE_STREAM_H

#include "src/state.h"
#include "src/pure/stream.h"

// Arduino `File` adapter for the pure IReadStream interface.
class FileReadStream : public IReadStream {
public:
  explicit FileReadStream(File& f) : f_(f) {}

  int read() override {
    if (!f_ || !f_.available()) return -1;
    return f_.read();
  }
  bool seek(uint32_t pos) override { return f_.seek(pos); }
  uint32_t position() override { return (uint32_t)f_.position(); }
  size_t size() override { return f_.size(); }
  bool available() override { return f_.available() > 0; }

private:
  File& f_;
};

// Buffered variant of the FileReadStream that reads in FILE_STREAM_BUF_BYTES
// at a time from the file and then hands them out one by one in calls to read.
// This avoids traversing the whole stack up and down (twice, once to check available
// and once to read) to the file system for each and every byte read.
// IMPORTANT: The tradeoff here is that the position and seek are calls are handled
// virtually within the buffer layer and wont necessarily be reflected on the underlying
// file object. At the time of writing this is okay because all callsites perform an
// explicit seek before starting to read, but care should be taken for new callsites.
class BufferedFileReadStream : public IReadStream {
public:
  explicit BufferedFileReadStream(File& f)
      : f_(f), size_(static_cast<uint32_t>(f.size())), pos_(static_cast<uint32_t>(f.position())) {
    buf_ = static_cast<uint8_t*>(malloc(kBufCap));
  }
  ~BufferedFileReadStream() override { free(buf_); }

  int read() override {
    if (pos_ >= size_) return -1;
    if (!buf_) {
      if (!f_.seek(pos_)) return -1;
      int b = f_.read();
      if (b >= 0) pos_++;
      return b;
    }
    if (pos_ < winStart_ || pos_ >= winStart_ + winLen_) {
      if (!fill(pos_)) return -1;
    }
    return buf_[pos_++ - winStart_];
  }

  bool seek(uint32_t pos) override {
    if (pos > size_) return false;
    pos_ = pos;
    return true;
  }

  uint32_t position() override { return pos_; }
  size_t size() override { return size_; }
  bool available() override { return pos_ < size_; }

private:
  bool fill(uint32_t at) {
    if (!f_.seek(at)) return false;
    size_t n = f_.read(buf_, kBufCap);
    if (n == 0) return false;
    winStart_ = at;
    winLen_ = (uint32_t)n;
    return true;
  }

  static constexpr size_t kBufCap = FILE_STREAM_BUF_BYTES;

  File&    f_;
  uint8_t* buf_;
  uint32_t size_;
  uint32_t pos_;
  uint32_t winStart_ = 0;
  uint32_t winLen_ = 0;
};

#endif  // PALA_HAL_FILE_STREAM_H
