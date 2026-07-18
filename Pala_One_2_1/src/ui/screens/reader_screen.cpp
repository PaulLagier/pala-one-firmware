#include "src/ui/screens/reader_screen.h"

#include "src/hal/display.h"
#include "src/hal/input.h"
#include "src/ui/lock.h"                     // Lock::engage on ACTION_LOCK
#include "src/ui/reader.h"
#include "src/ui/reader_actions.h"           // Gestures::actionFor + ButtonAction
#include "src/ui/reader_menu.h"              // overlay state + dispatch
#include "src/ui/screens/library_screen.h"
#include "src/ui/sleep.h"                    // Sleep::enter on ACTION_LOCK
#include "src/ui/text.h"
#include "src/ui/toast.h"                    // Toast::show for bookmark-saved feedback
#include "src/config.h"
#include "src/ui/screen_settings.h"

// Carry out one of the bindable reader actions. No-op for ACTION_NONE.
// Lives at the screen layer because the actions are reader-context things
// (bookmark, lock, menu) — not generic input-layer concepts.
static void performReaderAction(ButtonAction action) {
  switch (action) {
    case ACTION_BOOKMARK: {
      const char* msg = addBookmarkForCurrentBook();
      if (msg) Toast::show(msg);
      g_bookview.cursor.pageTurnsSinceFull++;
      renderCurrentPage();
      break;
    }
    case ACTION_LOCK:
      // Persist lock state, then sleep immediately. On wake, the main loop
      // sees Lock::isLocked() and swallows input until an unlock gesture.
      // ReaderScreen::onSleep persists progress + arms resume, so the next
      // boot lands back on this page.
      Lock::engage();
      Sleep::enter();   // does not return
      break;
    case ACTION_MENU:
      ReaderMenu::open();
      break;
    case ACTION_ROTATE:
      ScreenSettings::toggleScreenRotation();
      break;
    case ACTION_NONE:
    default:
      break;
  }
}

void ReaderScreen::onEnter() {
  draw();
}

void ReaderScreen::draw() {
  renderCurrentPage();
}

void ReaderScreen::onButton(const ButtonEvent& e) {
  // Reader menu overlay swallows all input while open. On close, force a
  // full refresh because the overlay redraw replaced the page.
  if (ReaderMenu::isActive()) {
    if (ReaderMenu::onButton(e) && !ReaderMenu::isActive()) {
      g_bookview.cursor.pageTurnsSinceFull = FULL_REFRESH_EVERY_N_PAGES;
      renderCurrentPage();
    }
    return;
  }

  if (Gestures::resolveLegacyAction(e, ButtonEvent::Triple, ACTION_HOME)) {
    // Catch any progress / pagination since the last throttled save before
    // we lose the active book.
    persistReaderState();
    navigateToLibraryRoot();
    return;
  }

  if (Gestures::resolveLegacyAction(e, ButtonEvent::Double, ACTION_PREV)) {
    if (retreatPage()) {
      saveProgressThrottled();
      draw();
    }
    return;
  }

  if (Gestures::resolveLegacyAction(e, ButtonEvent::Short, ACTION_NEXT)) {
    if (advancePage()) {
      saveProgressThrottled();
      draw();
    }
    return;
  }

  performReaderAction(Gestures::actionFor(e.kind));

}

void ReaderScreen::onSleep() {
  // Strong invariant: reader screen is never active without an open book.
  persistReaderState();
  armResumeOnWake();        // captures path before resetBookView() drops it
  resetBookView();
}
