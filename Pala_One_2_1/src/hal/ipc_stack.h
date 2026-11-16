#pragma once
#include <stdint.h>
namespace RuntimeStack {
uint32_t ipcBytes(int core);
uint32_t ipcFreeBytes(int core);
}
