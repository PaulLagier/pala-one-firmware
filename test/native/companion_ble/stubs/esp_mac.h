#pragma once
#include <stdint.h>
#include <cstring>
constexpr int ESP_MAC_WIFI_STA=0;
inline int esp_read_mac(uint8_t* bytes,int){const uint8_t mac[6]={2,0,0,0,0,1};std::memcpy(bytes,mac,6);return 0;}
