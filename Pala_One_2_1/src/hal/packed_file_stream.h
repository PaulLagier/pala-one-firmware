#pragma once
#include "src/hal/file_stream.h"
#include "src/pure/packed_book.h"
#include <miniz.h>

// SDK decoder state is too large for the Arduino task stack. Each call has
// bounded input/output and accepts exactly one complete zlib stream.
inline bool inflateBookBlock(const uint8_t* input, size_t inputSize,
                             uint8_t* output, size_t outputSize) {
  std::unique_ptr<tinfl_decompressor> state(new(std::nothrow) tinfl_decompressor);
  if (!state) return false;
  tinfl_init(state.get());
  size_t consumed = inputSize, produced = outputSize;
  const auto status = tinfl_decompress(state.get(), input, &consumed, output,
      output, &produced, TINFL_FLAG_PARSE_ZLIB_HEADER |
      TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
  return status == TINFL_STATUS_DONE && consumed == inputSize && produced == outputSize;
}
inline bool isPackedBook(File& file) {
  return String(file.path()).endsWith(".pbt");
}
inline size_t bookLogicalSize(File& file) {
  if (!isPackedBook(file)) return file.size();
  const auto position = file.position();
  PalaPacked::Header header;
  const bool valid = PalaPacked::inspect(file, header);
  const bool restored = file.seek(position);
  return valid && restored ? header.size : 0;
}

// A stream owns the file cursor. Keep it alive for an active book/index scan;
// never use the underlying File cursor concurrently with this adapter.
class BookReadStream : public IReadStream {
public:
  explicit BookReadStream(File& file) : packedFile_(isPackedBook(file)) {
    if (packedFile_) packed_.reset(new(std::nothrow) PalaPacked::Stream<File>(file, inflateBookBlock));
    else plain_.reset(new(std::nothrow) BufferedFileReadStream(file));
  }
  int read() override { auto* s = stream(); return s ? s->read() : -1; }
  bool seek(uint32_t at) override { auto* s = stream(); return s && s->seek(at); }
  uint32_t position() override { auto* s = stream(); return s ? s->position() : 0; }
  size_t size() override { auto* s = stream(); return s ? s->size() : 0; }
  bool available() override { auto* s = stream(); return s && s->available(); }
  bool valid() const { return packedFile_ ? (packed_ && packed_->valid()) : bool(plain_); }
private:
  IReadStream* stream() { return packedFile_ ? static_cast<IReadStream*>(packed_.get()) : plain_.get(); }
  bool packedFile_;
  std::unique_ptr<BufferedFileReadStream> plain_;
  std::unique_ptr<PalaPacked::Stream<File>> packed_;
};

// Upload staging names end in .tmp; validate the container independently of
// its temporary extension. Every block and CRC must pass before atomic rename.
inline bool validatePackedBook(File& file) {
  PalaPacked::Stream<File> stream(file, inflateBookBlock);
  if (!stream.valid()) return false;
  const size_t expected = stream.size();
  size_t count = 0;
  while (stream.read() >= 0) ++count;
  return stream.valid() && count == expected;
}
