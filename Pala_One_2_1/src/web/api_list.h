#ifndef PALA_WEB_API_LIST_H
#define PALA_WEB_API_LIST_H

// Mounts:
//   GET  /api/list   -> { "max": int, "items": [ { "text": str, "done": bool }, ... ] }
//   POST /api/list   -> body { "items": [ { "text": str, "done": bool }, ... ] };
//                       replaces the whole list, blanks ignored, capped at
//                       MAX_LIST_ITEMS. Returns { "ok": true }.
//
// Legacy /list (GET HTML + POST form) keeps working through registerListRoutes()
// during the strangler migration. Delete in Phase 4.
void registerApiListRoutes();

#endif  // PALA_WEB_API_LIST_H
