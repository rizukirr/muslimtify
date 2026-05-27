#ifndef MUSLIMTIFY_GUI_PRAYER_H
#define MUSLIMTIFY_GUI_PRAYER_H

#include "prayer_helper.h"

// Compute today's prayer snapshot into the module-static cache. Quiet: falls
// back to a default-config snapshot if config/location are unavailable.
void guiLoadPrayer(void);

// Borrow the cached snapshot. Valid until the next guiLoadPrayer() call.
const PrayerSnapshot *guiGetPrayer(void);

#endif // MUSLIMTIFY_GUI_PRAYER_H
