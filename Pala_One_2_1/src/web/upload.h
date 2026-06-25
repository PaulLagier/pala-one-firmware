#ifndef PALA_WEB_UPLOAD_H
#define PALA_WEB_UPLOAD_H

// Mounts /upload — book upload, multipart. Stream handler (chunk receiver)
// + done handler (JSON response). Consumed by the SPA `#/files` upload card.
void registerUploadRoutes();

// Close any open tmp file and clear all per-session fields. Called by the
// device-side upload screen at session start/stop so a stale field from a
// prior session can't leak through. Storage lives file-static in upload.cpp.
void resetBookUpload();

#endif  // PALA_WEB_UPLOAD_H
