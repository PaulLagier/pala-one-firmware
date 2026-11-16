#include "src/ui/reader_index_feedback.h"
#include "src/config.h"
#include "src/hal/input.h"
#include "src/hal/display.h"
#include "src/storage/statistics.h"
#include "src/ui/font.h"
#include "src/ui/lock.h"
#include "src/ui/reader.h"
#include "src/ui/widgets.h"

#if PALA_DIAGNOSTICS && defined(PALA_COMPANION_TEST)
static bool s_testCancel = false;
void readerIndexTestCancel(bool armed) { s_testCancel = armed; }
#endif

ReaderIndexFeedback::ReaderIndexFeedback() : started_(millis()), enabled_(!Lock::isLocked()) {}
ReaderIndexFeedback::~ReaderIndexFeedback() {
  if (!visible_) return;
  // Consume the operation's input, including unfinished click groups, so a
  // cancel press cannot turn a page or select a book after the transition.
  g_btns.poll();
  Statistics::bumpButtons(g_btns.peekPressCount());
  resetInputFrontend();
  markUserActivity();
  forceNextMenuFrameFull();
  forceNextRenderFull();
}
bool ReaderIndexFeedback::step(uint32_t current, uint32_t target) {
  if (!enabled_) return true; // retain the locked sleep image and wake gesture
  const uint32_t now = millis();
  if (!visible_ && uint32_t(now - started_) < 500) return true;
  if (!visible_) {
    // The gesture which opened this operation has already been dispatched.
    // Do not reinterpret its release or old classifier flags as cancellation.
    resetInputFrontend();
    visible_ = true;
    forceNextMenuFrameFull();
    markUserActivity();
  }
  g_btns.poll();
  maybeRecoverFromIsrOverflow();
  if (g_btns.anyClick()) return false;
  const int percent = target ? int(min(uint64_t(100), uint64_t(current) * 100 / target)) : 0;
  if (percent_ < 0 || (uint32_t(now - lastFrame_) >= 1500 && percent >= percent_ + 5)) {
    prepareMenuFrame();
    drawPalaList(D_READER_INDEXING, 2, 1, [percent](int row) {
      return row ? String(D_READER_INDEX_CANCEL) : String(percent) + "%";
    });
    display.update();
    Font::useBody(); // feedback typography must never affect pagination
    percent_ = percent;
    lastFrame_ = now;
  }
#if PALA_DIAGNOSTICS && defined(PALA_COMPANION_TEST)
  if (s_testCancel) { s_testCancel = false; return false; }
#endif
  return true;
}
