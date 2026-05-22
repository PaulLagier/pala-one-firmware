#ifndef PALA_UI_READER_H
#define PALA_UI_READER_H

#include "src/config.h"
#include "src/state.h"  // File, FS macro
#include "src/pure/page_offset_table.h"

// ============================================================================
//  The currently open book and where the user is looking at it. Shared
//  between `ReaderScreen` (active reading session) and
//  `BookmarkPreviewScreen` (transient view before commit). Both screens
//  legitimately operate on a `BookView`; only the reader does anything
//  beyond view (auto-save throttle, idle prefetch, wake arming) — those
//  live private inside `reader.cpp`.
//
//  Strong invariant: a book is "open" iff `book.isOpen()` returns true,
//  and that's true iff `book.path()` is non-empty. The file handle, path,
//  and derived NVS key always move as a unit through `OpenBook`'s methods.
//
//  Library / upload / web flows fully clear `g_bookview` on leaving the
//  reader (see `LibraryScreen::onEnter`), so non-reader code never has to
//  reason about stale book state.
// ============================================================================

// File handle + path + derived NVS key, managed together. The class enforces
// the invariant "file is open iff path is non-empty" — public callers can't
// observe the file-closed-with-path-still-set intermediate state.
class OpenBook {
public:
  // Open `path` for reading. Sets path + key + opens file as one atomic
  // step. Returns false (and leaves the object closed/empty) if the file
  // can't be opened or is a directory.
  bool open(const String& path);

  // Close the file and clear path + key. No-op if already closed.
  void close();

  bool          isOpen() const { return (bool)file_; }
  const String& path() const   { return path_; }
  const String& key() const    { return key_; }
  File&         file()         { return file_; }
  size_t        size()         { return file_ ? file_.size() : 0; }

private:
  File   file_;
  String path_;
  String key_;
};

// UI cursor state: where the user is looking + how long since a full refresh.
struct ReaderCursor {
  int pageIndex = 0;
  int pageTurnsSinceFull = 0;
};

struct BookView {
  OpenBook        book;
  PageOffsetTable pages;
  ReaderCursor    cursor;
};

extern BookView g_bookview;

bool openBookByIndex(int idx);
void renderCurrentPage();
void forceNextRenderFull();  // one-shot: next renderCurrentPage() does a full refresh

// Cursor navigation. Both move the cursor and bump `pageTurnsSinceFull` on
// a real page change; `advancePage` lazily paginates one ahead and clamps
// to EOF. Return true iff the cursor actually moved — caller decides what
// to do on a successful move (reader saves progress + renders; preview
// just renders).
bool advancePage();
bool retreatPage();

// Find which page of the active book contains `targetOffset` — the largest
// N where `g_bookview.pages.offsets[N] <= targetOffset`. Paginates the
// active book forward as needed (so the page table grows up to the answer).
// Used to resume from a saved byte offset and to navigate to a bookmark
// whose offset is invariant under font/layout changes.
int findPageForOffset(uint32_t targetOffset);

// Boot-time wake-resume. Checks if wake state asks us to resume reading and
// if the book still exists. On success the reader is fully set up for an
// immediate renderCurrentPage(); the caller owns the actual render and the
// screen transition.
bool tryRestoreReadingSession();

// Wake state — a single NVS key (`wake_path`) that survives deep sleep and
// tells `setup()` whether to resume the reader on next boot. Owned by the
// reader: armed by `ReaderScreen::onSleep` (via `armResumeOnWake`),
// consumed and cleared at boot by `tryRestoreReadingSession`. The clear
// side is private to reader.cpp; nothing else needs to disarm.
void armResumeOnWake();    // arm: persist currently-open book for resume

// Layout changed mid-session — the byte offsets in `g_bookview.pages` were
// computed under the old layout and no longer map to page boundaries under
// the new one. Resets the in-memory table, reloads the on-disk cache (which
// is layout-fingerprinted and will be rejected if the layout actually
// differs), and re-finds the current page from the saved byte offset.
// No-op if no book is open. Caller is responsible for the redraw.
//
// Called from `Statusbar::setMode`, which is the only path that can change
// layout while the reader screen is active. Font size / line gap go through
// the web /settings form — the user can't be in the reader at that moment,
// and the next `openBookByIndex` rebuilds the table from scratch anyway.
void repaginateForLayoutChange();

// ============================================================================
//  View lifecycle — full reset of every piece of `g_bookview` (and the reader's
//  private save-throttle state). Called when leaving the reader entirely
//  (library entry, factory reset).
// ============================================================================
void resetBookView();

// ============================================================================
//  Progress + bookmark glue — manipulates `g_bookview`, persists via storage.
//  Lives on the ui side of the boundary because the bulk of the logic is
//  view-state reasoning ("is a book even open right now"); the underlying
//  save is delegated to the pure storage API. Auto-save throttle bookkeeping
//  is private to `reader.cpp` — preview never auto-saves, only the reader
//  does, so there's no caller outside this module that needs to see it.
// ============================================================================

// Persist the current reading position to NVS unconditionally.
void saveProgress();

// Persist the current reading position to NVS only if it has actually
// changed since the last save AND the throttle window has elapsed. Used
// on every page turn in the reader to bound NVS write rate.
void saveProgressThrottled();

// Commit progress + page table to durable storage. Call at every "leaving"
// moment (sleep, exit-to-home, preview commit) so a later boot or open
// finds both the latest cursor and a fresh on-disk page cache. No-op if
// no book is open.
void persistReaderState();

// Add a bookmark at the currently-open view's page. Returns a UI message
// (e.g. "Bookmark saved" / "Bookmark exists") or nullptr if no book is open.
const char* addBookmarkForCurrentBook();

#endif  // PALA_UI_READER_H
