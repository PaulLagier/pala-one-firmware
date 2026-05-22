#ifndef PALA_HAL_WIFI_PROVISIONING_H
#define PALA_HAL_WIFI_PROVISIONING_H

#include <Arduino.h>

// Browser-side Wi-Fi provisioning over the USB-CDC port. Listens whenever a
// host has the CDC port open — i.e. a computer is on the other end of the
// cable. Wall chargers and disconnected cables register as "no host" and we
// stay idle, so there's no battery cost when nobody's around to provision.
//
// On a successful provisioning the saved credentials get persisted via
// [[wifi-creds]] so the next upload-mode entry can use STA.
//
// "Listening" just means calling `loop()` from the main loop; the underlying
// library is only polled when `Serial` is truthy. The protocol is driven
// byte-by-byte on each call — no thread, no interrupt.
//
// Two flavours of listening — only matters for Wi-Fi ownership accounting:
//   * Outside upload mode: we may bring Wi-Fi up to verify creds and are
//     responsible for tearing it down again ~2s after a successful provision.
//   * Inside upload mode: Wi-Fi is owned by the upload session; we just
//     persist creds and don't touch Wi-Fi state. Upload screen toggles
//     this via notifyUploadSession().
//
// Underlying protocol: Improv Serial (https://www.improv-wifi.com). The
// chip-side library is jnthas/Improv-WiFi-Library, pinned in platformio.ini.
// "Improv" is the protocol name — this namespace wraps it under a more
// self-explanatory name for callers in the rest of the firmware.
namespace WifiProvisioning {

void begin();                                // setup() — register lib callbacks
void loop();                                 // main loop — gated internally by Serial
bool isActive();                             // true while listening (for lightsleep gate)

// upload_screen marks the session boundaries so we know to keep our hands
// off Wi-Fi while a session is up.
void notifyUploadSession(bool active);

}  // namespace WifiProvisioning

#endif  // PALA_HAL_WIFI_PROVISIONING_H
