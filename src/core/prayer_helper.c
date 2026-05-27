#include "prayer_helper.h"

#include "location.h"
#include "platform.h"

#include <time.h>

void prayer_helper_compute(const Config *cfg, const struct tm *now, PrayerSnapshot *out) {
  out->config = *cfg;
  out->date = *now;

  MethodParams params = method_params_from_config(&out->config);
  out->times = calculate_prayer_times(out->date.tm_year + 1900, out->date.tm_mon + 1,
                                      out->date.tm_mday, out->config.latitude,
                                      out->config.longitude, out->config.timezone_offset, &params);

  struct tm now_copy = out->date;
  out->minutes_until = 0;
  out->next = prayer_get_next(&out->config, &now_copy, &out->times, &out->minutes_until);
}

PrayerHelperStatus prayer_helper_today(PrayerSnapshot *out) {
  Config cfg;
  if (config_load(&cfg) != 0)
    return PRAYER_HELPER_ERR_CONFIG;
  if (location_prepare(&cfg) < 0)
    return PRAYER_HELPER_ERR_LOCATION;

  time_t now = time(NULL);
  struct tm tm_buf;
  platform_localtime(&now, &tm_buf);

  prayer_helper_compute(&cfg, &tm_buf, out);
  return PRAYER_HELPER_OK;
}
