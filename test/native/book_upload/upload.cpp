#include <fstream>
#include <iterator>
#include "test_framework.h"
#include "src/state.h"
#include "src/storage/library.h"
#include "src/web/upload.h"
#include "src/pure/paths.h"
#include "src/hal/packed_file_stream.h"
MemoryFS FS;
TestServer server;
TestCatalog g_library;
unsigned cacheDeletes=0;
namespace {
std::vector<uint8_t> fixture() {
  std::ifstream input(std::string(PACKED_FIXTURES)+"/book.pbt",std::ios::binary);
  return {std::istreambuf_iterator<char>(input),{}};
}
void reset() {
  resetBookUpload(); FS=MemoryFS{}; server=TestServer{};
  g_library.bookCount=0; cacheDeletes=0; registerUploadRoutes();
}
void event(UploadStatus status) {
  server.incoming.status=status; server.routes.at("/upload").second();
}
void upload(const std::vector<uint8_t>& bytes,const char* name="book.pbt",size_t chunk=17) {
  server.incoming.filename=name; event(UPLOAD_FILE_START);
  for(size_t at=0;at<bytes.size();at+=chunk) {
    server.incoming.buf=const_cast<uint8_t*>(bytes.data()+at);
    server.incoming.currentSize=(std::min)(chunk,bytes.size()-at);
    event(UPLOAD_FILE_WRITE);
  }
  server.incoming.totalSize=bytes.size(); event(UPLOAD_FILE_END);
  server.routes.at("/upload").first();
}
void oldBook() { auto f=FS.open("/books/book.pbt","w"); f.print("old book"); }
bool oldRetained() {
  auto f=FS.open("/books/book.pbt","r");
  return f.bytes && std::string(f.bytes->begin(),f.bytes->end())=="old book";
}
}
TEST_CASE("Web packed upload retains exact bytes across binary chunk boundaries") {
  for(size_t chunk : {1u,17u,512u,8192u}) {
    reset(); auto bytes=fixture(); REQUIRE(!bytes.empty());
    upload(bytes,"book.pbt",chunk);
    CHECK_EQ(server.status,200); CHECK_EQ(cacheDeletes,1u);
    REQUIRE(FS.exists("/books/book.pbt"));
    CHECK(*FS.files.at("/books/book.pbt")==bytes);
    CHECK(!FS.exists("/books/book.pbt.tmp"));
    auto f=FS.open("/books/book.pbt","r");
    REQUIRE(validatePackedBook(f));
  }
}
TEST_CASE("Web packed upload rejects corrupt containers without replacing existing books") {
  auto bytes=fixture(); REQUIRE(!bytes.empty());
  for(size_t at : {0u,4u,16u,unsigned(bytes.size()-1)}) {
    reset(); oldBook(); auto bad=bytes; bad[at]^=0x40;
    upload(bad,"new-book.pbt"); CHECK_EQ(server.status,400); CHECK(oldRetained());
    CHECK_EQ(cacheDeletes,0u); CHECK(!FS.exists("/books/book.pbt.tmp"));
  }
  reset(); oldBook(); upload({},"new-book.pbt"); CHECK_EQ(server.status,400); CHECK(oldRetained());
}
TEST_CASE("Web packed upload preserves old book on failed writes rename and low space") {
  for(int failure=0;failure<3;++failure) {
    reset(); oldBook();
    FS.shortWrite=failure==0; FS.failRename=failure==1;
    if(failure==2) FS.freeBytes=1;
    upload(fixture(),"new-book.pbt"); CHECK_EQ(server.status,400); CHECK(oldRetained());
    CHECK_EQ(cacheDeletes,0u); CHECK(!FS.exists("/books/book.pbt.tmp"));
  }
}
TEST_CASE("Web packed upload abort and session reset remove staging only") {
  for(bool abort : {false,true}) {
    reset(); oldBook(); server.incoming.filename="new-book.pbt";
    event(UPLOAD_FILE_START); REQUIRE(FS.exists("/books/new-book.pbt.tmp"));
    if(abort) event(UPLOAD_FILE_ABORTED); else resetBookUpload();
    CHECK(!FS.exists("/books/book.pbt.tmp")); CHECK(oldRetained());
    CHECK_EQ(cacheDeletes,0u);
  }
}
TEST_CASE("Book adapter maintains decoded size position and plain buffering") {
  reset(); auto bytes=fixture();
  auto f=FS.open("/books/book.pbt","w"); f.write(bytes.data(),bytes.size());
  f=FS.open("/books/book.pbt","r"); REQUIRE(f.seek(3));
  auto logical=bookLogicalSize(f); CHECK(logical>bytes.size()); CHECK_EQ(f.position(),3u);
  BookReadStream packed(f); REQUIRE(packed.valid()); CHECK_EQ(packed.size(),logical);
  REQUIRE(packed.seek(4095)); CHECK_EQ(packed.read(),0xc3); CHECK_EQ(packed.read(),0xa9);
  REQUIRE(packed.seek(logical)); CHECK_EQ(packed.read(),-1);
  f=FS.open("/books/plain.txt","w"); f.print("plain text");
  f=FS.open("/books/plain.txt","r"); BookReadStream plain(f);
  REQUIRE(plain.valid()); CHECK_EQ(plain.size(),10u); CHECK_EQ(plain.read(),int('p'));
  REQUIRE(plain.seek(6)); CHECK_EQ(plain.read(),int('t'));
}

TEST_CASE("Browser uploads reject exact sanitized and truncated name collisions without changing books") {
  for(bool packed : {false,true}) {
    const auto bytes=packed?fixture():std::vector<uint8_t>{'f','i','r','s','t'};
    const std::string extension=packed?".pbt":".txt";
    for(bool longName : {false,true}) {
      reset();
      const std::string prefix=longName?std::string(110,'a'):"same";
      const std::string first=prefix+(longName?"-first":"?")+extension;
      const std::string second=prefix+(longName?"-second":":")+extension;
      REQUIRE(sanitizeUploadedFilename(first.c_str())==sanitizeUploadedFilename(second.c_str()));
      upload(bytes,first.c_str()); REQUIRE(server.status==200);
      const std::string path="/books/"+std::string(sanitizeUploadedFilename(first.c_str()).c_str());
      const auto original=*FS.files.at(path); const auto deleted=cacheDeletes;
      for(const auto& name : {first,second}) {
        upload(bytes,name.c_str()); CHECK_EQ(server.status,409);
        CHECK(*FS.files.at(path)==original); CHECK_EQ(cacheDeletes,deleted);
        CHECK_EQ(FS.files.size(),1u);
      }
    }
  }
}
TEST_CASE("Browser upload preserves a destination created after streaming starts") {
  reset(); server.incoming.filename="racing.txt";event(UPLOAD_FILE_START);
  uint8_t content[]={'n','e','w'};server.incoming.buf=content;server.incoming.currentSize=3;
  event(UPLOAD_FILE_WRITE);
  auto old=FS.open("/books/racing.txt","w");old.print("keep this");old.close();
  server.incoming.totalSize=3;event(UPLOAD_FILE_END);server.routes.at("/upload").first();
  CHECK_EQ(server.status,409);CHECK_EQ(cacheDeletes,0u);
  CHECK_EQ(FS.files.size(),1u);
  const auto& kept=*FS.files.at("/books/racing.txt");
  CHECK(std::string(kept.begin(),kept.end())=="keep this");
}
