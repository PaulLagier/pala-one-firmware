#pragma once
#include "src/pure/arduino_compat.h"
#include <initializer_list>

namespace Language {
// Construct translated HTML with one reserved buffer, without temporary
// String chains for each translated label and surrounding literal fragment.
String join(std::initializer_list<const char*> parts);
}
