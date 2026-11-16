#pragma once
// Host adapter only: validate the production HAL's bounded decoder call
// against zlib. The actual SDK miniz implementation is compiled by PIO.
#include <zlib.h>
using tinfl_decompressor = int;
constexpr int TINFL_FLAG_PARSE_ZLIB_HEADER=1;
constexpr int TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF=2;
constexpr int TINFL_STATUS_DONE=0;
inline void tinfl_init(tinfl_decompressor*) {}
inline int tinfl_decompress(tinfl_decompressor*,const uint8_t* input,size_t* consumed,
                           uint8_t*,uint8_t* output,size_t* produced,int) {
  z_stream state{}; if(inflateInit(&state)!=Z_OK) return -1;
  state.next_in=const_cast<Bytef*>(input); state.avail_in=uInt(*consumed);
  state.next_out=output; state.avail_out=uInt(*produced);
  auto status=inflate(&state,Z_FINISH);
  *consumed=state.total_in; *produced=state.total_out;
  inflateEnd(&state); return status==Z_STREAM_END?TINFL_STATUS_DONE:-1;
}
