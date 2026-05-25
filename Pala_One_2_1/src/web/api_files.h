#ifndef PALA_WEB_API_FILES_H
#define PALA_WEB_API_FILES_H

// Mounts the file-management JSON endpoints used by the SPA `#/files` screen:
//
//   GET  /api/files                     -> { storage, limits, books, folders, apps }
//   POST /api/books/delete              body { id }
//   POST /api/books/move                body { id, folder }
//   POST /api/books/jumppage            body { id, page }    (page is 1-based)
//   POST /api/folders/create            body { folder }
//   POST /api/folders/delete            body { folder }
//   POST /api/apps/delete               body { name }        (basename, no slashes)
//
// File *upload* still uses the legacy multipart /upload and /upload-app
// routes (see registerUploadRoutes / registerAppUploadRoutes) — porting
// those is its own task (Phase 3 / Uploads).
void registerApiFilesRoutes();

#endif  // PALA_WEB_API_FILES_H
