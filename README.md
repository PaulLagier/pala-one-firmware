<img width="1892" height="1053" alt="palaOne" src="https://github.com/user-attachments/assets/0fdef5ba-eabd-4b71-9a0c-4c1dc78a4bee" />

# pala-one-firmware
Pala One — A tiny E-Ink reader project by Paul Lagier

The goal of the project was to create a simple, distraction-free reading device that feels minimal, portable and easy to build while still looking and behaving more like a real product than a typical DIY electronics project.

## Contributing

If you improve the firmware, add features or fix bugs, feel free to open a pull request.
Please clearly mention:
- which board version(s) you tested on (V1.1, V1.2, or both)
- what was changed
- how it was tested

## Board Versions

There are currently two supported Heltec Wireless Paper versions:
- `Heltec V1.1`
- `Heltec V1.2`

The board version is usually printed on the back of the PCB.

Pick your board's revision in the build step below — either by uncommenting the matching `#define` at the top of `Pala_One_2_1/Pala_One_2_1.ino` (Arduino IDE), or by selecting the matching env (PlatformIO).

## Language

The firmware ships with two built-in languages, selected at build time:

- `LANG_EN` — English (default)
- `LANG_ES_LA` — Spanish (Latin America)

One language per binary. PlatformIO users pick a leaf env that already encodes both the board and the language (`wireless-paper-v1_2-en`, `wireless-paper-v1_2-es`, `wireless-paper-v1_1-en`, `wireless-paper-v1_1-es`). Arduino IDE users uncomment one of `LANG_EN` / `LANG_ES_LA` near the top of `Pala_One_2_1/Pala_One_2_1.ino`, alongside the board `#define`. If nothing is set, the firmware compiles with `LANG_EN` and a `#pragma message` warning.

Strings live in `Pala_One_2_1/src/lang/` — `en.h` is the canonical key set; `es_la.h` mirrors it. Adding a new language is additive: clone one of the headers, add the include arm in `src/lang/lang.h`, and add two leaf envs in `platformio.ini` (one per board). See `src/lang/lang.h` for the authoring rules (key set, placeholders, JS-confirm quoting constraint).

Glyph coverage: Latin Extended (`á é í ó ú ñ Ñ ¿ ¡ ü Ü`) is provided by `u8g2_font_helv*_te` for body, bold, app-large and toast roles. The small bitmap fonts used for the battery percentage and page-number indicator stay on ASCII-only `_tf` tables — they only render digits / `%`, and any translation routed through them would render missing-glyph boxes. Web responses declare `charset=utf-8`.

## Building the firmware

The same sources build under either toolchain.

### Arduino IDE 2

1. Install the **esp32 by Espressif Systems** board package (Boards Manager) and select the **Heltec WiFi LoRa 32 V3** board.
2. Install these libraries via Library Manager (or by URL):
   - [`heltec-eink-modules`](https://github.com/todd-herbert/heltec-eink-modules) (todd-herbert fork)
   - **Adafruit GFX Library** (Adafruit)
   - **U8g2_for_Adafruit_GFX** (olikraus)
3. Open `Pala_One_2_1/Pala_One_2_1.ino`. Uncomment exactly one of `BOARD_V1_1` / `BOARD_V1_2` at the top.
4. Tools → Partition Scheme → **Custom** (the sketch ships its own `partitions.csv`).
5. Verify / Upload.

### PlatformIO

1. Install [PlatformIO Core](https://platformio.org/install/cli) (CLI) or the PlatformIO IDE extension for VS Code.
2. From the repo root:
   ```
   pio run -e wireless-paper-v1_2-en -t upload    # V1.2 panel, English
   pio run -e wireless-paper-v1_2-es -t upload    # V1.2 panel, Spanish-LA
   pio run -e wireless-paper-v1_1-en -t upload    # V1.1 panel, English
   pio run -e wireless-paper-v1_1-es -t upload    # V1.1 panel, Spanish-LA
   ```
3. Serial monitor:
   ```
   pio device monitor
   ```

Both envs share libraries and partition table via `platformio.ini`. The PIO build also runs `scripts/build_info.py` to inject the current git short hash as `BUILD_GIT_HASH`; Arduino IDE builds fall back to `"unknown"`.

## Codebase layout

```
Pala_One_2_1/
├── Pala_One_2_1.ino     # Sketch entry: board selection + setup()/loop()
├── pala_api.h           # Public app API (firmware ↔ app ABI)
├── pala_app.h           # PalaAppHeader + version constants
├── partitions.csv       # ESP32 partition table
└── src/                 # Firmware modules
    ├── config.h         # Compile-time constants
    ├── state.{h,cpp}    # Globals (display, WiFi server, prefs)
    ├── pure/            # Pure C++, no Arduino headers — host-testable
    ├── hal/             # Hardware adapters (display, battery, input)
    ├── storage/         # KV store + on-disk persistence
    ├── ui/              # Screens, fonts, sleep, toasts, widgets
    │   └── screens/     # One file per screen
    └── web/             # Captive-portal HTTP server (route groups)
docs/                    # Architecture notes + refactor journal
scripts/                 # PlatformIO pre-build helpers
test/                    # Host-side CMake unit tests for pure/ + storage/
examples/                # Sample apps (click_counter, palagotchi)
archive/                 # Past firmware revisions, kept for reference
```

The intent of the layering is **pure → storage → hal → ui**: pure modules never include `Arduino.h`, so they compile into the host test build verbatim. Storage adds an on-disk persistence shim behind `KeyValueStore`, hal isolates the hardware, and the UI sits on top.

### Include style

Firmware sources include each other by the full path from the sketch root, e.g. `#include "src/hal/display.h"`, not the shorter `#include "hal/display.h"`.

This is to stay compatible with Arduino IDE. The IDE recursively compiles files inside the `src/` subfolder of a sketch, **but does not add `src/` to the compiler's include path** — only the sketch folder root is on it. So `#include "config.h"` from a file at sketch root fails (the file actually lives at `src/config.h`), whereas `#include "src/config.h"` resolves correctly. PlatformIO is happy either way; we picked the Arduino-IDE-compatible form so the same `#include` lines work in both build systems with no extra `-I` flags.

## Host-side tests

Pure modules and KV-backed storage have host-side unit tests under [`test/`](test/). They build with CMake and run on your laptop — no board required.

See [test/README.md](test/README.md) for prerequisites (CMake + a C++17 compiler) and per-platform setup / run instructions for Windows, Linux, and macOS.

## Apps

Pala One supports user-installable apps — self-contained position-independent C binaries that run on top of the firmware and have access to the display, button, RTC, and a per-app key-value store. Apps are uploaded over Wi-Fi through the same web UI used for books, and they appear under the **Apps** entry of the library menu. No firmware rebuild is needed to install one.

See [examples/GETTING_STARTED.md](examples/GETTING_STARTED.md) for the full app-author guide — binary format, the `PalaAPI` (v3), required compiler flags, and upload steps.

### Building an app

You need:
- The `xtensa-esp32s3-elf-gcc` cross-compiler, which ships with the Arduino ESP32 board package. On Linux it is found under `~/.arduino15/packages/esp32/tools/esp-x32/<version>/bin/`; the `Makefile` locates it automatically.
- `python3` (for the post-build step that patches the entry point offset into the binary).

An app is a single C file that includes `pala_app.h` and `pala_api.h` from the firmware source and exports a `void app_main(const PalaAPI* api)` entry point. The header struct at the start of the binary carries the display name and API version check.

See `examples/click_counter/` for a complete working example with a `Makefile`.

```bash
cd examples/click_counter
make
# produces click_counter.bin
```

### Uploading an app

1. Select **Upload** from the library menu on the device.
2. Connect to the `PALA-XXXXXX` WiFi network (password: `palaread`).
3. Open `http://192.168.4.1` in a browser.
4. Use the **Upload app (.bin)** card to upload your `.bin` file.
5. Triple-click to exit upload mode — the app will appear in the Apps menu immediately.

### App API

Apps communicate with the firmware through the `PalaAPI` function pointer table passed to `app_main`. The current API version is **v3** (`PALA_API_VERSION 3` in `pala_app.h`).

#### Display

| Function | Description |
|---|---|
| `clearScreen()` | Clear the display buffer and prepare a new frame |
| `drawHeader(title)` | Draw the standard section header bar |
| `drawTextAt(x, y, text, bold)` | Draw text at a pixel position |
| `drawCenteredLarge(text)` | Draw text centred on screen in a large font |
| `refreshDisplay()` | Push the frame buffer to the e-ink panel |

#### Input

| Function | Description |
|---|---|
| `waitForEvent()` | Block until a button gesture; returns `PALA_CLICK` / `PALA_DOUBLE` / `PALA_TRIPLE` / `PALA_LONG` |
| `pollEvent()` | Non-blocking variant; returns 0 if no event is ready |
| `buttonPressed()` | Returns 1 if the button is currently held, 0 otherwise |
| `pendingPresses()` | Count of individual short press-release events since last call; bypasses multi-click grouping |

#### Timing

| Function | Description |
|---|---|
| `millisNow()` | Current uptime in milliseconds |
| `delayMs(ms)` | Yield for `ms` milliseconds |
| `rtcSeconds()` | Monotonic seconds counter that survives deep sleep; use for cross-session timing |

#### Storage

| Function | Description |
|---|---|
| `storageRead(key, buf, maxlen)` | Read from `/apps/{key}.dat`; returns bytes read, -1 on error |
| `storageWrite(key, buf, len)` | Write to `/apps/{key}.dat`; returns bytes written, -1 on error |

#### Utilities

| Function | Description |
|---|---|
| `snprintf_wrap(buf, len, fmt, ...)` | Standard `snprintf` |

Return from `app_main` to exit back to the Apps menu. Apps decide their own exit gesture — the firmware does not impose one.

**Constraints:**
- Apps must be compiled `-fPIC -mlongcalls` (position-independent).
- Apps must not use static mutable variables — the loader does not patch `.data` relocations.
- Maximum binary size: 48 KB.
- The `api_version` field in `PalaAppHeader` must match `PALA_API_VERSION` exactly — the firmware rejects mismatched binaries.

## Features

- TXT book support
- Adjustable font size
- Adjustable line spacing
- Deep sleep mode
- Reading progress saving
- USB-C charging
- Lightweight portable design
- Open-source firmware

## Hardware

Pala One is based on:
- Heltec Wireless Paper
- 3D printed housing
- LiPo battery

## Downloads

This repository contains the firmware source code for the project.

Additional files such as:
- STL files
- STEP files
- assembly guides
- printable files
- project downloads

are available separately via Ko-fi:

https://ko-fi.com/s/e14ed892ea

## Community & Modifications

Community improvements, forks and firmware modifications are welcome.
If you build your own version or improve the project, feel free to share it with the community.

## License & Copyright

The firmware in this repository is provided for personal and educational use.

Please do not:
- reupload paid project files
- redistribute complete download packages
- resell the project files
- commercially redistribute modified versions of paid assets

The design, branding, documentation and paid project assets remain copyright © Paul Lagier.

---

Created by Paul Lagier
