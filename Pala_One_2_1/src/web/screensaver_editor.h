#ifndef PALA_WEB_SCREENSAVER_EDITOR_H
#define PALA_WEB_SCREENSAVER_EDITOR_H

#include "src/pure/arduino_compat.h"  // String

// In-browser canvas editor for 250x122 1-bit screensaver images (3904 bytes).
// When multiScreensaver is false, uploads always go to /upload-sleep (/sleep.bin).
String screensaverEditorHtml(bool multiScreensaver, bool hasFreeSlot);

#endif
