#pragma once
#include <stdint.h>

// Tracks the existing Improv framing without inspecting or storing payloads.
// Diagnostic command markers inside a binary payload belong to Improv.
class ImprovFrameGuard {
public:
  bool active() const { return position_ != 0; }
  void reset(){position_=0;remaining_=0;}
  bool expire(uint32_t now){
    if(!active() || uint32_t(now-lastByteMs_)<=1000)return false;
    reset();return true;
  }
  void observe(uint8_t byte,uint32_t now=0) {
    lastByteMs_=now;
    static constexpr char prefix[] = "IMPROV";
    if (position_ < 6) {
      position_ = byte == (uint8_t)prefix[position_] ? position_ + 1 : 0;
    } else if (position_ == 6) {
      position_ = byte == 1 ? 7 : 0;
    } else if (position_ == 7) {
      position_ = 8;
    } else if (position_ == 8) {
      remaining_ = (uint16_t)byte + 1; // payload plus checksum
      position_ = 9;
    } else if (--remaining_ == 0) {
      position_ = 0;
    }
  }
private:
  uint8_t position_ = 0;
  uint16_t remaining_ = 0;
  uint32_t lastByteMs_ = 0;
};
