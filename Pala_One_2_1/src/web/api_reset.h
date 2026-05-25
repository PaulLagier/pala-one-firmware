#ifndef PALA_WEB_API_RESET_H
#define PALA_WEB_API_RESET_H

// Mounts:
//   POST /api/reset  — factory reset; returns {"ok":true} on success.
//
// The legacy /reset (GET = confirm page, POST = perform) still mounts via
// registerResetRoutes() during the strangler-pattern migration. Delete it
// in Phase 4 once the SPA is the only client.
void registerApiResetRoutes();

#endif  // PALA_WEB_API_RESET_H
