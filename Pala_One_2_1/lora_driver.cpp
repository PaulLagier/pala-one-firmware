#include "lora_driver.h"

#include <Arduino.h>
#include <RadioLib.h>

// ── SX1262 pin assignments (Heltec Wireless Paper, WirelessPaper.h:41-47) ────
#define LORA_NSS   8
#define LORA_DIO1  14
#define LORA_RST   12
#define LORA_BUSY  13
#define LORA_SCK   9
#define LORA_MISO  11
#define LORA_MOSI  10

// ── Raw receive queue ─────────────────────────────────────────────────────────
#define LORA_RX_QUEUE   4
#define LORA_PKT_MAX    250

struct LoraRxEntry {
    uint8_t buf[LORA_PKT_MAX];
    int     len;
};
static LoraRxEntry s_rxQueue[LORA_RX_QUEUE];
static int s_rxHead = 0;
static int s_rxTail = 0;

// ── Hardware objects — single owner (this TU) ────────────────────────────────
static SPIClass      s_spi(HSPI);
static SX1262        s_radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY, s_spi);
static bool          s_ready = false;
static volatile bool s_rxFlag = false;

static void IRAM_ATTR loraRxIsr() { s_rxFlag = true; }

uint32_t loraNodeId() {
    return (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF);
}

int loraInit(float freq_mhz, float bw_khz, int sf, int cr,
             uint8_t sync_word, int tx_power, int preamble, float tcxo_v) {
    s_spi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    int state = s_radio.begin(freq_mhz, bw_khz, sf, cr, sync_word, tx_power, preamble, tcxo_v);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LORA] Init failed: %d\n", state);
        return state;
    }
    s_radio.setDio1Action(loraRxIsr);
    state = s_radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LORA] startReceive failed: %d\n", state);
        return state;
    }
    s_ready = true;
    Serial.printf("[LORA] Ready — %.4f MHz SF%d BW%.0f node=!%08lx\n",
                  freq_mhz, sf, bw_khz, (unsigned long)loraNodeId());
    return 0;
}

void loraSleep() {
    if (s_ready) {
        s_radio.sleep(false);
        s_ready = false;
    }
}

bool loraReady() {
    return s_ready;
}

// Drain ISR flag into receive queue.
// Limitation: the queue holds LORA_RX_QUEUE packets. If the app doesn't drain
// it fast enough and another packet arrives, the new one is dropped silently
// at the radio level (we log it but cannot recover the bytes). The app polls
// loraRecv() each main-loop iteration so this is unlikely in practice.
static void loraProcessRx() {
    if (!s_ready || !s_rxFlag) return;
    s_rxFlag = false;
    size_t rxLen = s_radio.getPacketLength();
    if (rxLen >= 4 && rxLen <= LORA_PKT_MAX) {
        int nextHead = (s_rxHead + 1) % LORA_RX_QUEUE;
        if (nextHead != s_rxTail) {
            int state = s_radio.readData(s_rxQueue[s_rxHead].buf, rxLen);
            if (state == RADIOLIB_ERR_NONE) {
                s_rxQueue[s_rxHead].len = (int)rxLen;
                s_rxHead = nextHead;
            }
        } else {
            Serial.printf("[LORA] RX queue full, packet dropped (%u bytes)\n",
                          (unsigned)rxLen);
        }
    }
    s_radio.startReceive();
}

int loraRecv(uint8_t* buf, int maxlen) {
    loraProcessRx();
    if (s_rxHead == s_rxTail) return 0;
    LoraRxEntry& e = s_rxQueue[s_rxTail];
    int n = (e.len < maxlen) ? e.len : maxlen;
    memcpy(buf, e.buf, n);
    s_rxTail = (s_rxTail + 1) % LORA_RX_QUEUE;
    return n;
}

void loraSend(const uint8_t* buf, int len) {
    if (!s_ready || len <= 0) return;
    Serial.printf("[LORA] TX %d bytes\n", len);
    s_radio.transmit(buf, (size_t)len);
    s_radio.startReceive();
}
