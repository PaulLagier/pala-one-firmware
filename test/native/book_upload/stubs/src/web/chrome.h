#pragma once
#include "src/pure/arduino_compat.h"
inline String htmlEscape(const String& s) { return s; }
inline String humanBytes(size_t n) { return String(static_cast<unsigned long>(n)); }
inline String storageCardHtml() { return ""; }
inline String successPage(const String&,const String&,const String&,const String&) { return ""; }
