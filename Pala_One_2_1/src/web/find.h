#ifndef PALA_WEB_FIND_H
#define PALA_WEB_FIND_H

// Mounts the in-browser book reader + find/jump routes:
//
//   GET  /read           ?id=N — rendered book view with search + jump UI
//   GET  /readbook-text  ?id=N — raw normalized book text, for client-side search
//   POST /jumpoffset     id=N, offset=BYTES — set the position to a byte offset
//
// The browser does all the searching client-side over /readbook-text; the
// device only persists the final jump position. Page-number jumps already
// live in /jumppage (src/web/files.cpp).
void registerFindRoutes();

#endif  // PALA_WEB_FIND_H
