#ifndef PALA_HAL_WIFI_PROVISIONING_H
#define PALA_HAL_WIFI_PROVISIONING_H

#include <Arduino.h>

// Browser-side Improv Serial provisioning. USB commands keep a finite
// activity window awake; an idle USB cable/host does not inhibit sleep.
// Connection progress is cooperative and does not wait for association in
// the input loop. Credentials are saved only after a successful connection.
// Outside upload mode the wrapper releases its radio after success/failure;
// an upload screen retains ownership of its own radio.
namespace WifiProvisioning {

void beginLocalSetup(); // USB metadata and NVS settings only; no radio or book storage.
void loopLocalSetup();
void begin();    // setup() — register lib callbacks
void loop();     // main loop — gated internally by Serial
bool isActive(); // queued/recent host traffic, not mere cable attachment

// upload_screen marks the session boundaries so we know to keep our hands
// off Wi-Fi while a session is up.
void notifyUploadSession(bool active);

} // namespace WifiProvisioning

#endif // PALA_HAL_WIFI_PROVISIONING_H
