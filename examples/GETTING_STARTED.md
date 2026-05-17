# Building a Pala One App

Apps for the Pala One are self-contained position-independent C binaries uploaded to the device over WiFi. This guide covers everything from prerequisites to uploading your first app.

## Prerequisites

- **xtensa-esp32s3 toolchain** — installed automatically by Arduino IDE when you add the Heltec ESP32 board package. The Makefile looks for it under `~/.arduino15/packages/esp32/tools/esp-x32/`. Override `TOOLCHAIN` on the `make` command line if yours is elsewhere.
- **Python 3** — used by the post-build step that patches the binary header.
- **`make`** — standard GNU make.

## App Binary Format

Every app binary begins with a `PalaAppHeader` struct at byte offset 0. The firmware reads this header to validate and load the app.

```c
typedef struct {
    uint32_t magic;         // Must be PALA_APP_MAGIC (0x50414C41, 'PALA')
    uint32_t api_version;   // Must equal PALA_API_VERSION in pala_app.h
    char     name[32];      // Display name shown in the app launcher
    uint32_t entry_offset;  // Byte offset to app_main() — patched by Makefile
    uint32_t reloc_offset;  // Byte offset to relocation table — patched by Makefile
    uint32_t reloc_count;   // Number of relocation entries — patched by Makefile
} PalaAppHeader;
```

Always declare it in the `.header` section and initialise `entry_offset`, `reloc_offset`, and `reloc_count` to 0 — the Makefile patches the real values after linking:

```c
#include "../../Pala_One_2_1/pala_app.h"
#include "../../Pala_One_2_1/pala_api.h"

__attribute__((section(".header")))
const PalaAppHeader pala_header = {
    .magic        = PALA_APP_MAGIC,
    .api_version  = PALA_API_VERSION,
    .name         = "My App",
    .entry_offset = 0,
    .reloc_offset = 0,
    .reloc_count  = 0,
};
```

## Entry Point

Your app must define exactly one function with this signature:

```c
void app_main(const PalaAPI* api);
```

The firmware calls it after loading the binary and relocating it into RAM. The `api` pointer gives you access to all firmware services. When `app_main` returns, the firmware returns to the app launcher.

## The PalaAPI (v4)

All interaction with the device goes through function pointers in `PalaAPI`. Field order is frozen — new fields are always appended, never inserted. Do not cache the pointer; use it directly.

| Function | Description |
|---|---|
| `clearScreen()` | Erase the frame buffer |
| `drawHeader(title)` | Draw the standard title bar at the top |
| `drawTextAt(x, y, text, bold)` | Draw text at pixel coordinates; `bold=1` for bold weight |
| `drawCenteredLarge(text)` | Draw large bold text horizontally centred |
| `drawXBitmap(x, y, bits, w, h, color)` | Blit a 1bpp XBitmap. `int16_t` coords/dims, `uint16_t color`. **LSB-first**, packed rows, `((w+7)/8)` bytes per row. `color=1` draws set bits as black ink on the current canvas (default white after `clearScreen`); `color=0` draws them as white (useful when you have inverted to a black canvas). Off-screen coordinates are clipped automatically. **Silently skipped** if `bits==NULL`, `w<=0`, `h<=0`, the right/bottom edge would exceed `INT16_MAX`, or the total byte budget `((w+7)/8)*h` exceeds 64 KB (~17x full-screen) — design the bitmap budget around this, the firmware will not error. See `examples/campfire/` for a worked example with a PIL-based Bayer 4x4 converter that bakes a PNG spritesheet into a 1bpp header. |
| `refreshDisplay()` | Flush the frame buffer to the e-ink panel |
| `waitForEvent()` | Block until a button gesture arrives; returns an event code |
| `pollEvent()` | Non-blocking version; returns 0 if no event is ready |
| `buttonPressed()` | Returns 1 if the button is physically held down right now |
| `pendingPresses()` | Count of individual short press-release events since last call; bypasses multi-click grouping |
| `millisNow()` | Milliseconds since boot (resets after deep sleep) |
| `rtcSeconds()` | Monotonic seconds; **survives deep sleep** — use for cross-session timing |
| `delayMs(ms)` | Yield for the given number of milliseconds |
| `snprintf_wrap(buf, len, fmt, ...)` | `snprintf` — the only safe way to do formatted strings (no stdlib) |
| `storageRead(key, buf, maxlen)` | Read `/apps/{key}.dat`; returns bytes read, -1 on error |
| `storageWrite(key, buf, len)` | Write `/apps/{key}.dat`; returns bytes written, -1 on error |

### Button Event Codes

```c
#define PALA_CLICK   1   // single click
#define PALA_DOUBLE  2   // double click
#define PALA_TRIPLE  3   // triple click
#define PALA_LONG    4   // long press (fired on release, via waitForEvent/pollEvent)
```

For an exit-while-held pattern (button held → exit immediately, no release needed), poll `buttonPressed()` and time it yourself — see the click_counter example.

## Display

The e-ink panel is **250 × 122 pixels**. The coordinate origin (0, 0) is the top-left corner; y increases downward. Text y coordinates are baselines, not tops.

`drawHeader()` occupies roughly the top 18 pixels. Draw content below y ≈ 20 to avoid overlap.

The font used by `drawTextAt` is proportional (Helvetica 8pt), so character widths vary. If you need fixed-width columns (e.g. stat bars), draw each character individually at a manually computed x offset.

## Building

Copy an existing example directory and adapt it. The required files are:

```
my_app/
  app.c         your app source
  Makefile      build rules (copy from an example and change the target name)
  pala_app.ld   linker script (identical for all apps — just copy it)
```

```
cd examples/my_app
make
```

This produces `my_app.bin`. The Makefile:

1. Compiles `app.c` with `-fPIC -mlongcalls -Os` into a shared-style ELF.
2. Strips to a raw binary with `objcopy`.
3. Runs a Python snippet that locates `app_main` via `nm`, finds all `R_XTENSA_RELATIVE` relocations via `readelf`, and patches the header fields in-place.

### Critical Compiler Flags

| Flag | Why it is required |
|---|---|
| `-fno-jump-tables` | **Required for apps with if-else chains or enums.** GCC may emit jump tables in `.rodata`. The firmware's relocator only patches `.literal` pool entries; `.rodata` jump tables are **not** patched. At runtime the dispatch jumps to unrelocated addresses and the device crashes. |
| `-fPIC` | Generates position-independent code so the binary can be loaded at any address. |
| `-mlongcalls` | Needed for long-range calls in PIC mode on Xtensa. |
| `-nostdlib -nodefaultlibs` | No standard library — `malloc`, `printf`, `memcpy`, etc. are unavailable. Use only `api->snprintf_wrap` for formatting. |

> **If the device restarts the moment you open your app**, the most likely cause is a jump table in `.rodata`. Make sure `-fno-jump-tables` is in your CFLAGS.

## Uploading

1. On the device, navigate to **Upload** mode (triple-click from the library screen).
2. The device shows its WiFi AP name (`PALA-xxxxxx`) and password (`palaread`).
3. Connect to that network, then open `http://192.168.4.1` in a browser.
4. Upload your `.bin` file to the path `/apps/my_app.bin`.

The app appears in the app launcher the next time the device loads the app list.

## Tips

- **No dynamic memory.** Allocate everything on the stack or as static locals.
- **No global mutable state with pointers.** Globals that contain absolute pointers will not be relocated correctly. Pass data through function arguments instead.
- **String literals are fine** — they land in `.rodata` or `.literal` and are patched by the relocator.
- **Keep binaries small.** The app is loaded into RAM at runtime. A few kilobytes is typical; the device has limited free heap.
- **Test with `pollEvent` + `delayMs(10)`** in your main loop if you need timers to fire alongside input — `waitForEvent` blocks indefinitely.
- **Use `rtcSeconds()` for persistence timing**, not `millisNow()`. The millisecond timer resets after every deep sleep cycle (idle timeout is 120 s by default).

## Examples

| App | What it demonstrates |
|---|---|
| `click_counter/` | Minimal app: event loop, display updates, long-press exit, `pendingPresses()` |
| `palagotchi/` | Timers, cross-session state with `storageRead`/`storageWrite` + `rtcSeconds`, action screens, stat display |
| `campfire/` | Full-screen `drawXBitmap` animation. `convert.py` upscales the PNG 4x with LANCZOS, rotates 90 deg, Bayer 4x4 dithers aligned to the panel pixel grid, and packs an inverted XBitmap so a single blit per frame renders white flames on a black background |
