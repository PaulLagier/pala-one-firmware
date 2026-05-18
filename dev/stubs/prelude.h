#pragma once
// Force-included before every translation unit in the emulator build.
// System headers must be parsed before arduino_compat.h defines the min/max
// two-argument macros, otherwise zero-argument calls like numeric_limits<T>::min()
// are misinterpreted as macro invocations and fail to compile.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>
