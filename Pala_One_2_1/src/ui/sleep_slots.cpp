#include "src/ui/sleep_slots.h"

#include "src/state.h"

String sleepSlotPath(int slot) {
  char buf[24];
  snprintf(buf, sizeof(buf), "/sleep-slot-%d.bin", slot);
  return String(buf);
}

int collectSleepSlots(int outSlots[MAX_MULTI_SLEEP_SLOTS]) {
  int c = 0;
  for (int i = 0; i < MAX_MULTI_SLEEP_SLOTS; i++) {
    if (FS.exists(sleepSlotPath(i))) {
      if (outSlots && c < MAX_MULTI_SLEEP_SLOTS) outSlots[c] = i;
      c++;
    }
  }
  return c;
}

int findNextFreeSleepSlot() {
  for (int i = 0; i < MAX_MULTI_SLEEP_SLOTS; i++) {
    if (!FS.exists(sleepSlotPath(i))) return i;
  }
  return -1;
}

int parseSleepSlotArg(const String& argValue) {
  if (argValue.length() == 0) return -1;
  int slot = argValue.toInt();
  if (slot < 0 || slot >= MAX_MULTI_SLEEP_SLOTS) return -1;
  return slot;
}

int loadSleepCycleIndex() {
  File f = FS.open("/sleep-cycle.idx", "r");
  if (!f) return 0;
  String s = f.readString();
  f.close();
  s.trim();
  if (s.length() == 0) return 0;
  int v = s.toInt();
  if (v < 0) v = 0;
  return v;
}

void saveSleepCycleIndex(int idx) {
  if (idx < 0) idx = 0;
  File f = FS.open("/sleep-cycle.idx", "w");
  if (!f) return;
  f.print(String(idx));
  f.close();
}

void resetSleepRotationState() {
  prefs.putUInt("cfg_sleep_ss_idx", 0);
  prefs.putInt("cfg_ss_last_slot", -1);
  saveSleepCycleIndex(0);
}

uint8_t reverseBits8(uint8_t b) {
  b = (uint8_t)(((b & 0xF0) >> 4) | ((b & 0x0F) << 4));
  b = (uint8_t)(((b & 0xCC) >> 2) | ((b & 0x33) << 2));
  b = (uint8_t)(((b & 0xAA) >> 1) | ((b & 0x55) << 1));
  return b;
}
