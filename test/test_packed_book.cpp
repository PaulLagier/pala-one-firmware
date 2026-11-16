#include <functional>
#include <fstream>
#include <iterator>
#include <zlib.h>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include "test_framework.h"
#include "src/pure/packed_book.h"
#include "src/pure/paginator.h"

namespace {
struct Source {
  std::vector<uint8_t> bytes;
  size_t cursor = 0;
  bool failRead = false, failSeek = false;
  size_t size() const { return bytes.size(); }
  bool seek(uint32_t at) {
    if (failSeek || at > bytes.size()) return false;
    cursor = at; return true;
  }
  size_t read(uint8_t* out, size_t n) {
    if (failRead) return 0;
    n = (std::min)(n, bytes.size() - cursor);
    std::copy_n(bytes.data() + cursor, n, out); cursor += n; return n;
  }
};
void set32(std::vector<uint8_t>& bytes, size_t at, uint32_t word) {
  for (unsigned i = 0; i < 4; ++i) bytes[at+i] = uint8_t(word >> (8*i));
}
unsigned inflations = 0;
bool inflateBlock(const uint8_t* input, size_t inSize, uint8_t* output, size_t outSize) {
  ++inflations;
  z_stream state{};
  if (inflateInit(&state) != Z_OK) return false;
  state.next_in = const_cast<Bytef*>(input); state.avail_in = uInt(inSize);
  state.next_out = output; state.avail_out = uInt(outSize);
  const auto status = inflate(&state, Z_FINISH);
  const bool ok = status == Z_STREAM_END && state.total_in == inSize && state.total_out == outSize;
  inflateEnd(&state); return ok;
}
Source encode(const std::string& text, bool raw = false) {
  const size_t count = (text.size()+4095)/4096;
  Source source; auto& b = source.bytes; b.resize(16 + 4*(count+1));
  b[0]='P'; b[1]='B'; b[2]='T'; b[3]='1';
  set32(b,4,uint32_t(text.size())); set32(b,8,4096); set32(b,12,uint32_t(count));
  for (size_t i=0; i<count; ++i) {
    set32(b,16+4*i,uint32_t(b.size()));
    const auto n = (std::min)(size_t(4096), text.size()-i*4096);
    const auto* input = reinterpret_cast<const uint8_t*>(text.data()+i*4096);
    uLongf capacity = compressBound(n); std::vector<uint8_t> payload(capacity);
    if (compress2(payload.data(), &capacity, input, n, 9) != Z_OK)
      throw std::runtime_error("fixture compression failed");
    payload.resize(capacity); const bool uncompressed = raw || capacity >= n;
    if (uncompressed) payload.assign(input,input+n);
    const auto at=b.size(); b.resize(at+8);
    set32(b,at,uint32_t(payload.size()) | (uncompressed?0x80000000U:0));
    set32(b,at+4,uint32_t(::crc32(0,input,n)));
    b.insert(b.end(),payload.begin(),payload.end());
  }
  set32(b,16+4*count,uint32_t(b.size())); return source;
}
std::string decode(Source& source, bool* valid = nullptr) {
  PalaPacked::Stream<Source> stream(source,inflateBlock);
  std::string text; for (int byte; (byte=stream.read())>=0;) text += char(byte);
  if(valid) *valid=stream.valid(); return text;
}
}

TEST_CASE("PBT compressed and raw block boundaries retain exact logical bytes") {
  for (size_t length : {1u,4095u,4096u,4097u,8193u}) {
    std::string text; for(size_t i=0;i<length;++i) text += char(' '+i%90);
    for (bool raw : {false,true}) {
      auto source=encode(text,raw); bool valid=false;
      CHECK(decode(source,&valid)==text); CHECK(valid);
      PalaPacked::Stream<Source> stream(source,inflateBlock);
      CHECK_EQ(stream.size(),length);
      for (uint32_t at : {0u,uint32_t(length/2),uint32_t(length-1)}) {
        REQUIRE(stream.seek(at)); CHECK_EQ(stream.read(),int(uint8_t(text[at])));
      }
      REQUIRE(stream.seek(uint32_t(length))); CHECK_EQ(stream.read(),-1);
      CHECK(!stream.seek(uint32_t(length+1)));
    }
  }
}
TEST_CASE("PBT metadata rejects truncation size overflow and offset-table corruption") {
  const auto original=encode(std::string(8193,'a'),true);
  for(size_t n=0;n<original.bytes.size();++n) {
    auto source=original; source.bytes.resize(n); PalaPacked::Header header;
    CHECK(!PalaPacked::inspect(source,header)); CHECK_EQ(header.size,0u);
  }
  for (auto change : std::vector<std::pair<size_t,uint32_t>>{
      {0,0},{4,0},{4,2097153},{8,4095},{12,0},{12,0xffffffff},
      {16,0},{20,28},{20,0xffffffff},{28,0xffffffff}}) {
    auto source=original; set32(source.bytes,change.first,change.second);
    PalaPacked::Header header; CHECK(!PalaPacked::inspect(source,header));
  }
  auto trailing=original; trailing.bytes.push_back(0); PalaPacked::Header header;
  CHECK(!PalaPacked::inspect(trailing,header));
}
TEST_CASE("PBT CRC payload and changed source failures invalidate the stream") {
  auto original=encode(std::string(8192,'x'));
  const auto start=PalaPacked::read32(original.bytes.data()+16);
  for(size_t at=start;at<original.bytes.size();++at) {
    auto source=original; source.bytes[at]^=0x40; bool valid=true;
    decode(source,&valid); CHECK(!valid);
  }
  auto source=original; PalaPacked::Stream<Source> stream(source,inflateBlock);
  set32(source.bytes,16,0); CHECK_EQ(stream.read(),-1); CHECK(!stream.valid());
  source=original; PalaPacked::Stream<Source> noDecoder(source,nullptr);
  CHECK_EQ(noDecoder.read(),-1); CHECK(!noDecoder.valid());
}
TEST_CASE("PBT decoder rejects extra compressed input and incorrect decoded size") {
  auto source=encode(std::string(100,'a'));
  const auto start=PalaPacked::read32(source.bytes.data()+16);
  const auto length=PalaPacked::read32(source.bytes.data()+start);
  source.bytes.push_back(0); set32(source.bytes,start,length+1);
  set32(source.bytes,20,uint32_t(source.bytes.size())); bool valid=true;
  decode(source,&valid); CHECK(!valid);
  source=encode(std::string(100,'a')); set32(source.bytes,4,99);
  decode(source,&valid); CHECK(!valid);
  source=encode(std::string(100,'a')); set32(source.bytes,4,101);
  decode(source,&valid); CHECK(!valid);
}
TEST_CASE("PBT sequential pagination reuses blocks and matches plain UTF8 text") {
  std::string text(4095,'x');
  for(int i=0;i<700;++i) text += "é café Grüße words\n\n";
  auto source=encode(text); PalaPacked::Stream<Source> packed(source,inflateBlock);
  StringReadStream plain(text.c_str());
  LayoutMetrics layout{}; layout.maxWidth=23; layout.maxLines=5;
  auto measure=[](const char* s){return int(std::strlen(s));};
  uint32_t offset=0; inflations=0;
  while(offset<text.size()) {
    std::string actual,expected;
    auto next=paginatePage(packed,offset,layout,measure,[&](const char* s,size_t n){actual.append(s,n);actual+='\n';});
    CHECK_EQ(next,paginatePage(plain,offset,layout,measure,[&](const char* s,size_t n){expected.append(s,n);expected+='\n';}));
    CHECK(actual==expected); REQUIRE(next>offset); offset=next;
  }
  // One-block lookahead can cause a boundary to be decoded twice, but never per page.
  CHECK(inflations <= 3*((text.size()+4095)/4096));
}
TEST_CASE("PBT maximum logical size stays bounded and supports last-byte seeks") {
  auto source=encode(std::string(PalaPacked::MaximumSize,'z'),true);
  CHECK_EQ(source.bytes.size(),2103316u);
  PalaPacked::Stream<Source> stream(source,inflateBlock);
  REQUIRE(stream.valid()); CHECK_EQ(stream.size(),PalaPacked::MaximumSize);
  REQUIRE(stream.seek(PalaPacked::MaximumSize-1)); CHECK_EQ(stream.read(),int('z'));
  CHECK_EQ(stream.read(),-1);
  source.failRead=true; REQUIRE(stream.seek(0)); CHECK_EQ(stream.read(),-1); CHECK(!stream.valid());
}

TEST_CASE("PBT firmware stream decodes Python encoder fixtures without text changes") {
  std::ifstream textFile(std::string(PACKED_FIXTURES)+"/book.txt", std::ios::binary);
  REQUIRE(bool(textFile));
  const std::string expected{std::istreambuf_iterator<char>(textFile), {}};
  for (const char* name : {"book.pbt", "raw.pbt"}) {
    std::ifstream input(std::string(PACKED_FIXTURES)+"/"+name, std::ios::binary);
    REQUIRE(bool(input));
    Source source; source.bytes.assign(std::istreambuf_iterator<char>(input), {});
    bool valid=false; CHECK(decode(source,&valid)==expected); CHECK(valid);
  }
}
