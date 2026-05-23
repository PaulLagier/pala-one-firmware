#pragma once
#include <stdint.h>
#include <stdarg.h>

// Firmware-to-app API v1 — field order is frozen once shipped.
// To extend, append new fields and bump PALA_API_VERSION in pala_app.h.
typedef struct {
    void     (*clearScreen)(void);
    void     (*drawHeader)(const char* title);
    void     (*drawTextAt)(int x, int y, const char* text, int bold);
    void     (*drawCenteredLarge)(const char* text);
    void     (*refreshDisplay)(void);
    uint8_t  (*waitForEvent)(void);       // blocks until a gesture; returns PALA_CLICK/DOUBLE/TRIPLE/LONG
    int      (*snprintf_wrap)(char* buf, int len, const char* fmt, ...);
    uint8_t  (*pollEvent)(void);          // non-blocking; returns 0 if no event ready
    uint32_t (*millisNow)(void);          // current time in milliseconds
    int      (*buttonPressed)(void);      // 1 if button currently held, 0 otherwise
    void     (*delayMs)(uint32_t ms);     // yield for ms milliseconds
    uint32_t (*pendingPresses)(void);     // count of individual short press-release events since last call; bypasses multi-click grouping
    int      (*storageRead) (const char* key, void* buf, int maxlen);        // read from /apps/{key}.dat; returns bytes read, -1 on error
    int      (*storageWrite)(const char* key, const void* buf, int len);     // write to /apps/{key}.dat; returns bytes written, -1 on error
    uint32_t (*rtcSeconds)  (void);                                          // monotonic seconds; survives deep sleep; use for cross-session timing
    // v4 additions — always appended; never insert above this line
    int      (*loraInit)    (float freq_mhz, float bw_khz, int sf, int cr,
                             uint8_t sync_word, int tx_power, int preamble,
                             float tcxo_v);                                  // configure and start the SX1262; returns 0 on success
    void     (*loraSleep)   (void);                                          // put radio to sleep (call before app exits)
    int      (*loraReady)   (void);                                          // 1 if radio is initialised and listening
    int      (*loraRecv)    (uint8_t* buf, int maxlen);                      // raw packet received; returns byte count, 0 if none
    void     (*loraSend)    (const uint8_t* buf, int len);                   // transmit raw LoRa packet
    uint32_t (*loraNodeId)  (void);                                          // this device's 32-bit node ID (from chip MAC)
} PalaAPI;
