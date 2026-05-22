#ifndef PALA_UI_SLEEP_SLOTS_H
#define PALA_UI_SLEEP_SLOTS_H

#include <Arduino.h>

static const size_t SLEEP_FRAME_BYTES = 3904;
static const int MAX_SLEEP_FRAMES = 16;
static const int MAX_MULTI_SLEEP_SLOTS = 8;

String sleepSlotPath(int slot);
int collectSleepSlots(int outSlots[MAX_MULTI_SLEEP_SLOTS]);
int findNextFreeSleepSlot();
int parseSleepSlotArg(const String& argValue);
int loadSleepCycleIndex();
void saveSleepCycleIndex(int idx);
void resetSleepRotationState();
uint8_t reverseBits8(uint8_t b);

#endif  // PALA_UI_SLEEP_SLOTS_H
