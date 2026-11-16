#pragma once
#include <Arduino.h>
// Cooperative foreground feedback; no reentrant screen dispatch. Fast cached
// lookups leave input and display untouched. Only a visible operation owns input.
class ReaderIndexFeedback {
public:
  explicit ReaderIndexFeedback(bool enabled = true);
  ~ReaderIndexFeedback();
  bool step(uint32_t current, uint32_t target);

private:
  uint32_t started_, lastFrame_ = 0;
  int percent_ = -1;
  bool visible_ = false;
  bool enabled_;
};

#if PALA_DIAGNOSTICS && defined(PALA_COMPANION_TEST)
// Private device-test hook: cancel only after the real progress frame is visible.
void readerIndexTestCancel(bool armed);
#endif
