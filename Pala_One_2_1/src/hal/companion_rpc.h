#pragma once
#include <Arduino.h>
void companionRpcReset();
String companionRpcControl(const uint8_t *bytes, size_t length);
String companionRpcChunk(const uint8_t *bytes, size_t length);
