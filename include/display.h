#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include "prayer_checker.h"
#include "../src/prayertimes.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Display prayer times in table format
 */
void display_prayer_times_table(const struct PrayerTimes *times, 
                                const Config *cfg,
                                struct tm *date);

/**
 * Display prayer times in JSON format
 */
void display_prayer_times_json(const struct PrayerTimes *times,
                               const Config *cfg,
                               struct tm *date);

/**
 * Display next prayer info
 */
void display_next_prayer(const struct PrayerTimes *times,
                         const Config *cfg,
                         struct tm *current_time);

/**
 * Display location info
 */
void display_location(const Config *cfg);

/**
 * Display config
 */
void display_config(const Config *cfg);

/**
 * Display prayer list with status
 */
void display_prayer_list(const Config *cfg);

/**
 * Display reminders configuration
 */
void display_reminders(const Config *cfg);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H
