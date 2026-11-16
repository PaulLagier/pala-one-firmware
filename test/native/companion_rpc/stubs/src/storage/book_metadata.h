#pragma once
#include <Arduino.h>
#include "../../../../../../Pala_One_2_1/src/storage/book_metadata.h"
uint8_t loadBookmarksForKey(const String&,uint16_t[MAX_BOOKMARKS],uint32_t[MAX_BOOKMARKS]);
void saveBookmarksForKey(const String&,const uint16_t[MAX_BOOKMARKS],const uint32_t[MAX_BOOKMARKS],uint8_t);
void deleteBookMetadata(const String&);
