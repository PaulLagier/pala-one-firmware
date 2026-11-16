#pragma once
#include "src/pure/library_nav.h"
extern Catalog g_library;
void loadBooks();
int bookIndexForPath(const String& path);
