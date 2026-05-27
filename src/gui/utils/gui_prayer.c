#include "utils/gui_prayer.h"

#include "config.h"
#include "platform.h"

#include <time.h>

static PrayerSnapshot g_guiPrayer;

const PrayerSnapshot *guiGetPrayer(void) {
  prayer_helper_refresh_minute_until(&g_guiPrayer);
  return &g_guiPrayer;
}

void guiLoadPrayer(void) {
  if (prayer_helper_today(&g_guiPrayer) != PRAYER_HELPER_OK) {
    // Mirror gui_config's default-fallback so the UI always has data to show.
    Config def = config_default();
    time_t now = time(NULL);
    struct tm tm_buf;
    platform_localtime(&now, &tm_buf);
    prayer_helper_compute(&def, &tm_buf, &g_guiPrayer);
  }
}
