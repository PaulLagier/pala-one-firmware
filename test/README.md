# Host-side unit tests

The CMake/CTest suite runs on a computer without a reader. It combines pure
C++ tests, production firmware modules compiled against deterministic hardware
adapters, and Python/Node.js checks of generated assets, browser behavior and
release packaging. Firmware SDK builds are a separate validation layer.

Run the commands below from the repository root. Production sources live in
[`Pala_One_2_1/src/`](../Pala_One_2_1/src/). Host adapters replace external
hardware boundaries; keep the production implementation under test wherever
possible instead of duplicating its decisions in the test.

> **Note:** PlatformIO's built-in test runner (`pio test`) also defaults to
> this directory. We don't use it — `platformio.ini` sets `test_ignore = *`
> so `pio test` is a no-op. Run tests with CMake / `ctest` as shown below.

## Validation strategy

| Layer | Run / automation | What it establishes |
|---|---|---|
| Pure logic and storage codecs | CMake / CTest; `Host tests` on every push and PR | Deterministic algorithms, bounds, serialization and migration behavior |
| Firmware integration with host adapters | Same CTest suite | Production parser, upload, reader, recovery, input, idle-loop and companion lifecycle behavior under injected failures |
| Browser and generated resources | Python and Node.js targets registered in CTest | Locale completeness, deterministic bundles, browser search/editor behavior and installer selection |
| Static analysis | `Static analysis` workflow | cppcheck findings in the V1.2 English configuration; this is not exhaustive analysis of all preprocessor variants |
| ESP32 SDK compilation | `Build firmware` workflow | All six V1.1/V1.2 × EN/ES/FR default builds link and their final padded images fit the 1,900,544-byte OTA slot |
| Release packaging | CTest packaging checks and tag deployment validation | Board/chip identity, offsets, artifact metadata and checksums before publication |
| Physical reader | Separate observations on available hardware | Actual display, electrical input, wake response, radio behavior and persistence on that device |

Each default-language image contains all eleven runtime catalogs. Compilation
for both boards does not establish physical display or power behavior on both.
Host Wi-Fi, filesystem and graphics adapters do not measure charging, current
draw, router compatibility, real flash power loss or e-paper timing.

For a bug fix, first reproduce the observable failure at the smallest practical
layer, exercise the actual affected implementation, and check the resulting
state as well as the response. Add fault cases when they protect meaningful
behavior (for example interrupted uploads or stale provisioning callbacks).
Document a physical-only failure honestly when it cannot be reproduced on the
host. Formatting and prose changes normally need a diff review, not new tests.

Before merging firmware changes, require the host suite, static analysis and
all six SDK builds to pass. Use the release workflow to package a tagged build;
do not substitute an unrelated local binary. Record hardware findings separately,
including firmware version, board revision, steps and observed result. Never
publish credentials, raw device settings or book contents as test evidence.

## Requirements

- **CMake 3.13+**
- **Node.js 22** for browser-script tests.
- **Python 3.8+** and the **zlib and OpenSSL development libraries** for packed-book tests.
- A **C++17 compiler** — MSVC, GCC, or Clang. CMake picks one automatically.

### Installing the prerequisites

**Windows**

Install Python 3, zlib and OpenSSL (for example `vcpkg install zlib openssl`, then supply the vcpkg CMake toolchain).

- Install [Visual Studio 2019 or newer](https://visualstudio.microsoft.com/downloads/) with the *"Desktop development with C++"* workload (this gets you MSVC + the Windows SDK), **or** install [Build Tools for Visual Studio](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022) if you don't want the full IDE.
- Install CMake from [cmake.org/download](https://cmake.org/download/), or via `winget install Kitware.CMake` / `choco install cmake`. Make sure it's on `PATH`.
- Run the commands below from a regular PowerShell or `cmd` prompt — CMake will locate MSVC on its own. (You don't need to launch the "x64 Native Tools" shell.)

**Linux**

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake python3 zlib1g-dev libssl-dev

# Fedora
sudo dnf install gcc-c++ cmake make python3 zlib-devel openssl-devel

# Arch
sudo pacman -S base-devel cmake python zlib openssl
```

**macOS**

```bash
xcode-select --install        # Clang + make
brew install cmake openssl@3 node@22
export PATH="$(brew --prefix node@22)/bin:$PATH"
# If OpenSSL is not discovered, add -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)" to CMake.
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
[`src/pure/arduino_compat.h`](../Pala_One_2_1/src/pure/arduino_compat.h) — which provides
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

## Companion app connection

The host suite downloads the same ArduinoJson 7.2.1 release used by firmware,
with a fixed SHA-256 checksum. For offline builds, provide its source directory
using `-DFETCHCONTENT_SOURCE_DIR_ARDUINOJSON=/path/to/ArduinoJson`. Install OpenSSL
(for example `brew install openssl@3` on macOS, then configure with
`-DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl@3` if CMake does not find it).
OpenSSL supplies the host SHA-256 adapter; firmware uses SDK mbedTLS.

`companion_queue_tests` exercises actual owner/session/generation gating,
packet bounds, FIFO/window overflow and the SDK IPC stack-selection policy.
`companion_rpc_tests` compiles the actual JSON/binary RPC and bookmark code with
the real JSON library, production timing settings and metadata codecs. It tests
binary transfers and SHA-256, staging cleanup/failure/overwrite refusal,
settings type/range and cross-field checks, explicit wire-to-public action
mapping, uint16 bookmark compatibility and UTF-8 text cursors.
`companion_ble_tests` compiles the actual transport against a NimBLE contract
adapter: callback security/ownership checks, deferred main-loop execution,
reused handles, disconnect races, overflow, four-operation tick limits,
response sizing and teardown. These host adapters do not claim physical radio,
phone pairing or ESP32 ROM decoder coverage.

The two `companion_ui_*` targets compile the actual connection screen in English
and Spanish. They verify Wi-Fi remains the default, selecting/backing out does
not start radios, Bluetooth initialization runs on the idle tick at 240 MHz,
startup failure allows normal sleep, PIN/status rendering stays on-screen,
legacy/standard exit gestures, no redundant first redraw and deadline wraparound.

## Idle-loop and provisioning lifecycle regressions

`idle_loop_tests` compiles the actual sketch loop extracted by
`scripts/extract_idle_loop.py`, using the production ISR queue and click classifier.
It checks held presses, pending clicks, edges arriving after polling, raw pin
changes, custom unlock sequences, idle deadlines and timer rollover. Screen and
sleep-entry adapters record decisions; these checks do not emulate e-paper hardware.

`provisioning_lifecycle_tests` compiles the production USB wrapper and framed
Improv parser together. Its deterministic clock, UART and radio adapters verify
that idle USB attachment cannot inhibit sleep forever, activity expiry works at
millis zero and rollover, association yields to the loop, retries/cancellation
cannot save stale credentials, and radio teardown respects upload ownership.
The parser target also checks delayed success, the 30-second timeout, recovery
isolation and SDK credential lengths. Tests access no real networks or credentials.

Physical reader checks are recorded separately from host tests. A native pass
or a board build does not establish the cause of an individual hardware freeze,
charging fault or router compatibility report.

## Optional reader controls

`display_settings_tests` compiles the actual browser reading/display-form handlers,
statusbar, lock and gesture storage. It checks strict mode validation before
writes, preservation of reading checkboxes when saving other cards, persisted Full/Minimal/Hidden modes, geometry
invalidation, redraw decisions and an independently hidden lock badge.
`lock_gesture_tests` also covers persisted Quad bindings and matching unlock
behavior. Companion RPC tests check additive control types/ranges and retain
legacy hold-action IDs and the response-size budget. Host graphics adapters do
not establish how a physical sleep image looks.

The reader and image-editor scripts are served as content-hashed gzip assets.
The host probe combines each production script with its actual runtime label
array. Browser tests execute the resulting scripts in all eleven languages;
asset tests separately verify exact decompression, hashes and regeneration.
