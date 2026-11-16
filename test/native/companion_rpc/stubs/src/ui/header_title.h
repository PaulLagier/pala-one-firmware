#pragma once
#include <Arduino.h>
namespace HeaderTitle {
inline String title="Pala";
inline const char* current(){return title.c_str();}
inline void set(const char* value){title=value;}
}
