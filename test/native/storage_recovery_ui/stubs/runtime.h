#pragma once
#include <vector>
#include <string>
#include <functional>
#include "src/config.h"
#include "src/hal/input.h"

struct RenderedLine { int x, y; std::string text; };
inline std::vector<RenderedLine> rendered;
inline unsigned resets = 0, updates = 0;
inline int selectedBaseline = -1, selectionColor = -1;
struct MockGraphics {
  void drawRect(int, int y, int, int, int color) {
    selectedBaseline = y + 11; selectionColor = color;
  }
};
struct MockFont {
  int x = 0, y = 0;
  void setCursor(int a, int b) { x = a; y = b; }
  void print(const char* text) { rendered.push_back({x,y,text}); }
};
struct MockDisplay { void update() { ++updates; } };
inline MockGraphics gfx;
inline MockFont u8g2;
inline MockDisplay display;
inline ButtonState g_btns;
inline std::vector<ButtonEvent::Kind> bootEvents;
inline size_t bootEventIndex = 0;
inline unsigned bootPolls = 0;
inline void ButtonState::poll() {
  resetClicks();
  if (++bootPolls % 4 || bootEventIndex >= bootEvents.size()) return;
  switch (bootEvents[bootEventIndex++]) {
    case ButtonEvent::Short: shortClick_ = true; break;
    case ButtonEvent::Double: doubleClick_ = true; break;
    case ButtonEvent::Long: longClick_ = true; break;
    default: break;
  }
}
inline void resetInputFrontend() { ++resets; g_btns.resetState(); }
inline bool maybeRecoverFromIsrOverflow() { return false; }
namespace Font { inline void useToast() {} inline void loadSettings() {} }
namespace ClickTimings { inline void loadSettings() {} inline uint32_t debounceMs() { return 25; } }
namespace Lock {
inline bool locked = false;
inline bool isLocked() { return locked; }
inline void disengage() { locked = false; }
inline bool isUnlockGesture(const ButtonEvent& e) {
  return e.kind == ButtonEvent::Long || e.kind == ButtonEvent::VeryLong || e.kind == ButtonEvent::ClickHold;
}
}
inline void prepareMenuFrame() { rendered.clear(); selectedBaseline = selectionColor = -1; }
inline void forceNextMenuFrameFull() {}
inline int drawSectionHeader(const char*, bool, bool) { return 35; }
inline void drawCenter(const char*, const char*) { ++updates; }
