#pragma once
// Adapt only the platform's inflation primitive; use the production book
// stream, CRC, metadata, buffering and reader unchanged.
#include <zlib.h>
struct tinfl_decompressor {};
inline void tinfl_init(tinfl_decompressor*){}
constexpr int TINFL_FLAG_PARSE_ZLIB_HEADER=1,TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF=2,TINFL_STATUS_DONE=0;
inline int tinfl_decompress(tinfl_decompressor*,const uint8_t* input,size_t* inputSize,uint8_t*,uint8_t* output,size_t* outputSize,int){
 z_stream stream{};if(inflateInit(&stream)!=Z_OK)return -1;
 stream.next_in=const_cast<Bytef*>(input);stream.avail_in=uInt(*inputSize);stream.next_out=output;stream.avail_out=uInt(*outputSize);
 const int status=inflate(&stream,Z_FINISH);*inputSize=stream.total_in;*outputSize=stream.total_out;inflateEnd(&stream);return status==Z_STREAM_END?TINFL_STATUS_DONE:-1;
}
