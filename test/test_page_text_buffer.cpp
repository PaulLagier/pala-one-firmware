#include "test_framework.h"
#include "pure/page_text_buffer.h"
#include <string>
TEST_CASE("page prefetch text buffer preserves UTF-8 and empty lines") {
  PageTextBuffer b;b.append("café",5);b.append("",0);
  CHECK(b.valid);CHECK_EQ(b.count,2u);CHECK_EQ(std::string(b.line(0)),std::string("café"));
  CHECK_EQ(std::string(b.line(1)),std::string(""));
}
TEST_CASE("page prefetch rejects excess bytes without partial lines") {
  PageTextBuffer b;std::string line(PageTextBuffer::capacity-1,'a');
  b.append(line.c_str(),line.size());CHECK(b.valid);CHECK_EQ(b.used,PageTextBuffer::capacity);
  b.append("x",1);CHECK(!b.valid);CHECK_EQ(b.count,1u);
  b.clear();CHECK(b.valid);CHECK_EQ(b.count,0u);
}
TEST_CASE("page prefetch rejects excess lines") {
  PageTextBuffer b;for(size_t i=0;i<PageTextBuffer::maxLines;++i)b.append("",0);
  CHECK(b.valid);b.append("",0);CHECK(!b.valid);CHECK_EQ(b.count,PageTextBuffer::maxLines);
}
