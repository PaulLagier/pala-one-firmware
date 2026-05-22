# Web UI emulator

The `dev/` directory contains a host-side emulator for the captive-portal web UI. It builds a native binary (`pala_web_emu`) that serves the same route handlers as the firmware, backed by a local filesystem instead of LittleFS. This lets you iterate on the web UI without flashing the device.

It shares a CMake build with the integration tests (`pala_web_test`).

## Requirements

- **CMake 3.14+**
- A **C++17 compiler** — GCC, Clang, or MSVC
- Internet access on first build (CMake fetches [cpp-httplib v0.15.3](https://github.com/yhirose/cpp-httplib) via `FetchContent`)

## Run

```bash
cmake -S dev -B dev/build && cmake --build dev/build
dev/build/pala_web_emu
```

Then open `http://localhost:8080` in a browser.

Run from the repo root — the emulator defaults to `dev/fs` as the LittleFS root. The repo ships `dev/fs/` with a short sample book (`books/sample.txt`) so the files page loads populated out of the box. Drop any UTF-8 `.txt` file into `dev/fs/books/` to add more.

To override the filesystem root or port:

```bash
dev/build/pala_web_emu --port=9090
```

## Request logging

Every request and its HTTP response status print to stderr:

```
POST /jumptext body=id=0&query=Bennet
  -> 302
GET /files
  -> 200
```

This is always on — no flag needed.

## Integration tests

The integration tests (`pala_web_test`) spin up the same server in a thread and make real HTTP requests against it. They live in `dev/integration_test.cpp` and use the same `dev/fs_test/` fixture filesystem.

Run them after building:

```bash
ctest --test-dir dev/build --output-on-failure
```

Or as a one-liner from the repo root:

```bash
cmake -S dev -B dev/build && cmake --build dev/build && ctest --test-dir dev/build --output-on-failure
```

## Limitations

- **Character widths** use a lookup table approximation for `u8g2_font_helvR12_te`. Line breaks match the device closely but may differ by a word or two for lines containing many narrow or wide characters.
- **On-disk page cache** (`src/storage/page_cache`) is stubbed and does nothing — page walks are always computed from scratch.
- **App upload** returns HTTP 501 (not implemented).
