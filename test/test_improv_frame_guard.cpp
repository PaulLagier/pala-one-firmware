#include "test_framework.h"
#include "pure/improv_frame_guard.h"
TEST_CASE("Improv owns punctuation and newline bytes inside its payload") {
  ImprovFrameGuard guard;
  const uint8_t frame[]={'I','M','P','R','O','V',1,3,3,'!','\n',0xff,0};
  for(size_t i=0;i<sizeof(frame);++i){
    guard.observe(frame[i]);CHECK_EQ(guard.active(),i+1<sizeof(frame));
  }
}
TEST_CASE("Improv framing rejects a bad prefix and resumes at a fresh frame") {
  ImprovFrameGuard guard;guard.observe('I');CHECK(guard.active());
  guard.observe('!');CHECK(!guard.active());
  const uint8_t frame[]={'I','M','P','R','O','V',1,3,0,0};
  for(auto byte:frame)guard.observe(byte);
  CHECK(!guard.active());
}
TEST_CASE("Improv framing handles maximum byte-length payload without overflow") {
  ImprovFrameGuard guard;
  const uint8_t header[]={'I','M','P','R','O','V',1,3,255};
  for(auto byte:header)guard.observe(byte);
  for(int i=0;i<255;++i){guard.observe('!');CHECK(guard.active());}
  guard.observe(0);CHECK(!guard.active());
}

TEST_CASE("Abandoned Improv frames expire without owning the next diagnostic") {
  ImprovFrameGuard guard;guard.observe('I',100);
  CHECK(!guard.expire(1100));CHECK(guard.active());
  CHECK(guard.expire(1101));CHECK(!guard.active());CHECK(!guard.expire(5000));
  guard.observe('I',6000);guard.reset();CHECK(!guard.active());
}
TEST_CASE("Improv frame expiry handles clock wrap and active payload bytes") {
  ImprovFrameGuard guard;guard.observe('I',0xffffff00U);
  CHECK(!guard.expire(0x100U));CHECK(guard.active());
  CHECK(guard.expire(0x400U));CHECK(!guard.active());
  const uint8_t frame[]={'I','M','P','R','O','V',1,3,2,'!',0,0};
  uint32_t now=8000;
  for(size_t i=0;i<sizeof(frame);++i){guard.observe(frame[i],now);CHECK(!guard.expire(now+500));now+=500;}
  CHECK(!guard.active());
}
