#pragma once
#include "src/pure/library_nav.h"

namespace NavigationProfile {
// The default retains the public interface. The flasher/app/web can select
// the reading-focused view; changing views never rewrites the custom order.
void loadSettings();
bool readingFocused();
bool setReadingFocused(bool enabled);
int filterSystemEntries(LibraryEntryType *entries, int count);
} // namespace NavigationProfile
