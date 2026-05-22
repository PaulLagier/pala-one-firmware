#ifndef PALA_WEB_CHROME_H
#define PALA_WEB_CHROME_H

#include "src/pure/arduino_compat.h"  // String
#include "src/config.h"                // D_WEB_STORAGE_HEADING default for storageCardHtml (via lang.h)

// ============================================================================
//  Web page chrome — shared helpers used by every route group.
//
//  Pages are built via String concatenation today. The CSS is served as a
//  cached `/style.css` resource; pages just `<link>` to it.
// ============================================================================

// Open a page: <html><head> + title/viewport + linked stylesheet, then a
// <body> wrapper with header (title + subtitle + nav). `wide` widens the
// content max-width for tabular pages (files, bookmarks, list).
String webPageStart(const String& title, const String& subtitle,
                    const String& navHtml, bool wide = false);

// Close out the body + html the matching way.
String webPageEnd();

// Standard "we did the thing" page — banner + inner card content + nav.
String successPage(const String& title, const String& subtitle,
                   const String& banner, const String& innerHtml);

// HTML-escape user-supplied text so we don't break the page or open XSS.
String htmlEscape(const String& in);

// "1.4 KB" / "23.7 MB" formatting for storage stats.
String humanBytes(size_t bytes);

// Used + free + total + percent-used storage card. Title is the card heading
// (defaults to the language-appropriate "Storage" string).
String storageCardHtml(const char* title = D_WEB_STORAGE_HEADING);

// For external callers that want just the percent (e.g. banners).
int storageUsedPct();

// Mount /style.css — call once from registerWebRoutes().
void registerChromeRoutes();

#endif  // PALA_WEB_CHROME_H
