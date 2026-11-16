#pragma once
#include "src/state.h"
#include "src/pure/bookmarks_codec.h"
uint32_t resolveBookmarkOffset(const String&,uint16_t,uint32_t);
uint32_t extractPageText(File&,uint32_t,String&);
