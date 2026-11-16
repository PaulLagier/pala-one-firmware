# Host-side unit tests

These tests run on your laptop (not on the device) against the pure modules
in [`src/pure/`](../src/pure/) and the KV-store-backed modules in
[`src/storage/`](../src/storage/) (`bookmarks.cpp`, `list_items.cpp`).

They're a separate build from the firmware. The firmware build (`pio run`)
ignores this directory; CMake here ignores everything that depends on the
ESP32 toolchain.

> **Note:** PlatformIO's built-in test runner (`pio test`) also defaults to
> this directory. We don't use it — `platformio.ini` sets `test_ignore = *`
> so `pio test` is a no-op. Run tests with CMake / `ctest` as shown below.

## Requirements

- **CMake 3.13+**
- **Python 3.8+** and the **zlib development library** for packed-book tests.
- A **C++17 compiler** — MSVC, GCC, or Clang. CMake picks one automatically.

### Installing the prerequisites

**Windows**

Install Python 3 and zlib (for example `vcpkg install zlib`, then supply the vcpkg CMake toolchain).

- Install [Visual Studio 2019 or newer](https://visualstudio.microsoft.com/downloads/) with the *"Desktop development with C++"* workload (this gets you MSVC + the Windows SDK), **or** install [Build Tools for Visual Studio](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022) if you don't want the full IDE.
- Install CMake from [cmake.org/download](https://cmake.org/download/), or via `winget install Kitware.CMake` / `choco install cmake`. Make sure it's on `PATH`.
- Run the commands below from a regular PowerShell or `cmd` prompt — CMake will locate MSVC on its own. (You don't need to launch the "x64 Native Tools" shell.)

**Linux**

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake python3 zlib1g-dev

# Fedora
sudo dnf install gcc-c++ cmake make python3 zlib-devel

# Arch
sudo pacman -S base-devel cmake python zlib
```

**macOS**

```bash
xcode-select --install        # Clang + make
brew install cmake            # CMake (or download from cmake.org)
```

## Run

From the repo root:

```bash
cmake -S test -B test/build                                 # configure (once, or after CMakeLists.txt changes)
cmake --build test/build --config Release                   # build
ctest --test-dir test/build -C Release --output-on-failure  # run
```

Or as a one-liner:

```bash
cmake -S test -B test/build && cmake --build test/build --config Release && ctest --test-dir test/build -C Release --output-on-failure
```

The `-C Release` flag is only meaningful on multi-config generators (Visual Studio on Windows) — it's harmless on single-config generators (Make / Ninja on Linux / macOS), so the same command line works everywhere.

For the verbose per-test output, run the binary directly:

```bash
test/build/Release/host_tests.exe     # Windows / MSVC
test/build/host_tests                 # Linux / macOS / MinGW
```

## What's covered

| Test file | Module under test |
|---|---|
| [`test_paths.cpp`](test_paths.cpp) | path / folder / filename sanitization |
| [`test_text_util.cpp`](test_text_util.cpp) | UTF-8 helpers + typography normalization |
| [`test_hashing.cpp`](test_hashing.cpp) | FNV-1a + per-book preference keys |
| [`test_paginator.cpp`](test_paginator.cpp) | line wrapping at words / punctuation / UTF-8 |
| [`test_bookmark_label.cpp`](test_bookmark_label.cpp) | first-words bookmark label |
| [`test_bookmarks_codec.cpp`](test_bookmarks_codec.cpp) | bookmark byte-blob encode/decode + v1 migration |
| [`test_list_codec.cpp`](test_list_codec.cpp) | todo-list byte-blob encode/decode |
| [`test_bookmarks_store.cpp`](test_bookmarks_store.cpp) | `loadBookmarks` / `saveBookmarks` against `MapKvStore` |
| [`test_list_store.cpp`](test_list_store.cpp) | `loadList` / `saveList` against `MapKvStore` |
| [`test_streak_log.cpp`](test_streak_log.cpp) | reading-streak rolling window, bitmap, v1 → v2 migration |

## Adding a new test

1. Add a `test_<thing>.cpp` here. Mirror the source file you're testing.
2. List the source file (and the new test file) in
   [`CMakeLists.txt`](CMakeLists.txt) under `SUT_SRC` / `TEST_SRC`.
3. Use `TEST_CASE("name") { ... }` with `CHECK(expr)` / `CHECK_EQ(a, b)` /
   `REQUIRE(expr)` from [`test_framework.h`](test_framework.h).

The pure modules compile here because they include
[`src/pure/arduino_compat.h`](../src/pure/arduino_compat.h) — which provides
a minimal `String` shim when `ARDUINO` isn't defined. If you need a method
on `String` that isn't there yet, add it to the shim.

## Boot storage recovery

CTest also builds `storage_recovery_tests` and the recovery screen for English
and Spanish-LA (`storage_recovery_ui_EN`, `storage_recovery_ui_ES_LA`). These
compile the production storage and screen implementations with deterministic
filesystem, GPIO and graphics adapters. They cover non-destructive mount/retry,
explicit initialization and its failures, cancel defaults, final hold-only
confirmation, held/stale transition input, locking and the isolated boot loop.
The UI tests use the real `ButtonEvent`/`ButtonState` declarations; they inject
classified events and do not simulate electrical debounce or an e-ink panel.
No actual filesystem is formatted by these host tests.

## Improv Serial regression tests

`improv_serial_tests` compiles the exact vendored serial parser with deterministic
Stream, clock and Wi-Fi adapters. It does not access real networks or save
credentials. It covers consecutive requests, all 256 advertised payload lengths,
short credential fields, wire checksums, prefix resynchronization, partial-frame
timeouts (including clock wraparound), explicit reset and valid/open-network
credentials. Every device-info response is checked for framing and checksum.

For memory-safety checks with GCC or Clang, configure a separate build with:

```bash
cmake -S test -B test/build-sanitized -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build test/build-sanitized
ctest --test-dir test/build-sanitized --output-on-failure
```

## Packed books

`packed_book_tests` exercises the production container stream with an independent
system-zlib callback: compressed/raw blocks, all header truncations, corrupted
metadata/payload/CRC, exact decoded/input sizes, the 2 MiB boundary, random seeks,
UTF-8 across block boundaries and identical pagination versus plain text. Python
encoder fixtures are consumed by the C++ stream. `pack_book_encoder` checks the
CLI separately, including refusal to overwrite the input or an existing output.
Firmware builds compile the SDK miniz callback; host zlib tests do not claim to
exercise the ESP32 ROM decoder or physical display.

`book_upload_tests` compiles the production multipart upload handler and book
HAL against an in-memory filesystem/server and a host zlib adapter. It checks
binary chunk boundaries, replacement only after full validation, cleanup on
abort/reset, failed writes/rename, low space, cache invalidation and logical
size/seek behavior. The adapter is not a power-loss test of LittleFS.
