#ifndef PRAYER_HELPER_H
#define PRAYER_HELPER_H

#include "config.h"
#include "prayer_checker.h"
#include "prayertimes.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Computed prayer data for a single day. Self-contained: owns a copy of the
 * Config and the date it was computed for, so consumers can hold it freely.
 */
typedef struct {
  struct tm date;           // day (and time-of-day) the snapshot was computed for
  Config config;            // owned copy of the config used
  struct PrayerTimes times; // the 7 computed times
  PrayerType next;          // PRAYER_NONE if none upcoming for `date`
  int minutes_until;        // minutes until `next` (undefined when next == PRAYER_NONE)
} PrayerSnapshot;

typedef enum {
  PRAYER_HELPER_OK = 0,
  PRAYER_HELPER_ERR_CONFIG,
  PRAYER_HELPER_ERR_LOCATION,
} PrayerHelperStatus;

/**
 * Pure: given a config and a moment (`now` carries both the calendar day and
 * the time-of-day), fill `out`. No I/O, no printing. This is the single shared
 * unit used by both the CLI and the GUI.
 */
void prayer_helper_compute(const Config *cfg, const struct tm *now, PrayerSnapshot *out);

/**
 * Convenience for silent callers (GUI, tests): config_load + location_prepare
 * + localtime(today) + prayer_helper_compute. Quiet — uses location_prepare, never the
 * interactive ensure_location. Returns PRAYER_HELPER_OK or an error code.
 */
PrayerHelperStatus prayer_helper_today(PrayerSnapshot *out);

#ifdef __cplusplus
}
#endif

#endif // PRAYER_HELPER_H
