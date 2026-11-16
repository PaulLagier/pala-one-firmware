#ifndef PALA_HAL_FILE_STREAM_H
#define PALA_HAL_FILE_STREAM_H
#include "src/state.h"
#include "src/pure/buffered_stream.h"
#include "src/pure/packed_book.h"
#include <miniz.h>

// Reusable heap decoder: reserve its large state before allocating output blocks.
// The language switch shares one instance across all dictionary segments; books keep
// the original one-block lifetime. Never put miniz state on the Arduino stack.
class BookBlockInflater {
  std::unique_ptr<tinfl_decompressor> state_{new(std::nothrow) tinfl_decompressor};
public:
  bool ready()const{return bool(state_);}
  bool inflate(const uint8_t* input,size_t inputSize,uint8_t* output,size_t outputSize){
    if(!state_)return false;
    tinfl_init(state_.get());size_t in=inputSize,out=outputSize;
    const auto status=tinfl_decompress(state_.get(),input,&in,output,output,&out,TINFL_FLAG_PARSE_ZLIB_HEADER|TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    return status==TINFL_STATUS_DONE && in==inputSize && out==outputSize;
  }
};
inline bool inflateBookBlock(const uint8_t* input,size_t inputSize,uint8_t* output,size_t outputSize){
  BookBlockInflater decoder;return decoder.inflate(input,inputSize,output,outputSize);
}
inline bool isPackedBook(File& file){return String(file.path()).endsWith(".pbt");}
inline size_t bookLogicalSize(File& file){
  if(!isPackedBook(file))return file.size();
  const auto old=file.position();PalaPacked::Header header;
  const bool valid=PalaPacked::inspect(file,header);file.seek(old);return valid?header.size:0;
}
class FileReadStream:public IReadStream {
public:
  explicit FileReadStream(File& file):plain_(file){if(isPackedBook(file))packed_.reset(new(std::nothrow) PalaPacked::Stream<File>(file,inflateBookBlock));packedFile_=isPackedBook(file);}
  int read() override{return packedFile_?(packed_?packed_->read():-1):plain_.read();}
  bool seek(uint32_t pos) override{return packedFile_?(packed_&&packed_->seek(pos)):plain_.seek(pos);}
  uint32_t position() override{return packedFile_?(packed_?packed_->position():0):plain_.position();}
  size_t size() override{return packedFile_?(packed_?packed_->size():0):plain_.size();}
  bool available() override{return packedFile_?(packed_&&packed_->available()):plain_.available();}
private:
  BufferedReadStream<File> plain_;bool packedFile_=false;
  std::unique_ptr<PalaPacked::Stream<File>> packed_;
};
inline bool validatePackedBook(File& file){
  FileReadStream stream(file);if(!stream.size())return false;
  const size_t expected=stream.size();size_t count=0;while(stream.read()>=0)++count;
  return count==expected;
}
#endif
