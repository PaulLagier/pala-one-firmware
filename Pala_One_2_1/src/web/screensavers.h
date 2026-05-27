#ifndef PALA_WEB_SCREENSAVERS_H
#define PALA_WEB_SCREENSAVERS_H

// Mounts the binary screensaver endpoints used by the SPA editor:
//
//   POST /screensavers/upload   multipart upload, query `slot=N` or `single=1`
//   GET  /screensavers/thumb    ?slot=N or ?single=1 — 250x122 BMP
//   GET  /screensavers/download ?slot=N or ?single=1 — raw .bin attachment
//
// State changes (mode picker + delete) live in src/web/api_screensavers.cpp
// as JSON endpoints (/api/screensavers, /api/screensavers/{mode,delete}).
void registerScreensaverRoutes();

#endif  // PALA_WEB_SCREENSAVERS_H
