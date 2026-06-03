#include "src/pure/reading_time_log.h"

ReadingTimeFile applyReadingTime(const ReadingTimeFile& cur, uint32_t day, uint32_t seconds) {
  ReadingTimeFile f = cur;
  if (seconds == 0) return f;

  uint32_t week  = day / RTIME_DAYS_PER_WEEK;
  uint32_t month = day / RTIME_DAYS_PER_MONTH;
  uint32_t year  = day / RTIME_DAYS_PER_YEAR;

  // Roll over any bucket that has moved on since its last write.
  if (f.dayIndex   != day)   { f.dayIndex   = day;   f.daySeconds   = 0; }
  if (f.weekIndex  != week)  { f.weekIndex  = week;  f.weekSeconds  = 0; }
  if (f.monthIndex != month) { f.monthIndex = month; f.monthSeconds = 0; }
  if (f.yearIndex  != year)  { f.yearIndex  = year;  f.yearSeconds  = 0; }

  f.daySeconds   += seconds;
  f.weekSeconds  += seconds;
  f.monthSeconds += seconds;
  f.yearSeconds  += seconds;
  f.totalSeconds += seconds;
  return f;
}

ReadingTimeView viewReadingTime(const ReadingTimeFile& f, uint32_t curDay) {
  ReadingTimeView v{};
  v.total = f.totalSeconds;
  v.today = (f.dayIndex   == curDay)                        ? f.daySeconds   : 0;
  v.week  = (f.weekIndex  == curDay / RTIME_DAYS_PER_WEEK)  ? f.weekSeconds  : 0;
  v.month = (f.monthIndex == curDay / RTIME_DAYS_PER_MONTH) ? f.monthSeconds : 0;
  v.year  = (f.yearIndex  == curDay / RTIME_DAYS_PER_YEAR)  ? f.yearSeconds  : 0;

  // curDay is 0-based, so the epoch day counts as 1 elapsed day.
  uint32_t daysElapsed  = curDay + 1;
  uint32_t weeksElapsed = curDay / RTIME_DAYS_PER_WEEK + 1;
  v.avgPerDay  = (uint32_t)(f.totalSeconds / daysElapsed);
  v.avgPerWeek = (uint32_t)(f.totalSeconds / weeksElapsed);
  return v;
}
