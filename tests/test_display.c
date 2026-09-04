// Mutation record for Task 5 of the day-offset-visible plan.
//
// Each mutant below was applied to the committed source, built with
// cmake --build build -j, and run with
// ctest --test-dir build -R display --output-on-failure
// then the source file was restored with git checkout -- <file> and
// git status --porcelain was confirmed empty before the next mutant.
//
// Mutant one: src/core/display.c, format_time_hm_day. Changed
//   snprintf(outBuffer, bufSize, "%s%s", hm, day > 0 ? "+" : (day < 0 ? "-" : ""));
// to always pass the empty string for the marker.
// This mutant bit. Exit code 8. Verbatim tail of the run:
//
// FAIL [+ marker at or above 24:00]
// (repeated 172 times)
// FAIL [marker is + for next day]
// (repeated 1801 times)
// FAIL [marker is - for previous day]
// (repeated 1800 times)
// FAIL [- marker below 0:00]
// Running display tests...
//   day offset helper...
//   no misordered row...
//   marker matches raw value...
//   cache triggers unchanged...
//
// Results: 18661 passed, 3774 failed
//
// 0% tests passed, 1 tests failed out of 1
//
// Mutant two: src/core/cache.c, cache_build_triggers. Deleted the wrap
//   if (pt < 0.0)
//     pt += 24.0;
//   else if (pt >= 24.0)
//     pt -= 24.0;
// that sits immediately before int prayer_min = (int)ceil(pt * 60.0).
// This mutant bit. Exit code 8. Verbatim tail of the run:
//
// FAIL [trigger minute in [0, 1440)]
// FAIL [trigger minute in [0, 1440)]
// Running display tests...
//   day offset helper...
//   no misordered row...
//   marker matches raw value...
//   cache triggers unchanged...
//
// Results: 22433 passed, 2 failed
//
// 0% tests passed, 1 tests failed out of 1
//
// Mutant three: src/core/config.c, prayer_times_for_config. Restored the
// original defect by putting a wrap back inside the per-prayer offset loop
//   for (int i = 0; i < PRAYER_COUNT; i++) {
//     *fields[i] += pcfgs[i]->offset / 60.0;
//     if (*fields[i] < 0.0)
//       *fields[i] += 24.0;
//     else if (*fields[i] >= 24.0)
//       *fields[i] -= 24.0;
//   }
// which discards the day offset at the source, before display or cache ever
// see it.
// This mutant bit. Exit code 8. Verbatim tail of the run:
//
// FAIL [Reykjavik: no misordered rows]
// FAIL [Anchorage: no misordered rows]
// FAIL [Murmansk: no misordered rows]
// FAIL [Tromso: no misordered rows]
// FAIL [found a Reykjavik day with isha >= 24]
// Running display tests...
//   day offset helper...
//   no misordered row...
//   marker matches raw value...
//   cache triggers unchanged...
//
// Results: 22409 passed, 5 failed
//
// 0% tests passed, 1 tests failed out of 1
//
// All three mutants were detected by the display suite. None left it green.
// src/core/display.c, src/core/cache.c and src/core/config.c were restored
// to their committed state with git checkout -- after each mutant and
// verified byte-identical before the next one was applied.

#define _GNU_SOURCE
#include "cache.h"
#include "config.h"
#include "display.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int passed = 0;
static int failed = 0;

static void check_bool(const char *test, bool cond) {
  if (cond) {
    passed++;
  } else {
    failed++;
    fprintf(stderr, "FAIL [%s]\n", test);
  }
}

// -- Site fixtures ------------------------------------------------------------

typedef struct {
  const char *name;
  double latitude;
  double longitude;
  double offset;
} Site;

// Coordinates and fixed UTC offsets recorded in the day-offset-visible design
// spec's Problem section, all under CALC_MWL.
static const Site SITES[] = {
    {"Reykjavik", 64.1466, -21.9426, 0.0},
    {"Anchorage", 61.2181, -149.9003, -9.0},
    {"Murmansk", 68.9585, 33.0827, 3.0},
    {"Tromso", 69.6492, 18.9553, 1.0},
};
#define SITE_COUNT (sizeof(SITES) / sizeof(SITES[0]))

static Config site_config(const Site *s) {
  Config cfg = config_default();
  cfg.latitude = s->latitude;
  cfg.longitude = s->longitude;
  cfg.timezone_offset = s->offset;
  // Use the stored fixed offset directly rather than a named zone lookup, the
  // same reason tests/test_config.c does this in test_offset_apply.
  cfg.timezone[0] = '\0';
  strncpy(cfg.calculation_method, "mwl", sizeof(cfg.calculation_method) - 1);
  cfg.calculation_method[sizeof(cfg.calculation_method) - 1] = '\0';
  return cfg;
}

// -- test_day_offset_helper ----------------------------------------------------

// Exhaustive check of prayer_time_day_offset and format_time_hm_day over
// synthetic input, stepping one minute at a time from -30h to +54h.
static void test_day_offset_helper(void) {
  printf("  day offset helper...\n");

  for (double h = -30.0; h <= 54.0; h += (1.0 / 60.0)) {
    long total = (long)ceil(h * 60.0);
    int expected_offset = total < 0 ? -1 : (total >= 24L * 60 ? 1 : 0);

    int offset = prayer_time_day_offset(h);
    check_bool("day offset matches minute-rounded rule", offset == expected_offset);

    char hd[8];
    format_time_hm_day(h, hd, sizeof(hd));

    char marker = hd[strlen(hd) - 1];
    bool has_marker = (marker == '+' || marker == '-');
    if (expected_offset > 0) {
      check_bool("marker is + for next day", has_marker && marker == '+');
    } else if (expected_offset < 0) {
      check_bool("marker is - for previous day", has_marker && marker == '-');
    } else {
      check_bool("no marker for same day", !has_marker);
    }

    char hm[6];
    format_time_hm(h, hm, sizeof(hm));
    char hd_prefix[6];
    strncpy(hd_prefix, hd, 5);
    hd_prefix[5] = '\0';
    check_bool("hm prefix matches format_time_hm", strcmp(hd_prefix, hm) == 0);
  }
}

// -- test_no_misordered_row ----------------------------------------------------

// Walk every site through every day of 2026 and assert the rendered sequence
// fajr, dhuhr, asr, maghrib, isha is non-decreasing once the rendered day
// offset is applied, comparing day * 1440 + minute_of_day.
static void test_no_misordered_row(void) {
  printf("  no misordered row...\n");

  long start = mt_days_from_civil(2026, 1, 1);
  long end = mt_days_from_civil(2026, 12, 31);

  for (size_t s = 0; s < SITE_COUNT; s++) {
    Config cfg = site_config(&SITES[s]);
    int misordered = 0;
    int day_count = 0;

    for (long z = start; z <= end; z++) {
      int y, m, d;
      mt_civil_from_days(z, &y, &m, &d);
      day_count++;

      struct PrayerTimes t = prayer_times_for_config(&cfg, y, m, d);
      double values[5] = {t.fajr, t.dhuhr, t.asr, t.maghrib, t.isha};

      // A prayer the Sun never reaches on this day is non-finite and has no
      // slot in the ordering, the same reason cache_build_triggers skips it
      // (src/core/cache.c). Only finite fields are compared.
      long prev_key = LONG_MIN;
      bool row_ok = true;
      for (int i = 0; i < 5; i++) {
        if (!isfinite(values[i]))
          continue;
        int day_off = prayer_time_day_offset(values[i]);
        long minute_of_day = (long)ceil(values[i] * 60.0) - (long)day_off * 24L * 60L;
        long key = (long)day_off * 1440L + minute_of_day;
        if (key < prev_key)
          row_ok = false;
        prev_key = key;
      }
      if (!row_ok)
        misordered++;
    }

    check_bool("site has days in 2026", day_count == 365);

    char msg[64];
    snprintf(msg, sizeof(msg), "%s: no misordered rows", SITES[s].name);
    check_bool(msg, misordered == 0);
  }
}

// -- test_marker_matches_raw_value ---------------------------------------------

// Over the same four sites and year, assert '+' is rendered exactly on fields
// whose raw value is at or above 24 and '-' exactly on those below 0. Without
// this, test_no_misordered_row would pass with a marker that is always '+'.
static void test_marker_matches_raw_value(void) {
  printf("  marker matches raw value...\n");

  long start = mt_days_from_civil(2026, 1, 1);
  long end = mt_days_from_civil(2026, 12, 31);

  for (size_t s = 0; s < SITE_COUNT; s++) {
    Config cfg = site_config(&SITES[s]);
    MethodParams mp = method_params_from_config(&cfg);

    int seen_plus = 0;
    int seen_minus = 0;

    for (long z = start; z <= end; z++) {
      int y, m, d;
      mt_civil_from_days(z, &y, &m, &d);

      struct PrayerTimes t =
          calculate_prayer_times(y, m, d, cfg.latitude, cfg.longitude, cfg.timezone_offset, &mp);
      double values[5] = {t.fajr, t.dhuhr, t.asr, t.maghrib, t.isha};

      for (int i = 0; i < 5; i++) {
        if (!isfinite(values[i]))
          continue;

        char hd[8];
        format_time_hm_day(values[i], hd, sizeof(hd));
        char marker = hd[strlen(hd) - 1];

        // The marker follows the minute-rounded value, matching format_time_hm's
        // own rounding (display.h documents this: 23.999 rounds up to 24:00 and
        // counts as next day), so the threshold here is applied after the same
        // ceil-to-the-minute step rather than to the raw double.
        long total_minutes = (long)ceil(values[i] * 60.0);
        if (total_minutes >= 24L * 60) {
          check_bool("+ marker at or above 24:00", marker == '+');
          seen_plus++;
        } else if (total_minutes < 0) {
          check_bool("- marker below 0:00", marker == '-');
          seen_minus++;
        } else {
          check_bool("no marker within the same day", marker != '+' && marker != '-');
        }
      }
    }

    // Reykjavik and Tromso are the ones the spec recorded as having wrapped
    // days (107 and 1 respectively), so at least one of the four sites must
    // actually exercise the '+' branch, or this test would pass vacuously.
    if (strcmp(SITES[s].name, "Reykjavik") == 0) {
      check_bool("Reykjavik: some raw value >= 24 seen", seen_plus > 0);
    }
    (void)seen_minus;
  }
}

// -- test_cache_triggers_unchanged --------------------------------------------

// Pin the non-goal: cache_build_triggers still produces trigger minutes in
// [0, 1440) after the wrap moved into it, on a Reykjavik day whose isha is
// at or above 24.
static void test_cache_triggers_unchanged(void) {
  printf("  cache triggers unchanged...\n");

  Config cfg = site_config(&SITES[0]); // Reykjavik

  long start = mt_days_from_civil(2026, 1, 1);
  long end = mt_days_from_civil(2026, 12, 31);
  bool found = false;

  for (long z = start; z <= end && !found; z++) {
    int y, m, d;
    mt_civil_from_days(z, &y, &m, &d);
    struct PrayerTimes t = prayer_times_for_config(&cfg, y, m, d);
    if (!isfinite(t.isha) || t.isha < 24.0)
      continue;

    found = true;

    PrayerCache cache = {0};
    char date_str[16];
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", y, m, d);
    cache_build_triggers(&cache, &cfg, &t, 0, date_str);

    check_bool("cache produced triggers", cache.trigger_count > 0);
    for (int i = 0; i < cache.trigger_count; i++) {
      check_bool("trigger minute in [0, 1440)",
                 cache.triggers[i].minute >= 0 && cache.triggers[i].minute < 1440);
    }
  }

  check_bool("found a Reykjavik day with isha >= 24", found);
}

// -- main ---------------------------------------------------------------------

int main(void) {
  printf("Running display tests...\n");
  test_day_offset_helper();
  test_no_misordered_row();
  test_marker_matches_raw_value();
  test_cache_triggers_unchanged();

  printf("\nResults: %d passed, %d failed\n", passed, failed);
  return failed > 0 ? 1 : 0;
}
