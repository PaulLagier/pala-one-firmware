#ifndef PALA_UI_PALA_API_IMPL_H
#define PALA_UI_PALA_API_IMPL_H

// ============================================================================
//  PalaAPI shim — wires the v3 function-pointer table apps see at runtime
//  to the firmware's own modules. Field assignments live in one place
//  (`initPalaAPI`) so the frozen field order has exactly one source of truth.
//
//  See apps/include/pala_api.h for the public contract; docs/APPS_LAYER.md
//  §4.3 for why this lives in `ui/` rather than `hal/`.
// ============================================================================

// Populate the static PalaAPI table. Call once from setup() after fonts,
// input, and FS are up. Safe to call again after the table has been used
// (re-writes the same pointers).
void initPalaAPI();

// Load and run the app binary at `path` via the native loader. Handles
// error display (drawCenter + delay) before returning so the caller can
// simply repaint its own screen. Exposed so AppsScreen can wire its
// launch path against a stable name.
void runApp(const char* path);

#endif  // PALA_UI_PALA_API_IMPL_H
