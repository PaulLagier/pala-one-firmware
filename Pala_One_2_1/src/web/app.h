#ifndef PALA_WEB_APP_H
#define PALA_WEB_APP_H

// ============================================================================
//  SPA + JSON API routes.
//
//  Mounts:
//    GET /          — gzipped SPA shell (see scripts/build_webui.py)
//    GET /api/info  — { lang, fw, build } — used by the SPA at boot to pick
//                     a locale (compile-time language wins over browser).
// ============================================================================

void registerAppRoutes();

#endif  // PALA_WEB_APP_H
