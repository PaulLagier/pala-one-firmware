#pragma once
#include "src/state.h"
inline size_t fsFreeBytesSafe(){return FS.freeBytes;}
inline size_t fsTotalBytesSafe(){return 4*1024*1024;}
inline size_t fsUsedBytesSafe(){return fsTotalBytesSafe()-FS.freeBytes;}
