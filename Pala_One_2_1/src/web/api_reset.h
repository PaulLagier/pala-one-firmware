#ifndef PALA_WEB_API_RESET_H
#define PALA_WEB_API_RESET_H

// Mounts:
//   POST /api/reset  — factory reset; returns {"ok":true} on success.
void registerApiResetRoutes();

#endif  // PALA_WEB_API_RESET_H
