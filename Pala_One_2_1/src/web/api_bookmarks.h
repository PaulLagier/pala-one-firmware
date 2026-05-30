#ifndef PALA_WEB_API_BOOKMARKS_H
#define PALA_WEB_API_BOOKMARKS_H

// Mounts:
//   GET  /api/bookmarks                          -> all books + their bookmarks (+ label snippets)
//   GET  /api/bookmarks/view?book=N&idx=M        -> single bookmark with page text
//   POST /api/bookmarks/delete                   -> body { "book": N, "idx": M }, returns { "ok": true }
//   GET  /api/bookmarks/export?book=N            -> text/plain attachment download
void registerApiBookmarksRoutes();

#endif  // PALA_WEB_API_BOOKMARKS_H
