<img width="1892" height="1053" alt="palaOne" src="https://github.com/user-attachments/assets/0fdef5ba-eabd-4b71-9a0c-4c1dc78a4bee" />

# pala-one-firmware
Pala One — A tiny E-Ink reader project by Paul Lagier

The goal of the project was to create a simple, distraction-free reading device that feels minimal, portable and easy to build while still looking and behaving more like a real product than a typical DIY electronics project.

## Install (no toolchain needed)

[Web Installer](https://paullagier.github.io/pala-one-firmware/)

The easiest way to flash a board is via the web installer. Plug your Heltec Wireless Paper into a desktop computer running Chrome, Edge, or Opera, then open the installer page and pick a channel:

- **Stable** ([`/stable/`](https://paullagier.github.io/pala-one-firmware/stable/)) — latest tagged release (`vX.Y.Z`). Use this unless you have a reason not to.
- **Development** ([`/dev/`](https://paullagier.github.io/pala-one-firmware/dev/)) — latest build from `dev`; new features, may break.

Each channel page lists both display revisions (V1.1 / V1.2) and both languages (English / Spanish-LA) — four install buttons total. Pick the one that matches your board + language and click **Install**. The installer keeps existing reading progress, bookmarks, and uploaded books across re-flashes.

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

## Web UI theme

The browser-side configuration UI ships with a light palette and a dark palette and a per-page toggle button in the header. The toggle's choice is stored in the browser's `localStorage` (`palaTheme`), so each device that connects to the captive portal remembers its own preference — there is no server-side persistence.

Out of the box, a first visit defaults to **light**. To change the firmware default (e.g. so a freshly connected device lands in dark mode), pick one of `WEB_THEME_LIGHT` / `WEB_THEME_DARK` at build time, mirroring the language flow:

- **Arduino IDE** — uncomment one of `WEB_THEME_LIGHT` / `WEB_THEME_DARK` near the top of `Pala_One_2_1/Pala_One_2_1.ino` (beneath the language block).
- **PlatformIO** — add `-D WEB_THEME_DARK` to your env's `build_flags` if you want dark as the default; otherwise leave it alone.

The build-time default only affects the *first* visit from a given browser — once the toggle is used, the localStorage choice wins from then on.

## Device lock

The device can be locked to stop accidental input (page turns, menu, navigation) while it rests in a bag or pocket. The locked state is persisted to NVS (`cfg_locked`), so a device that fully powers down comes back locked.

Locking is a remappable button action. In the web UI under **Settings → Buttons** you can bind each of the three hold gestures — long press, very-long press, click-hold — to *None*, *Bookmark*, *Lock device*, or *Menu*. Out-of-box defaults:

- **Long press** → Bookmark
- **Very-long press** (≥ 2 s) → Lock device
- **Click-hold** → Menu

So by default you lock with a very-long press. Unlocking is intentionally **permissive**: *any* long, very-long, or click-hold press unlocks the device and shows an "Unlocked" toast — after a deep-sleep wake the firmware can't reconstruct a specific chord, so it accepts any hold gesture rather than risk locking you out. While locked, the sleep screen shows a small padlock badge in the top-right corner.

## Wi-Fi provisioning (Improv)

Besides the SoftAP captive portal, the firmware supports **Improv Serial** Wi-Fi provisioning ([improv-wifi.com](https://www.improv-wifi.com)) over the USB-CDC port, using the [`jnthas/Improv-WiFi-Library`](https://github.com/jnthas/Improv-WiFi-Library). When the board is plugged into a computer, a browser can hand it Wi-Fi credentials directly — the [web installer](https://paullagier.github.io/pala-one-firmware/) does this right after flashing and then redirects to `connected.html`. Provisioning runs only while a USB host is actually present, so there's no battery cost otherwise.

Saved credentials let the device join your network in **Station mode** the next time it enters the web UI / upload mode; if none are saved (or the join fails) it falls back to the open SoftAP at `192.168.4.1`. See `Pala_One_2_1/src/hal/wifi_provisioning.{h,cpp}` and `src/hal/wifi.cpp`.

## Building the firmware

The same sources build under either toolchain.

### Arduino IDE 2

1. Install the **esp32 by Espressif Systems** board package (Boards Manager) and select the **Heltec WiFi LoRa 32 V3** board.
2. Install these libraries via Library Manager (or by URL):
   - [`heltec-eink-modules`](https://github.com/todd-herbert/heltec-eink-modules) (todd-herbert fork)
   - **Adafruit GFX Library** (Adafruit)
   - **U8g2_for_Adafruit_GFX** (olikraus)
   - [`Improv-WiFi-Library`](https://github.com/jnthas/Improv-WiFi-Library) (jnthas) — serial Wi-Fi provisioning; PlatformIO installs it automatically, Arduino IDE users add it by URL
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

Both envs share libraries and partition table via `platformio.ini`. The PIO build also runs `scripts/build_info.py` to inject:

- `FW_VERSION` from `git describe --tags --always --dirty` (e.g. `v2.1`, `v2.1-3-gabc1234`, `…-dirty`)
- `BUILD_GIT_HASH` from the current short SHA

Arduino IDE / host-test builds skip the script and fall back to `"dev"` and `"unknown"` respectively — those toolchains are for developer iteration; releases go through the PIO + tagged-CI flow where the real values get injected.

### Installer site (channels & CI)

The [web installer](https://paullagier.github.io/pala-one-firmware/) is published to the `gh-pages` branch by [`.github/workflows/deploy-installer.yml`](.github/workflows/deploy-installer.yml). Two channels live side-by-side and never overwrite each other:

| Trigger                       | Channel  | URL path     | `DEBUG_BUILD` | Manifest version |
|-------------------------------|----------|--------------|---------------|------------------|
| push to `dev`                 | `dev`    | `/dev/`      | `1` (git hash visible on device) | `dev-<sha>` |
| tag `v*`                      | `stable` | `/stable/`   | `0` (clean UI) | `vX.Y.Z` |
| `workflow_dispatch`           | choose   | matches      | depends on channel | `dev-<sha>` / `manual-<sha>` |

Merges to `main` do **not** auto-publish. Tagging is the explicit release event, so `/stable/` never carries an arbitrary mid-release snapshot and the install dialog always shows a clean version name. To cut a release: merge to `main`, then `git tag vX.Y.Z && git push origin vX.Y.Z`.

Every run rebuilds the channel it owns and publishes only the corresponding `gh-pages/<channel>/` subdirectory (the workflow uses `keep_files: true`, so the other channel's directory is preserved). A small landing page at the gh-pages root links to both — it is republished on every run with identical content, so the cross-write is safe. The Improv post-provisioning `connected.html` also lives at the gh-pages root (its content is channel-agnostic and the URL is baked into the firmware).

Source HTML/manifests live in `install/` on the normal branches. The `gh-pages` branch is fully generated; do not edit it by hand.

#### Local development

To iterate on the installer page (HTML, Improv Serial provisioning flow, manifest tweaks) without CI:

1. Build all four leaf envs at least once so the firmware bins exist:
   ```
   pio run -e wireless-paper-v1_1-en
   pio run -e wireless-paper-v1_1-es
   pio run -e wireless-paper-v1_2-en
   pio run -e wireless-paper-v1_2-es
   ```
2. Assemble the bundle. Two layouts are supported:
   ```
   python scripts/assemble_site.py                       # flat layout in site/
   python scripts/assemble_site.py --channel dev         # site/index.html + site/dev/
   ```
3. Serve it. Web Serial works on `localhost` without HTTPS:
   ```
   python -m http.server 8000 --directory site
   ```
4. Open <http://localhost:8000> in Chrome, Edge, or Opera. With `--channel`, the landing page is served; without, the installer is served directly.

Optional flags: `--version <string>` to label the manifest, `--out <dir>` to write somewhere other than `site/`. The channel-aware layout produced locally is bit-identical to what the workflow uploads to `gh-pages`.

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
install/                 # ESP Web Tools installer page (deployed to GitHub Pages by CI)
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
- Adjustable font size and line spacing
- Font family choice (Helvetica / OpenDyslexic)
- Bionic reading mode
- Reading progress saving
- Bookmarks (on-device and over the web UI)
- In-book text search (web UI)
- Todo list
- Device lock (see [Device lock](#device-lock))
- Remappable button gestures
- Custom screensaver image
- Adjustable idle sleep timeout
- Wi-Fi provisioning (Improv) + captive-portal web UI
- User-installable apps (see [Apps](#apps))
- Deep sleep mode
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
