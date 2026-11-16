#pragma once
#include "src/pure/arduino_compat.h"
#include <stddef.h>
#include <stdint.h>
namespace WebAssets {
enum class Id : uint8_t {CoreCss,ReadCss,EditorCss,EditorJs,Count};
struct Asset {const char* url;const char* mime;const uint8_t* gzip;size_t gzipSize;size_t rawSize;};
const Asset& get(Id id);
void serve(Id id,bool immutable=true);
void registerRoutes();
void appendStyle(String& out,Id id);
void appendScript(String& out,Id id);
}
