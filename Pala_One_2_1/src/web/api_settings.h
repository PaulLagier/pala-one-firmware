#ifndef PALA_WEB_API_SETTINGS_H
#define PALA_WEB_API_SETTINGS_H

// Mounts:
//   GET  /api/settings                  -> { font, family, sleep, lgap, bionic,
//                                            noScreensaver, hasSleepImage }
//   POST /api/settings                  -> body same shape; applies and returns
//                                            the new state.
//   POST /api/sleep-image/delete        -> wipes /sleep.bin (no body), returns
//                                            { ok: true, hasSleepImage: false }.
//
// Legacy /settings (GET form + POST) and /del-sleep keep working until the
// Phase 4 cutover.
void registerApiSettingsRoutes();

#endif  // PALA_WEB_API_SETTINGS_H
