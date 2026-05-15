#include "../../Pala_One_2_1/pala_app.h"
#include "../../Pala_One_2_1/pala_api.h"

__attribute__((section(".header")))
const PalaAppHeader pala_header = {
    .magic        = PALA_APP_MAGIC,
    .api_version  = PALA_API_VERSION,
    .name         = "Reading Streak",
    .entry_offset = 0,
    .reloc_offset = 0,
    .reloc_count  = 0,
};

#define LONG_PRESS_MS    850u
#define CONFIRM_SHOW_MS  1500u
#define DAY_SECS         86400u
#define HISTORY_CELLS    30
#define BITMAP_BITS      32
#define UNSET_DAY        0xFFFFFFFFu

/* Persisted state.  All uint32_t to keep alignment portable across rebuilds. */
typedef struct {
    uint32_t version;        /* schema version, currently 1 */
    uint32_t firstRtcSec;    /* rtcSeconds() the very first time app ran */
    uint32_t lastLoggedDay;  /* day index of most recent log, UNSET_DAY if never */
    uint32_t currentStreak;
    uint32_t longestStreak;
    uint32_t totalSessions;
    uint32_t bitmapHead;     
    uint32_t bitmap;        
} SavedState;

static void drawStreakBar(const PalaAPI* api, int y, uint32_t bitmap) {
    const int cellW  = 8;
    const int totalW = HISTORY_CELLS * cellW;
    const int startX = (250 - totalW) / 2;
    char buf[2];
    buf[1] = 0;
    for (int p = 0; p < HISTORY_CELLS; p++) {
        int bit  = (HISTORY_CELLS - 1) - p;
        int bold = (p == HISTORY_CELLS - 1) ? 1 : 0;
        buf[0] = ((bitmap >> bit) & 1u) ? '#' : '.';
        api->drawTextAt(startX + p * cellW, y, buf, bold);
    }
}

static void drawMain(const PalaAPI* api, const SavedState* s, int loggedToday) {
    char buf[40];

    api->clearScreen();
    api->drawHeader("Reading Streak");

    api->snprintf_wrap(buf, sizeof(buf), "Current streak: %u days",
                       (unsigned)s->currentStreak);
    api->drawTextAt(6, 34, buf, 1);

    api->snprintf_wrap(buf, sizeof(buf), "Longest: %u",
                       (unsigned)s->longestStreak);
    api->drawTextAt(6, 48, buf, 0);

    api->snprintf_wrap(buf, sizeof(buf), "Total: %u sessions",
                       (unsigned)s->totalSessions);
    api->drawTextAt(6, 62, buf, 0);

    api->drawTextAt(6, 80, "Last 30 days (today on right):", 0);
    drawStreakBar(api, 96, s->bitmap);

    if (loggedToday) {
        api->drawTextAt(6, 118, "Logged today.  Hold = exit", 0);
    } else {
        api->drawTextAt(6, 118, "Click = log today   Hold = exit", 1);
    }

    api->refreshDisplay();
}

static void drawConfirm(const PalaAPI* api, uint32_t streak) {
    char buf[16];
    api->clearScreen();
    api->drawHeader("Reading Streak");
    api->drawTextAt(95, 36, "Logged!", 1);
    api->snprintf_wrap(buf, sizeof(buf), "%u", (unsigned)streak);
    api->drawCenteredLarge(buf);
    api->drawTextAt(85, 116, "day streak", 0);
    api->refreshDisplay();
}

void app_main(const PalaAPI* api) {
    SavedState s;
    int hasSave = (api->storageRead("streak", &s, sizeof(s)) == (int)sizeof(s));
    uint32_t now = api->rtcSeconds();

    if (!hasSave || s.version != 1) {
        s.version        = 1;
        s.firstRtcSec    = now;
        s.lastLoggedDay  = UNSET_DAY;
        s.currentStreak  = 0;
        s.longestStreak  = 0;
        s.totalSessions  = 0;
        s.bitmapHead     = 0;
        s.bitmap         = 0;
    }

    /* If RTC went backwards reset the day baseline but keep cumulative stats so the user doesn't lose their record. */
    if (now < s.firstRtcSec) {
        s.firstRtcSec   = now;
        s.lastLoggedDay = UNSET_DAY;
        s.currentStreak = 0;
        s.bitmapHead    = 0;
        /* keep bitmap, longestStreak, totalSessions */
    }

    uint32_t today = (now - s.firstRtcSec) / DAY_SECS;

    /* Slide the bitmap forward so bit 0 represents today. */
    if (today > s.bitmapHead) {
        uint32_t delta = today - s.bitmapHead;
        if (delta >= BITMAP_BITS) s.bitmap = 0;
        else                      s.bitmap <<= delta;
        s.bitmapHead = today;
    }

    /* Streak is broken if there's a gap of more than one day since last log. */
    if (s.lastLoggedDay != UNSET_DAY && today > s.lastLoggedDay + 1) {
        s.currentStreak = 0;
    }

    /* Persist the normalized state so future opens see consistent values. */
    api->storageWrite("streak", &s, sizeof(s));

    int loggedToday      = (s.lastLoggedDay == today) ? 1 : 0;
    int needsRedraw      = 1;
    int showingConfirm   = 0;
    uint32_t confirmStart = 0;
    uint32_t pressStart   = 0;

    while (1) {
        uint32_t mNow = api->millisNow();

        /* Long - press exit, fires while held(same pattern as click_counter)*/
        if (api->buttonPressed()) {
            if (pressStart == 0) pressStart = mNow;
            if ((mNow - pressStart) >= LONG_PRESS_MS) {
                api->storageWrite("streak", &s, sizeof(s));
                return;
            }
        } else {
            pressStart = 0;
        }

        /* pendingPresses() bypasses multi-click grouping */
        uint32_t presses = api->pendingPresses();
        if (presses > 0 && !loggedToday) {
            int isContinuation = (s.lastLoggedDay != UNSET_DAY) &&
                                 (today == s.lastLoggedDay + 1);
            s.bitmap        |= 1u;
            s.currentStreak  = isContinuation ? (s.currentStreak + 1) : 1;
            if (s.currentStreak > s.longestStreak) s.longestStreak = s.currentStreak;
            s.lastLoggedDay  = today;
            s.totalSessions += 1;
            loggedToday      = 1;
            api->storageWrite("streak", &s, sizeof(s));

            showingConfirm = 1;
            confirmStart   = mNow;
            drawConfirm(api, s.currentStreak);
        }

        if (showingConfirm && (mNow - confirmStart) >= CONFIRM_SHOW_MS) {
            showingConfirm = 0;
            needsRedraw    = 1;
        }

        if (needsRedraw && !showingConfirm) {
            drawMain(api, &s, loggedToday);
            needsRedraw = 0;
        }

        api->delayMs(10);
    }
}
