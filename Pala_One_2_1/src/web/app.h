#ifndef PALA_WEB_APP_H
#define PALA_WEB_APP_H

// ============================================================================
//  SPA + JSON API routes (Phase 0 of the web-UI rewrite).
//
//  Mounts:
//    GET /app       — gzipped SPA shell (see scripts/build_webui.py)
//    GET /api/info  — { lang, fw, build } — used by the SPA at boot to pick
//                     a locale (compile-time language wins over browser).
//
//  Lives alongside the old String-built routes (registered separately in
//  web.cpp) until each screen has been ported to the SPA. The old routes
//  stay reachable at their current paths until Phase 4 cutover.
// ============================================================================

void registerAppRoutes();

#endif  // PALA_WEB_APP_H
