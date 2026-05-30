#ifndef PALA_WEB_API_SCREENSAVERS_H
#define PALA_WEB_API_SCREENSAVERS_H

// Mounts the SPA-side JSON endpoints for the screensaver editor:
//
//   GET  /api/screensavers              -> { mode, populated, max, hasSingle,
//                                            firstFree, slots: [{id, exists}] }
//   POST /api/screensavers/mode         body { "mode": "single"|"cycle"|"shuffle" }
//   POST /api/screensavers/delete       body { "single": true } or { "slot": N }
//
// Companion binary endpoints (mounted by registerScreensaverRoutes() in
// screensavers.cpp) -- the SPA points <img src=...>, <a href=...>, and
// fetch(FormData) at them directly:
//   GET  /screensavers/thumb            (?single=1 | ?slot=N)   image/bmp
//   GET  /screensavers/download         (?single=1 | ?slot=N)   octet-stream
//   POST /screensavers/upload           multipart, ?single=1 | ?slot=N | auto
void registerApiScreensaversRoutes();

#endif  // PALA_WEB_API_SCREENSAVERS_H
