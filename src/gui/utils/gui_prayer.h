#ifndef MUSLIMTIFY_GUI_PRAYER_H
#define MUSLIMTIFY_GUI_PRAYER_H

#include "prayer_helper.h"

#define GUI_PRAYER_COUNT 7

typedef struct {
  char name[16];
  char time[16];
  bool next;
  bool enabled;
} TodayPrayer;

// Compute today's prayer snapshot into the module-static cache. Quiet: falls
// back to a default-config snapshot if config/location are unavailable.
void guiLoadPrayer(void);

// Borrow the cached snapshot. Valid until the next guiLoadPrayer() call.
const PrayerSnapshot *guiGetPrayer(void);

// Fill `out` with GUI_PRAYER_COUNT prayers (time-ordered) from `snap`.
void guiTodayPrayer(const PrayerSnapshot *snap, TodayPrayer out[GUI_PRAYER_COUNT]);

#endif // MUSLIMTIFY_GUI_PRAYER_H
