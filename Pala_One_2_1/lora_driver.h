// lora_driver.h — SX1262 LoRa hardware driver for Pala One
//
// This driver only talks to the radio hardware. It knows nothing about
// Meshtastic, packet formats, encryption, or any higher-level protocol.
// Protocol logic lives entirely in loadable apps (e.g. examples/mesh_chat/).
//
// What lives here and why it cannot be in an app:
//   SPIClass / SX1262  — C++ objects; apps are position-independent C
//   loraRxIsr          — must be in IRAM; a loaded binary cannot be registered as an IRQ handler
//   receive queue      — ISR fires asynchronously; bytes must land somewhere before the app polls
//   loraInit / loraSleep /
//   loraRecv / loraSend — thin wrappers around RadioLib; exposed to apps via PalaAPI v4
//
// Declarations only — the radio object and queue are file-private in
// lora_driver.cpp so multiple firmware TUs can call these without each
// getting its own private radio instance.
#pragma once

#include <stdint.h>

int      loraInit(float freq_mhz, float bw_khz, int sf, int cr,
                  uint8_t sync_word, int tx_power, int preamble, float tcxo_v);
void     loraSleep();
bool     loraReady();
int      loraRecv(uint8_t* buf, int maxlen);
void     loraSend(const uint8_t* buf, int len);
uint32_t loraNodeId();
