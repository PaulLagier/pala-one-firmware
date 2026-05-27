#ifndef PALA_WEB_API_SCREENSAVERS_H
#define PALA_WEB_API_SCREENSAVERS_H

// Mounts the SPA-side JSON endpoints for the screensaver editor:
//
//   GET  /api/screensavers              -> { mode, populated, max, hasSingle,
//                                            firstFree, slots: [{id, exists}] }
//   POST /api/screensavers/mode         body { "mode": "single"|"cycle"|"shuffle" }
//   POST /api/screensavers/delete       body { "single": true } or { "slot": N }
//
// The binary endpoints stay where they are -- the SPA points <img src=...>
// and <a href=...> at them directly, no JSON involved:
//   GET  /screensavers/thumb            (?single=1 | ?slot=N)   image/bmp
//   GET  /screensavers/download         (?single=1 | ?slot=N)   octet-stream
//   POST /screensavers/upload           multipart, ?single=1 | ?slot=N | auto
//
// Legacy GET /screensavers (the full HTML page) keeps mounting through
// registerScreensaverRoutes() during the strangler migration; delete on
// Phase 4 cutover. The five non-HTML routes above survive cutover.
void registerApiScreensaversRoutes();

#endif  // PALA_WEB_API_SCREENSAVERS_H
