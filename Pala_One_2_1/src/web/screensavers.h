#ifndef PALA_WEB_SCREENSAVERS_H
#define PALA_WEB_SCREENSAVERS_H

// Mounts the in-firmware screensaver editor + slot manager:
//
//   GET  /screensavers              editor page + slot grid + mode picker
//   POST /screensavers/upload       multipart upload, query `slot=N` or `single=1`
//   GET  /screensavers/thumb        ?slot=N or ?single=1 — returns a BMP
//   POST /screensavers/delete       slot=N or single=1 (removes legacy /sleep.bin)
//   POST /screensavers/mode         mode=single|cycle|shuffle
//
// All upload routes register a stream handler + a final response handler;
// per-session state lives file-static inside the .cpp.
void registerScreensaverRoutes();

// Close any open tmp file and clear all per-session fields. Called by the
// upload screen at session start/stop so a stale field from a prior session
// can't leak through.
void resetScreensaverUpload();

#endif  // PALA_WEB_SCREENSAVERS_H
