#include "config.h"
#include "prayer_checker.h"
#include "prayer_helper.h"
#include "prayertimes.h"
#include <math.h>
#include <stdio.h>

static int passed = 0;
static int failed = 0;

static void check(const char *name, int cond) {
  if (cond) {
    passed++;
  } else {
    failed++;
    fprintf(stderr, "FAIL [%s]\n", name);
  }
}

// Jakarta, all defaults, fixed offset so the test is deterministic.
static Config jakarta_config(void) {
  Config cfg = config_default();
  cfg.latitude = -6.2088;
  cfg.longitude = 106.8456;
  cfg.timezone_offset = 7.0;
  cfg.auto_detect = false;
  return cfg;
}

static struct tm fixed_now(void) {
  struct tm t = {0};
  t.tm_year = 2026 - 1900;
  t.tm_mon = 0; // January
  t.tm_mday = 1;
  t.tm_hour = 10;
  t.tm_min = 0;
  return t;
}

int main(void) {
  Config cfg = jakarta_config();
  struct tm now = fixed_now();

  PrayerSnapshot snap;
  prayer_helper_compute(&cfg, &now, &snap);

  // Reference: call the underlying functions directly with identical inputs.
  MethodParams params = method_params_from_config(&cfg);
  struct PrayerTimes ref =
      calculate_prayer_times(now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, cfg.latitude,
                             cfg.longitude, cfg.timezone_offset, &params);
  struct tm now_copy = now;
  int ref_minutes = 0;
  PrayerType ref_next = prayer_get_next(&cfg, &now_copy, &ref, &ref_minutes);

  check("times.fajr matches", fabs(snap.times.fajr - ref.fajr) < 1e-9);
  check("times.sunrise matches", fabs(snap.times.sunrise - ref.sunrise) < 1e-9);
  check("times.dhuha matches", fabs(snap.times.dhuha - ref.dhuha) < 1e-9);
  check("times.dhuhr matches", fabs(snap.times.dhuhr - ref.dhuhr) < 1e-9);
  check("times.asr matches", fabs(snap.times.asr - ref.asr) < 1e-9);
  check("times.maghrib matches", fabs(snap.times.maghrib - ref.maghrib) < 1e-9);
  check("times.isha matches", fabs(snap.times.isha - ref.isha) < 1e-9);
  check("next matches", snap.next == ref_next);
  check("minutes_until matches", snap.minutes_until == ref_minutes);
  check("config copied", snap.config.timezone_offset == cfg.timezone_offset);
  check("date copied", snap.date.tm_mday == now.tm_mday);

  printf("test_prayer_helper: %d passed, %d failed\n", passed, failed);
  return failed == 0 ? 0 : 1;
}
