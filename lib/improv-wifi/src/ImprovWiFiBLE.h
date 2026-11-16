#pragma once

// Only compile BLE support if explicitly enabled
#ifdef IMPROV_WIFI_BLE_ENABLED

#include "ImprovTypes.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif

#include <NimBLEDevice.h>
#include <functional>
#include <vector>

/**
 * Improv WiFi BLE class
 *
 * Public API matches ImprovWiFi (Serial) so both transports can be used
 * interchangeably from the sketch.
 *
 * Usage example:
 *   ImprovWiFiBLE improvBLE;
 *   improvBLE.setDeviceInfo(...);
 *   improvBLE.onImprovError(...);
 *   improvBLE.onImprovConnected(...);
 *   improvBLE.setCustomConnectWiFi(...);
 */
class ImprovWiFiBLE : public NimBLECharacteristicCallbacks,
                      public NimBLEServerCallbacks {
public:
  /**
   * ## Type definitions (same as ImprovWiFi)
   */
  typedef void(OnImprovError)(ImprovTypes::Error);
  typedef void(OnImprovConnected)(const char *ssid, const char *password);
  typedef bool(CustomConnectWiFi)(const char *ssid, const char *password);

  /**
   * ## Constructors
   */
  ImprovWiFiBLE() = default;
  ~ImprovWiFiBLE();

  /**
   * ## Methods (names/signatures match ImprovWiFi)
   */

  // Set details of your device (advertising is set up inside this call)
  void setDeviceInfo(ImprovTypes::ChipFamily chipFamily,
                     const char *firmwareName, const char *firmwareVersion,
                     const char *deviceName, const char *deviceUrl);

  void setDeviceInfo(ImprovTypes::ChipFamily chipFamily,
                     const char *firmwareName, const char *firmwareVersion,
                     const char *deviceName);

  // Callback setters
  void onImprovError(OnImprovError *errorCallback);
  void onImprovConnected(OnImprovConnected *connectedCallback);
  void setCustomConnectWiFi(CustomConnectWiFi *connectWiFiCallBack);

  // Default WiFi connect helper (same name as serial transport)
  bool tryConnectToWifi(const char *ssid, const char *password);
  bool tryConnectToWifi(const char *ssid, const char *password,
                        uint32_t delayMs, uint8_t maxAttempts);

  // Mirror of ImprovWiFi::isConnected
  bool isConnected();

  // NimBLEServerCallbacks
  void onDisconnect(NimBLEServer *s);

  // NimBLECharacteristicCallbacks
  void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &info) override;

private:
  // === Improv BLE UUIDs ===
  static constexpr const char *SVC_UUID =
      "00467768-6228-2272-4663-277478268000";
  static constexpr const char *CHAR_STATE_UUID =
      "00467768-6228-2272-4663-277478268001";
  static constexpr const char *CHAR_ERROR_UUID =
      "00467768-6228-2272-4663-277478268002";
  static constexpr const char *CHAR_RPC_CMD_UUID =
      "00467768-6228-2272-4663-277478268003";
  static constexpr const char *CHAR_RPC_RES_UUID =
      "00467768-6228-2272-4663-277478268004";
  static constexpr const char *CHAR_CAPS_UUID =
      "00467768-6228-2272-4663-277478268005";
  static constexpr uint16_t SERVICE_DATA_UUID_16 =
      0x4677; // for advertisement service data

  enum : uint8_t {
    STATE_AUTH_REQUIRED = 0x01,
    STATE_AUTHORIZED = 0x02,
    STATE_PROVISIONING = 0x03,
    STATE_PROVISIONED = 0x04
  };

  // Basic error mapping for BLE-side status byte
  enum : uint8_t {
    ERR_NONE = 0x00,
    ERR_BAD_PACKET = 0x01,
    ERR_UNKNOWN_CMD = 0x02,
    ERR_CONNECT = 0x03,
    ERR_NOT_AUTH = 0x04,
    ERR_UNKNOWN = 0xFF
  };

  // Internal helpers
  void updateState(uint8_t s);
  void updateError(uint8_t e);
  void updateCaps(uint8_t caps);

  // Build advertisement payload (Flags + 128-bit UUID + Service Data)
  NimBLEAdvertisementData buildAdvData(uint8_t state, uint8_t caps);

  // (Re)apply adv data that reflects current state/caps; keeps scan response
  void advertiseNow();

  static uint8_t checksumLSB(const uint8_t *data, size_t len);

  // RPC handlers
  void handleRpc(const uint8_t *data, size_t len);
  void rpcSendWifi(const uint8_t *payload, size_t n);
  void rpcIdentify();

  // send response with device URL (mirrors serial transport semantics)
  void sendDeviceUrl();

  // BLE objects
  NimBLEServer *server_{nullptr};
  NimBLEService *service_{nullptr};
  NimBLECharacteristic *ch_state_{nullptr};
  NimBLECharacteristic *ch_error_{nullptr};
  NimBLECharacteristic *ch_rpc_cmd_{nullptr};
  NimBLECharacteristic *ch_rpc_res_{nullptr};
  NimBLECharacteristic *ch_caps_{nullptr};
  NimBLEAdvertising *adv_{nullptr};

  // identity
  ImprovTypes::ChipFamily chip_{ImprovTypes::ChipFamily::CF_ESP32};
  String firmware_name_;
  String firmware_version_;
  String device_name_;
  String device_friendly_name_;
  String device_url_;

  // state
  uint8_t state_{STATE_AUTHORIZED};
  uint8_t error_{ERR_NONE};
  uint8_t caps_{0x01}; // bit0: Identify supported

  // user callbacks
  OnImprovError *onImprovErrorCallback_{nullptr};
  OnImprovConnected *onImprovConnectedCallback_{nullptr};
  CustomConnectWiFi *customConnectWiFiCallback_{nullptr};
};

#endif // IMPROV_WIFI_BLE_ENABLED