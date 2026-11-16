#include "test_framework.h"
#include "pure/page_offset_table.h"
TEST_CASE("page table never exposes EOF as another page") {
  PageOffsetTable t; t.count=1; t.offsets[0]=0;
  CHECK(t.append(5, 13));
  CHECK(!t.append(13, 13));
  CHECK_EQ(t.count, 2);
  CHECK(t.eofReached);
  CHECK(!t.append(12,13));
}
TEST_CASE("page table rejects non-progress and beyond EOF") {
  PageOffsetTable t; t.count=1; t.offsets[0]=0;
  CHECK(!t.append(0,13));
  CHECK_EQ(t.count,1);
  t.eofReached=false;
  CHECK(!t.append(14,13));
  CHECK_EQ(t.count,1);
}

TEST_CASE("page table rolls without losing absolute positions or growing RAM") {
  PageOffsetTable table;table.reset();
  CHECK(sizeof(table)<=sizeof(uint32_t)*MAX_PAGES+32);
  for(int page=1;page<80000;++page){
    CHECK(table.append(uint32_t(page*5),500000));
    CHECK(table.count<=MAX_PAGES);CHECK_EQ(table.endPage(),page+1);
    CHECK(table.contains(page));CHECK_EQ(table.offsetAt(page),uint32_t(page*5));
    CHECK(table.contains(page-1));CHECK_EQ(table.offsetAt(page-1),uint32_t((page-1)*5));
  }
  CHECK(table.firstPage>0);CHECK(!table.contains(0));
  CHECK(!table.append(500000,500000));CHECK(table.eofReached);
  table.reset();CHECK_EQ(table.firstPage,0);CHECK_EQ(table.count,1);CHECK_EQ(table.offsetAt(0),0U);CHECK(!table.eofReached);
}
TEST_CASE("EOF at a full window does not roll away valid pages") {
  PageOffsetTable table;table.reset();
  for(int page=1;page<MAX_PAGES;++page)CHECK(table.append(page,MAX_PAGES));
  CHECK(!table.append(MAX_PAGES,MAX_PAGES));CHECK(table.eofReached);
  CHECK_EQ(table.firstPage,0);CHECK_EQ(table.count,MAX_PAGES);
}
