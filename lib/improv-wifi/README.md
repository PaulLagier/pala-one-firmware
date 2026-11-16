# Improv WiFi Library

Improv is a free and open standard with ready-made SDKs that offer a great user experience to configure Wi-Fi on devices. More details [here](https://www.improv-wifi.com/).

This library provides an easier way to handle the Improv serial protocol in your firmware, allowing you to configure the WiFi via web browser as in the following [example](https://jnthas.github.io/improv-wifi-demo/).

Simplest use example:

```cpp
#include <ImprovWiFiLibrary.h>

ImprovWiFi improvSerial(&Serial);

void setup() {
  improvSerial.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32, "My-Device-9a4c2b", "2.1.5", "My Device");
}

void loop() { 
  improvSerial.handleSerial();
}
```

## Bluetooth Support (Optional)

You can also use Improv over Bluetooth (uses about 25kb memory). BLE support is **optional** and must be explicitly enabled.

### Enabling BLE Support

To enable BLE functionality, you need to:

1. **Add the build flag** to your `platformio.ini`:
   ```ini
   build_flags =
       -DIMPROV_WIFI_BLE_ENABLED
   ```

2. **Add the NimBLE-Arduino dependency** to your `platformio.ini`:
   ```ini
   lib_deps =
       h2zero/NimBLE-Arduino@^2.3.7
   ```

**Note:** BLE is not supported on ESP32-S2 and ESP32-C2 as these chips lack Bluetooth hardware. Only enable BLE for chips that support it (ESP32, ESP32-C3, ESP32-S3).

### Example with BLE enabled:

```cpp
#include <ImprovWiFiBLE.h>

ImprovWiFiBLE improvBLE;

void setup() {
  improvBLE.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32, "My-Device-9a4c2b", "2.1.5", "My Device");
}

void loop() {
}
```

### Using both Serial and BLE:

```cpp
#include <ImprovWiFiLibrary.h>
#ifdef IMPROV_WIFI_BLE_ENABLED
#include <ImprovWiFiBLE.h>
#endif

ImprovWiFi improvSerial(&Serial);
#ifdef IMPROV_WIFI_BLE_ENABLED
ImprovWiFiBLE improvBLE;
#endif

void setup() {
  improvSerial.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32, "My-Device-9a4c2b", "2.1.5", "My Device");
  #ifdef IMPROV_WIFI_BLE_ENABLED
  improvBLE.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32, "My-Device-9a4c2b", "2.1.5", "My Device");
  #endif
}

void loop() {
  improvSerial.handleSerial();
}
```

## Documentation

The full library documentation can be seen in [docs/](docs/ImprovWiFiLibrary.md) folder.


## License

This open source code is licensed under the MIT license (see [LICENSE](LICENSE)
for details).