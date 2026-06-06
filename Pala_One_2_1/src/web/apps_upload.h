#ifndef PALA_WEB_APPS_UPLOAD_H
#define PALA_WEB_APPS_UPLOAD_H

// Mounts app routes:
//   POST /upload-app   — multipart streaming receiver for Pala app .bin
//                        binaries. Files land in /apps/. Validates the
//                        PalaAppHeader magic before committing so a bad
//                        upload is rejected at install time, not launch.
//   GET  /download-app — stream an installed .bin as attachment (?name=foo.bin)
//   POST /del-app      — delete an installed app (?name=foo.bin)
//
// Call once from registerWebRoutes().
void registerAppUploadRoutes();

// Close any open tmp file and clear all per-session fields. Called by the
// upload screen at session start/stop. Storage lives file-static inside
// apps_upload.cpp.
void resetAppUpload();

#endif  // PALA_WEB_APPS_UPLOAD_H
