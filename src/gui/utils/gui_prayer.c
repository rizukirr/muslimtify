#include "utils/gui_prayer.h"

#include "config.h"
#include "platform.h"

#include <stdio.h>
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

void guiTodayPrayer(const PrayerSnapshot *snap, TodayPrayer out[GUI_PRAYER_COUNT]) {
  const struct PrayerTimes *times = &snap->times;
  const Config *cfg = &snap->config;

  const char *prayer_names[] = {"Fajr", "Sunrise", "Dhuha", "Dhuhr", "Asr", "Maghrib", "Isha"};
  PrayerType types[] = {PRAYER_FAJR, PRAYER_SUNRISE, PRAYER_DHUHA, PRAYER_DHUHR,
                        PRAYER_ASR,  PRAYER_MAGHRIB, PRAYER_ISHA};

  for (int i = 0; i < GUI_PRAYER_COUNT; i++) {
    const PrayerConfig *pcfg = prayer_get_config(cfg, types[i]);

    double prayer_time = prayer_get_time(times, types[i]);
    format_time_hm(prayer_time, out[i].time, sizeof(out[i].time));

    snprintf(out[i].name, sizeof(out[i].name), "%s", prayer_names[i]);
    // Next prayer comes from the snapshot — single source of truth.
    out[i].next = (snap->next != PRAYER_NONE && types[i] == snap->next);
    out[i].enabled = pcfg->enabled;
  }
}
