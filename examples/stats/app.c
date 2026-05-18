#include "../../Pala_One_2_1/pala_app.h"
#include "../../Pala_One_2_1/pala_api.h"

__attribute__((section(".header")))
const PalaAppHeader pala_header = {
    .magic        = PALA_APP_MAGIC,
    .api_version  = PALA_API_VERSION,
    .name         = "Stats",
    .entry_offset = 0,  // patched by Makefile
    .reloc_offset = 0,  // patched by Makefile
    .reloc_count  = 0,  // patched by Makefile
};

#define LONG_PRESS_MS 850
#define STATS_SCHEMA  1u

// Must match StatsFile in Pala_One_2_1.ino byte-for-byte.
typedef struct {
    uint32_t version;
    uint32_t firstRtcSec;
    uint64_t pagesRead;
    uint64_t buttonPresses;
} StatsFile;

// snprintf in the app runtime may not advertise %llu support; render
// values >= 2^32 as a two-segment decimal to avoid relying on it.
static void formatU64(const PalaAPI* api, char* buf, int len, uint64_t v) {
    if (v <= 0xFFFFFFFFu) {
        api->snprintf_wrap(buf, len, "%lu", (unsigned long)v);
        return;
    }
    unsigned long hi = (unsigned long)(v / 1000000000ULL);
    unsigned long lo = (unsigned long)(v % 1000000000ULL);
    api->snprintf_wrap(buf, len, "%lu%09lu", hi, lo);
}

static void drawPage(const PalaAPI* api) {
    StatsFile s;
    int n = api->storageRead("stats", &s, (int)sizeof(s));
    int valid = (n == (int)sizeof(s) && s.version == STATS_SCHEMA);

    api->clearScreen();
    api->drawHeader("Stats");

    char num[32];

    api->drawTextAt(6, 36, "Pages read", 1);
    if (valid) formatU64(api, num, sizeof(num), s.pagesRead);
    else       api->snprintf_wrap(num, sizeof(num), "--");
    api->drawTextAt(6, 50, num, 0);

    api->drawTextAt(6, 70, "Button presses", 1);
    if (valid) formatU64(api, num, sizeof(num), s.buttonPresses);
    else       api->snprintf_wrap(num, sizeof(num), "--");
    api->drawTextAt(6, 84, num, 0);

    api->drawTextAt(6, 116, "hold to exit", 0);
    api->refreshDisplay();
}

void app_main(const PalaAPI* api) {
    uint32_t pressStart = 0;

    drawPage(api);

    while (1) {
        // Drive the input frontend so btns.poll() ticks.
        (void)api->pendingPresses();

        if (api->buttonPressed()) {
            if (pressStart == 0) pressStart = api->millisNow();
            if ((api->millisNow() - pressStart) >= LONG_PRESS_MS) return;
        } else {
            pressStart = 0;
        }

        api->delayMs(10);
    }
}
