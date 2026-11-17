# Pala One companion connection

The normal Upload menu now offers **Wi-Fi / browser**, **Bluetooth / app** and
Back. Wi-Fi remains first and retains its existing station/hotspot workflow,
web settings, book and app uploads. No existing library entry, application or
on-device setting is removed. The chooser can sleep normally; an active
Bluetooth or Wi-Fi session inhibits deep sleep and exits after 15 minutes.

Select Bluetooth, open the Pala app and choose Pala One. The reader displays a
six-digit pairing PIN. Enter that PIN in the phone's pairing dialog. The screen
shows Connected after authenticated pairing. Leave with the same exit gesture
as the existing Upload screen: short/triple click in legacy controls, or the
configured Home action in standard controls. Leaving stops the radio, removes
unfinished staging data and refreshes the library. A bonded phone may reconnect
without another PIN prompt. The PIN is never printed to the serial console.

## Builds

All six PlatformIO board/initial-language targets enable `PALA_COMPANION`.
They pin NimBLE-Arduino 2.5.2 and use LTO; the public flash partition layout is
unchanged. The existing Arduino IDE core build remains available without that
flag and retains Wi-Fi upload. The app-enabled images should be built with
PlatformIO: its linker setup supplies the SDK IPC stack guard required for
radio initialization on the pinned Arduino 3.3.8 / IDF 5.5.4 toolchain.

The guard raises **only** exact SDK `ipc0`/`ipc1` creation calls to at least
2048 bytes, checking core, argument, output handle and maximum task priority.
All unrelated task creation arguments remain unchanged. Defining a CONFIG
value in a sketch cannot change those precompiled SDK calls. The small policy
is host-tested; stack allocation/high-water functions remain available for
device diagnostics. Changing the platform or RTOS ABI requires revisiting it.

## Protocol v3

Service: `a28b1001-4d48-4e42-9b82-6b6c50414c41`.

| UUID suffix | Purpose | Maximum value |
| --- | --- | --- |
| `1002` | Identity, authenticated encrypted read | 512 bytes |
| `1003` | Control JSON, authenticated encrypted write | 512 bytes |
| `1004` | Response JSON, authenticated encrypted read | 512 bytes |
| `1005` | Binary upload data, authenticated encrypted write/write-without-response | 512 bytes |
| `1006` | Response request-ID notification | 4 bytes |

All suffixes use the same remaining UUID bytes as the service. Notifications
are hints; read Response and match the request ID and connection `session`.
The peripheral requests bonding, MITM and Secure Connections. Application
reads/writes require an encrypted, authenticated connection and the single
accepted owner. Identity contains model, stable device ID, firmware, free space,
protocol/version and capabilities: `bookmarks-v1`, `bulk-window-v1`, `session-v1`
and `pbt1`. A fresh nonzero session token and an internal generation isolate
reconnects, including controller-handle reuse. Old queued frames and replies do
not migrate to a new connection. A request accepted before disconnect may
finish; its old-session reply is suppressed.

Control is a JSON object with a nonzero uint32 `id` and string `op`:

- `ping`: storage totals, transfer status and supported packed format.
- `books.list`: `cursor` starts at zero; one bounded book per response, `next`
  is another cursor or null. Sizes describe actual stored bytes.
- `books.delete`: an existing catalog `path`; also removes its metadata/cache.
- `bookmarks.list`: opaque integer cursor; returns a bounded item with path,
  zero-based page and decoded byte offset (null for unresolved legacy marks).
- `bookmarks.text`: exact path/page/offset identity plus UTF-8 byte cursor;
  returns up to 160 text bytes, `resolvedOffset` and `next`. A stale mark is
  rejected instead of selecting another bookmark.
- `bookmarks.delete`: exact mark identity, followed by a persisted readback.
- `settings.get`, `settings.advanced.get`, `settings.set`: typed settings using
  existing storage APIs. Unsupported keys/types/ranges fail. Advanced settings
  include original public gesture IDs and legacy-control status.
- `upload.begin`, `upload.cancel`, `upload.finish`: staged transfer lifecycle.

Existing public bookmark blobs retain their uint16 page format. The app's
legacy hold-action fields use `0:none, 1:bookmark, 2:lock, 3:menu`, explicitly
mapped to public firmware IDs. A public binding outside those four values is
reported as `-1`; merely reading settings does not change it. Advanced
`rawActions` reports the original bindings. Short/double/triple bindings are
preserved. Timing changes reject values that would silently adjust another
field. All eleven runtime languages are advertised. Setting `language` selects and
persists a supported ISO code; failed preparation or persistence leaves the
previous selection active. See [Runtime languages](RUNTIME-LANGUAGES.md).

### Transfers

`upload.begin` takes a leaf `.txt` or `.pbt` `name` and physical byte `size`.
No existing book is overwritten. Paths/hidden names and oversize requests fail.
Plain text is limited to 2 MiB; PBT permits its header/table overhead up to
2103316 physical bytes (see [PBT1](PBT1.md)). Replies include `uploadId` and the
next expected `offset`.

Binary Data starts with three little-endian uint32 words: request ID, upload ID
and byte offset, followed by 1–500 data bytes. Negotiate the ATT MTU and choose
a payload that fits the connection. Offsets must be contiguous; duplicate,
out-of-order, stale-transfer and oversize frames fail. Use at most four pending
packets. The fixed queue holds eight and executes at most four operations per
main-loop tick. Overflow explicitly fails/clears the window; any RPC error
aborts the current staging transfer and discards queued work. After an error,
start a new transfer. Filesystem/NVS application work occurs on the main loop,
not in Bluetooth callbacks.

`upload.finish` takes `uploadId` and the lowercase or uppercase 64-character
SHA-256 of the **physical** bytes. The byte count and digest must match. PBT
containers additionally validate every decoded block/CRC before rename. Plain
text bytes are retained as sent; normalize them in the app if desired. A
successful finish atomically moves staging into `/books/`, then refreshes the
catalog/identity. Cancel, disconnect, session exit, write/hash/validation failure
remove incomplete staging. Reserved `.pala-ble-*.tmp` remnants are cleaned on
the first companion session after boot; unrelated temporary files are retained.

## Verification

Host tests exercise production queue, transport, RPC and bookmark code with
bounded SDK/filesystem adapters, real ArduinoJson, system zlib and OpenSSL.
They include malformed requests, ownership/authentication, stale session races,
queue overflow, exact binary transfers/hash failures, metadata format retention,
UTF-8 cursors and settings compatibility. Firmware builds compile the real
NimBLE/mbedTLS/miniz APIs for both board revisions. Host tests do not substitute
for a physical radio/phone pairing test or claim V1.1 hardware coverage.

### Reading-focused navigation

`settings.set` accepts the boolean `readingMenu`; `settings.advanced.get` reports it. The compact `settings.get` response keeps its existing size budget. The reading view shows books and folders, bookmarks, connection and Device. Device contains statistics, updates, firmware information and a library return. Short presses move focus; a double press or hold opens the selected item. Reader gesture mappings remain unchanged. Turning the view off restores the complete custom library menu order and installed apps without changing their data. The web settings page provides a separate reading-menu form.

Bookmarks support unsigned 32-bit page labels. Existing V1 and V2 records remain readable; ordinary records are still written as V2. A bookmark beyond page 65535 uses the extended V3 format with its canonical byte offset. Firmware predating V3 cannot read those extended records; retain a data backup before rolling back across this format boundary.

### Optional reader controls

`settings.set` accepts `quadAction` as an integer using the public action IDs
`0:none, 1:next, 2:previous, 3:menu, 4:lock, 5:home, 6:bookmark, 7:rotate`.
It defaults to no action. `settings.advanced.get` reports it as `rawActions.quad`.
The existing hold-action wire values and app API v3 event constants are unchanged.
Quad bindings apply with custom controls enabled; legacy controls retain their
existing gestures. A Quad binding to Lock also becomes a valid unlock gesture.

`lockIcon` is a boolean, enabled by default. It controls only the badge on the
sleep screen, independently of the saved lock state and unlock gestures.
The advanced response reports it alongside `statusbar` (`0:full, 1:minimal,
2:hidden`). The browser provides these two display preferences in a dedicated
form; changing progress-bar height preserves the canonical reading position.

### Optional bookmark-action toggling

`settings.set` accepts the boolean `bookmarkToggle`; `settings.advanced.get`
reports it. It defaults to false, preserving existing add-only gestures.
When enabled, the reader's bookmark action removes a unique mark whose
canonical offset is on the displayed page, or adds a mark at the current canonical offset. Stale page hints may repeat
for distinct offsets; capacity and eviction rules are unchanged. Legacy marks without offsets use their page hint. Multiple
matches are retained and the reader directs the user to the app or browser.
Reading progress and page geometry are unchanged by the mutation.
