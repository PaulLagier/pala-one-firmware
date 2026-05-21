#include "../../Pala_One_2_1/pala_app.h"
#include "../../Pala_One_2_1/pala_api.h"

__attribute__((section(".header")))
const PalaAppHeader pala_header = {
    .magic        = PALA_APP_MAGIC,
    .api_version  = PALA_API_VERSION,
    .name         = "Sleep Timer",
    .entry_offset = 0,
    .reloc_offset = 0,
    .reloc_count  = 0,
};

#define LONG_PRESS_MS  850u

/* Status values are part of the wire format the firmware reads from
   /apps/sleep_timer.dat. The firmware checks the file after each reader
   page turn and shows a "time's up" toast when ST_RUNNING has elapsed.
   Field order in SleepTimerFile is frozen — do not reorder. */
#define ST_IDLE          0
#define ST_RUNNING       1
#define ST_NEEDS_NOTIFY  2

typedef struct {
    uint32_t version;        /* schema version, currently 1 */
    uint32_t status;         /* ST_IDLE / ST_RUNNING / ST_NEEDS_NOTIFY */
    uint32_t endRtcSec;      /* rtcSeconds() value when timer should fire */
    uint32_t durationMin;    /* original duration; remembered across runs */
    uint32_t startedRtcSec;  /* rtcSeconds() at start of current run */
} SleepTimerFile;

static const uint32_t DURATIONS[] = {5, 10, 15, 20, 25, 30, 45, 60};
#define NUM_DURATIONS         ((int)(sizeof(DURATIONS) / sizeof(DURATIONS[0])))
#define DEFAULT_DURATION_IDX  4    /* 25 min — common reading session length */

static void drawIdle(const PalaAPI* api, uint32_t durationMin) {
    char buf[16];
    api->clearScreen();
    api->drawHeader("Sleep Timer");
    api->drawTextAt(78, 34, "Set duration", 0);
    api->snprintf_wrap(buf, sizeof(buf), "%u min", (unsigned)durationMin);
    api->drawCenteredLarge(buf);
    api->drawTextAt(8, 118, "Click=cycle  2x=start  Hold=exit", 0);
    api->refreshDisplay();
}

static void drawRunning(const PalaAPI* api, uint32_t remainingSec, uint32_t durationMin) {
    char buf[24];
    api->clearScreen();
    api->drawHeader("Sleep Timer");

    uint32_t mins = remainingSec / 60u;
    uint32_t secs = remainingSec - (mins * 60u);
    api->snprintf_wrap(buf, sizeof(buf), "%u:%02u", (unsigned)mins, (unsigned)secs);
    api->drawCenteredLarge(buf);

    api->snprintf_wrap(buf, sizeof(buf), "%u-min timer", (unsigned)durationMin);
    api->drawTextAt(82, 108, buf, 0);
    api->drawTextAt(8, 118, "Click=cancel  Hold=exit (keeps running)", 0);
    api->refreshDisplay();
}

static void drawNotify(const PalaAPI* api, uint32_t durationMin) {
    char buf[24];
    api->clearScreen();
    api->drawHeader("Sleep Timer");
    api->drawCenteredLarge("Time's up!");
    api->snprintf_wrap(buf, sizeof(buf), "(%u-min timer)", (unsigned)durationMin);
    api->drawTextAt(82, 108, buf, 0);
    api->drawTextAt(38, 118, "Click=dismiss   Hold=exit", 0);
    api->refreshDisplay();
}

void app_main(const PalaAPI* api) {
    SleepTimerFile s;
    int hasSave = (api->storageRead("sleep_timer", &s, sizeof(s)) == (int)sizeof(s));
    uint32_t now = api->rtcSeconds();

    if (!hasSave || s.version != 1) {
        s.version       = 1;
        s.status        = ST_IDLE;
        s.endRtcSec     = 0;
        s.durationMin   = DURATIONS[DEFAULT_DURATION_IDX];
        s.startedRtcSec = 0;
    }

    /* If a stored timer is running but already past its end, treat as expired. */
    if (s.status == ST_RUNNING && now >= s.endRtcSec) {
        s.status = ST_NEEDS_NOTIFY;
    }

    /* Find current duration index for cycling. Defaults to 25 min if unmatched. */
    int durIdx = DEFAULT_DURATION_IDX;
    for (int i = 0; i < NUM_DURATIONS; i++) {
        if (DURATIONS[i] == s.durationMin) { durIdx = i; break; }
    }

    api->storageWrite("sleep_timer", &s, sizeof(s));

    int needsRedraw       = 1;
    uint32_t pressStart   = 0;
    uint32_t lastDispSec  = 0;

    while (1) {
        uint32_t mNow = api->millisNow();
        now = api->rtcSeconds();

        /* Long-press exit. Timer state is preserved on disk; if it's running,
           the firmware will still show the toast when it expires. */
        if (api->buttonPressed()) {
            if (pressStart == 0) pressStart = mNow;
            if ((mNow - pressStart) >= LONG_PRESS_MS) {
                api->storageWrite("sleep_timer", &s, sizeof(s));
                return;
            }
        } else {
            pressStart = 0;
        }

        /* In-app expiration detection (when user is watching the countdown). */
        if (s.status == ST_RUNNING && now >= s.endRtcSec) {
            s.status = ST_NEEDS_NOTIFY;
            api->storageWrite("sleep_timer", &s, sizeof(s));
            needsRedraw = 1;
        }

        uint8_t evt = api->pollEvent();

        if (s.status == ST_IDLE) {
            if (evt == PALA_CLICK) {
                durIdx += 1;
                if (durIdx >= NUM_DURATIONS) durIdx = 0;
                s.durationMin = DURATIONS[durIdx];
                needsRedraw = 1;
            } else if (evt == PALA_DOUBLE) {
                s.status        = ST_RUNNING;
                s.startedRtcSec = now;
                s.endRtcSec     = now + (s.durationMin * 60u);
                api->storageWrite("sleep_timer", &s, sizeof(s));
                needsRedraw = 1;
            }
        } else if (s.status == ST_RUNNING) {
            if (evt == PALA_CLICK) {
                /* Cancel and return to idle. */
                s.status        = ST_IDLE;
                s.endRtcSec     = 0;
                s.startedRtcSec = 0;
                api->storageWrite("sleep_timer", &s, sizeof(s));
                needsRedraw = 1;
            }
            /* Tick the seconds display once per real second. */
            if (now != lastDispSec) {
                lastDispSec = now;
                needsRedraw = 1;
            }
        } else { /* ST_NEEDS_NOTIFY */
            if (evt != 0) {
                s.status        = ST_IDLE;
                s.endRtcSec     = 0;
                s.startedRtcSec = 0;
                api->storageWrite("sleep_timer", &s, sizeof(s));
                needsRedraw = 1;
            }
        }

        if (needsRedraw) {
            if (s.status == ST_IDLE) {
                drawIdle(api, s.durationMin);
            } else if (s.status == ST_RUNNING) {
                uint32_t remaining = (now < s.endRtcSec) ? (s.endRtcSec - now) : 0;
                drawRunning(api, remaining, s.durationMin);
            } else {
                drawNotify(api, s.durationMin);
            }
            needsRedraw = 0;
        }

        api->delayMs(50);
    }
}
