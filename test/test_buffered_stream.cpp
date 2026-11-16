#include "test_framework.h"
#include "pure/buffered_stream.h"
#include "pure/paginator.h"
#include <cstring>
#include <string>
struct CountingFile {
  std::string data; size_t pos=0; int reads=0; size_t shortLimit=1024;
  size_t size() { return data.size(); }
  size_t position() { return pos; }
  bool seek(uint32_t p) { if(p>data.size())return false;pos=p;return true; }
  size_t read(uint8_t* out,size_t n) {
    ++reads;n=(std::min)(n,(std::min)(shortLimit,data.size()-pos));
    memcpy(out,data.data()+pos,n);pos+=n;return n;
  }
};
TEST_CASE("buffered stream preserves bytes and logical position across block boundaries") {
  CountingFile f{std::string(4097,'a')}; BufferedReadStream<CountingFile> in(f);
  for(uint32_t i=0;i<4097;++i){ REQUIRE(in.available());CHECK_EQ(in.position(),i);CHECK_EQ(in.read(),(int)'a'); }
  CHECK(!in.available());CHECK_EQ(in.read(),-1);CHECK_EQ(f.reads,5);
  CHECK(in.seek(1023));CHECK_EQ(in.position(),1023u);CHECK_EQ(in.read(),(int)'a');
  CHECK(!in.seek(9999));CHECK_EQ(in.position(),1024u);
}
TEST_CASE("buffered stream handles short reads and exact EOF seeks") {
  CountingFile f{"abcdef"};f.shortLimit=2;BufferedReadStream<CountingFile> in(f);
  for(char c: f.data)CHECK_EQ(in.read(),(int)c);
  CHECK_EQ(f.reads,3);CHECK(in.seek(6));CHECK_EQ(in.read(),-1);
  CHECK(in.seek(0));CHECK_EQ(in.read(),(int)'a');
}
TEST_CASE("buffered pagination matches unbuffered page offsets") {
  std::string text;for(int i=0;i<200;++i)text+="hello café world abcdefghijklmnop\n";
  CountingFile f{text};BufferedReadStream<CountingFile> buffered(f);StringReadStream reference(text.c_str());
  LayoutMetrics m{};m.maxWidth=12;m.maxLines=4;uint32_t pos=0;
  auto measure=[](const char*s){return (int)strlen(s);};
  while(pos<text.size()){
    std::string a,b;
    auto next=paginatePage(buffered,pos,m,measure,[&](const char*s,size_t n){a.append(s,n);a+='\n';});
    auto expected=paginatePage(reference,pos,m,measure,[&](const char*s,size_t n){b.append(s,n);b+='\n';});
    REQUIRE(next>pos);CHECK_EQ(next,expected);CHECK(a==b);pos=next;
  }
}
TEST_CASE("buffered seeks reuse lookahead without losing the physical refill cursor") {
  std::string bytes;for(int i=0;i<5000;++i)bytes+=char('a'+i%26);
  CountingFile file{bytes};BufferedReadStream<CountingFile> in(file);
  CHECK_EQ(in.read(),int(bytes[0]));CHECK_EQ(file.reads,1);
  for(uint32_t p:{900u,0u,1023u,100u,1024u}){
    REQUIRE(in.seek(p));CHECK_EQ(in.position(),p);CHECK_EQ(file.reads,1);
  }
  CHECK_EQ(in.read(),int(bytes[1024]));CHECK_EQ(file.reads,2);
  REQUIRE(in.seek(1100));for(uint32_t p=1100;p<3100;++p)CHECK_EQ(in.read(),int(bytes[p]));
  CHECK(file.reads<=4);CHECK(!in.seek(5001));CHECK_EQ(in.position(),3100u);
}
TEST_CASE("many tiny pages keep a bounded number of underlying reads") {
  std::string text;for(int i=0;i<5000;++i)text+="a\n\n";
  CountingFile file{text};BufferedReadStream<CountingFile> stream(file);StringReadStream oracle(text.c_str());
  LayoutMetrics layout{};layout.maxWidth=10;layout.maxLines=2;
  auto measure=[](const char* s){return int(strlen(s));};uint32_t off=0;
  while(off<text.size()){
    auto next=paginatePage(stream,off,layout,measure,nullptr);
    REQUIRE(next>off);CHECK_EQ(next,paginatePage(oracle,off,layout,measure,nullptr));off=next;
  }
  CHECK(file.reads<=20);
}
