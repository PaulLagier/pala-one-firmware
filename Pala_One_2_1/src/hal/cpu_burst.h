#pragma once
#include <Arduino.h>
// Restore the caller's frequency even on error paths and nested calls.
class CpuBurst {
public:
  CpuBurst() : previous_(getCpuFrequencyMhz()) {
    if (previous_ != 240) setCpuFrequencyMhz(240);
  }
  ~CpuBurst() { restore(); }
  void restore() {
    if (!active_) return;
    if (getCpuFrequencyMhz() != previous_) setCpuFrequencyMhz(previous_);
    active_ = false;
  }
private:
  uint32_t previous_;
  bool active_ = true;
};
