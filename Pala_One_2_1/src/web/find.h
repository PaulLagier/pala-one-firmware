#ifndef PALA_WEB_FIND_H
#define PALA_WEB_FIND_H

// Mounts the in-browser book reader's data endpoints:
//
//   GET  /readbook-text  ?id=N — raw normalized book text, streamed.
//   POST /jumpoffset     id=N, offset=BYTES — set the resume position to a
//                                              byte offset; returns {"ok":true}.
//
// The SPA owns the find UI client-side over /readbook-text; the device only
// persists the chosen byte offset. Page-number jumps live separately at
// /api/books/jumppage (src/web/api_files.cpp).
void registerFindRoutes();

#endif  // PALA_WEB_FIND_H
