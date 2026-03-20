#define PRAYERTIMES_IMPLEMENTATION
#include "prayertimes.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static int failures = 0;
static int total = 0;

// Convert "HH:MM" string to total minutes
static int time_to_minutes(const char *hhmm) {
  int h, m;
  sscanf(hhmm, "%d:%d", &h, &m);
  return h * 60 + m;
}

// Convert double hours to total minutes using the same ceil rounding as
// format_time_hm (Kemenag method)
static int double_to_minutes(double hours) {
  int h = (int)hours;
  double fraction = hours - h;
  int m = (int)ceil(fraction * 60.0);
  if (m >= 60) {
    h += 1;
    m -= 60;
  }
  h %= 24;
  return h * 60 + m;
}

// Check a single prayer time against expected value
static void check_time(double calculated, const char *expected,
                       int tolerance_min, const char *label) {
  int calc_min = double_to_minutes(calculated);
  int exp_min = time_to_minutes(expected);
  int diff = abs(calc_min - exp_min);
  total++;

  if (diff <= tolerance_min) {
    printf("  PASS  %-10s  calculated=%02d:%02d  expected=%s  (diff=%d min)\n",
           label, calc_min / 60, calc_min % 60, expected, diff);
  } else {
    printf("  FAIL  %-10s  calculated=%02d:%02d  expected=%s  (diff=%d min)\n",
           label, calc_min / 60, calc_min % 60, expected, diff);
    failures++;
  }
}

// Helper: run all 7 prayer time checks for a test case
static void check_all_prayers(struct PrayerTimes *t,
                              const char *fajr, const char *sunrise,
                              const char *dhuha, const char *dhuhr,
                              const char *asr, const char *maghrib,
                              const char *isha) {
  check_time(t->fajr,    fajr,    2, "Subuh");
  check_time(t->sunrise, sunrise, 2, "Terbit");
  check_time(t->dhuha,   dhuha,   2, "Dhuha");
  check_time(t->dhuhr,   dhuhr,   2, "Dzuhur");
  check_time(t->asr,     asr,     2, "Ashar");
  check_time(t->maghrib, maghrib, 2, "Maghrib");
  check_time(t->isha,    isha,    2, "Isya");
}

// =============================================================================
// Reference data source: jadwalsholat.org (Kemenag method)
// All expected times include the 2-minute ihtiyat (safety margin)
// =============================================================================

// ---------------------------------------------------------------------------
// 1. JAKARTA PUSAT (id=308)
//    Coordinates: 6d10'S 106d49'E => lat=-6.1667, lon=106.8167, tz=7.0
// ---------------------------------------------------------------------------

static void test_jakarta_jan(void) {
  printf("Test 01: Jakarta Pusat — 2026-01-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 1, 15,
                                                -6.1667, 106.8167, 7.0);
  check_all_prayers(&t, "04:26", "05:46", "06:10", "12:04", "15:29",
                    "18:17", "19:32");
  printf("\n");
}

static void test_jakarta_feb(void) {
  printf("Test 02: Jakarta Pusat — 2026-02-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 2, 15,
                                                -6.1667, 106.8167, 7.0);
  check_all_prayers(&t, "04:40", "05:56", "06:20", "12:09", "15:23",
                    "18:18", "19:29");
  printf("\n");
}

static void test_jakarta_apr(void) {
  printf("Test 03: Jakarta Pusat — 2026-04-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 4, 15,
                                                -6.1667, 106.8167, 7.0);
  check_all_prayers(&t, "04:38", "05:52", "06:16", "11:55", "15:14",
                    "17:54", "19:04");
  printf("\n");
}

static void test_jakarta_jun(void) {
  printf("Test 04: Jakarta Pusat — 2026-06-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 6, 15,
                                                -6.1667, 106.8167, 7.0);
  check_all_prayers(&t, "04:38", "05:58", "06:22", "11:55", "15:17",
                    "17:48", "19:03");
  printf("\n");
}

static void test_jakarta_jul(void) {
  printf("Test 05: Jakarta Pusat — 2026-07-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 7, 15,
                                                -6.1667, 106.8167, 7.0);
  check_all_prayers(&t, "04:44", "06:03", "06:27", "12:01", "15:23",
                    "17:54", "19:08");
  printf("\n");
}

static void test_jakarta_oct(void) {
  printf("Test 06: Jakarta Pusat — 2026-10-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 10, 15,
                                                -6.1667, 106.8167, 7.0);
  check_all_prayers(&t, "04:16", "05:30", "05:54", "11:41", "14:46",
                    "17:48", "18:58");
  printf("\n");
}

static void test_jakarta_dec(void) {
  printf("Test 07: Jakarta Pusat — 2025-12-15\n");
  struct PrayerTimes t = calculate_prayer_times(2025, 12, 15,
                                                -6.1667, 106.8167, 7.0);
  check_all_prayers(&t, "04:10", "05:31", "05:55", "11:49", "15:16",
                    "18:04", "19:20");
  printf("\n");
}

// ---------------------------------------------------------------------------
// 2. SURABAYA (id=265)
//    Coordinates: 7d14'S 112d45'E => lat=-7.2333, lon=112.75, tz=7.0
// ---------------------------------------------------------------------------

static void test_surabaya_jan(void) {
  printf("Test 08: Surabaya — 2026-01-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 1, 15,
                                                -7.2333, 112.75, 7.0);
  check_all_prayers(&t, "04:01", "05:21", "05:45", "11:40", "15:05",
                    "17:55", "19:10");
  printf("\n");
}

static void test_surabaya_feb(void) {
  printf("Test 09: Surabaya — 2026-02-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 2, 15,
                                                -7.2333, 112.75, 7.0);
  check_all_prayers(&t, "04:15", "05:31", "05:55", "11:45", "14:58",
                    "17:55", "19:07");
  printf("\n");
}

static void test_surabaya_apr(void) {
  printf("Test 10: Surabaya — 2026-04-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 4, 15,
                                                -7.2333, 112.75, 7.0);
  check_all_prayers(&t, "04:15", "05:29", "05:53", "11:31", "14:51",
                    "17:30", "18:40");
  printf("\n");
}

static void test_surabaya_jun(void) {
  printf("Test 11: Surabaya — 2026-06-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 6, 15,
                                                -7.2333, 112.75, 7.0);
  check_all_prayers(&t, "04:16", "05:36", "06:00", "11:31", "14:52",
                    "17:22", "18:37");
  printf("\n");
}

static void test_surabaya_jul(void) {
  printf("Test 12: Surabaya — 2026-07-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 7, 15,
                                                -7.2333, 112.75, 7.0);
  check_all_prayers(&t, "04:22", "05:41", "06:05", "11:37", "14:58",
                    "17:29", "18:43");
  printf("\n");
}

static void test_surabaya_oct(void) {
  printf("Test 13: Surabaya — 2026-10-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 10, 15,
                                                -7.2333, 112.75, 7.0);
  check_all_prayers(&t, "03:51", "05:06", "05:30", "11:17", "14:21",
                    "17:25", "18:35");
  printf("\n");
}

static void test_surabaya_dec(void) {
  printf("Test 14: Surabaya — 2025-12-15\n");
  struct PrayerTimes t = calculate_prayer_times(2025, 12, 15,
                                                -7.2333, 112.75, 7.0);
  check_all_prayers(&t, "03:44", "05:05", "05:29", "11:26", "14:53",
                    "17:42", "18:58");
  printf("\n");
}

// ---------------------------------------------------------------------------
// 3. MAKASSAR (id=140)
//    Coordinates: 5d9'S 119d28'E => lat=-5.15, lon=119.4667, tz=8.0
// ---------------------------------------------------------------------------

static void test_makassar_jan(void) {
  printf("Test 15: Makassar — 2026-01-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 1, 15,
                                                -5.15, 119.4667, 8.0);
  check_all_prayers(&t, "04:38", "05:57", "06:21", "12:13", "15:38",
                    "18:25", "19:40");
  printf("\n");
}

static void test_makassar_feb(void) {
  printf("Test 16: Makassar — 2026-02-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 2, 15,
                                                -5.15, 119.4667, 8.0);
  check_all_prayers(&t, "04:51", "06:06", "06:30", "12:18", "15:33",
                    "18:26", "19:38");
  printf("\n");
}

static void test_makassar_apr(void) {
  printf("Test 17: Makassar — 2026-04-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 4, 15,
                                                -5.15, 119.4667, 8.0);
  check_all_prayers(&t, "04:47", "06:01", "06:25", "12:05", "15:23",
                    "18:04", "19:14");
  printf("\n");
}

static void test_makassar_jul(void) {
  printf("Test 18: Makassar — 2026-07-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 7, 15,
                                                -5.15, 119.4667, 8.0);
  check_all_prayers(&t, "04:52", "06:11", "06:35", "12:10", "15:33",
                    "18:05", "19:19");
  printf("\n");
}

static void test_makassar_oct(void) {
  printf("Test 19: Makassar — 2026-10-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 10, 15,
                                                -5.15, 119.4667, 8.0);
  check_all_prayers(&t, "04:26", "05:40", "06:04", "11:50", "14:57",
                    "17:57", "19:06");
  printf("\n");
}

// ---------------------------------------------------------------------------
// 4. BANDA ACEH (id=12)
//    Coordinates: 5d19'N 95d21'E => lat=5.3167, lon=95.35, tz=7.0
// ---------------------------------------------------------------------------

static void test_banda_aceh_jan(void) {
  printf("Test 20: Banda Aceh — 2026-01-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 1, 15,
                                                5.3167, 95.35, 7.0);
  check_all_prayers(&t, "05:32", "06:50", "07:14", "12:50", "16:12",
                    "18:45", "19:59");
  printf("\n");
}

static void test_banda_aceh_feb(void) {
  printf("Test 21: Banda Aceh — 2026-02-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 2, 15,
                                                5.3167, 95.35, 7.0);
  check_all_prayers(&t, "05:37", "06:52", "07:16", "12:55", "16:15",
                    "18:53", "20:04");
  printf("\n");
}

static void test_banda_aceh_apr(void) {
  printf("Test 22: Banda Aceh — 2026-04-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 4, 15,
                                                5.3167, 95.35, 7.0);
  check_all_prayers(&t, "05:16", "06:30", "06:54", "12:41", "15:50",
                    "18:48", "19:58");
  printf("\n");
}

static void test_banda_aceh_jul(void) {
  printf("Test 23: Banda Aceh — 2026-07-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 7, 15,
                                                5.3167, 95.35, 7.0);
  check_all_prayers(&t, "05:10", "06:30", "06:54", "12:47", "16:12",
                    "18:59", "20:14");
  printf("\n");
}

static void test_banda_aceh_oct(void) {
  printf("Test 24: Banda Aceh — 2026-10-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 10, 15,
                                                5.3167, 95.35, 7.0);
  check_all_prayers(&t, "05:09", "06:22", "06:46", "12:27", "15:44",
                    "18:27", "19:36");
  printf("\n");
}

// ---------------------------------------------------------------------------
// 5. DENPASAR (id=66)
//    Coordinates: 8d39'S 115d13'E => lat=-8.65, lon=115.2167, tz=8.0
// ---------------------------------------------------------------------------

static void test_denpasar_jan(void) {
  printf("Test 25: Denpasar — 2026-01-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 1, 15,
                                                -8.65, 115.2167, 8.0);
  check_all_prayers(&t, "04:48", "06:09", "06:33", "12:30", "15:55",
                    "18:47", "20:03");
  printf("\n");
}

static void test_denpasar_feb(void) {
  printf("Test 26: Denpasar — 2026-02-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 2, 15,
                                                -8.65, 115.2167, 8.0);
  check_all_prayers(&t, "05:03", "06:20", "06:44", "12:35", "15:47",
                    "18:47", "19:59");
  printf("\n");
}

static void test_denpasar_apr(void) {
  printf("Test 27: Denpasar — 2026-04-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 4, 15,
                                                -8.65, 115.2167, 8.0);
  check_all_prayers(&t, "05:06", "06:20", "06:44", "12:21", "15:41",
                    "18:19", "19:29");
  printf("\n");
}

static void test_denpasar_jul(void) {
  printf("Test 28: Denpasar — 2026-07-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 7, 15,
                                                -8.65, 115.2167, 8.0);
  check_all_prayers(&t, "05:14", "06:33", "06:57", "12:27", "15:48",
                    "18:17", "19:31");
  printf("\n");
}

static void test_denpasar_oct(void) {
  printf("Test 29: Denpasar — 2026-10-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 10, 15,
                                                -8.65, 115.2167, 8.0);
  check_all_prayers(&t, "04:40", "05:55", "06:19", "12:07", "15:10",
                    "18:16", "19:26");
  printf("\n");
}

// ---------------------------------------------------------------------------
// 6. JAYAPURA (id=84)
//    Coordinates: 0d32'S 140d27'E => lat=-0.5333, lon=140.45, tz=9.0
// ---------------------------------------------------------------------------

static void test_jayapura_jan(void) {
  printf("Test 30: Jayapura — 2026-01-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 1, 15,
                                                -0.5333, 140.45, 9.0);
  check_all_prayers(&t, "04:24", "05:42", "06:06", "11:49", "15:14",
                    "17:52", "19:06");
  printf("\n");
}

static void test_jayapura_feb(void) {
  printf("Test 31: Jayapura — 2026-02-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 2, 15,
                                                -0.5333, 140.45, 9.0);
  check_all_prayers(&t, "04:33", "05:47", "06:11", "11:54", "15:13",
                    "17:57", "19:08");
  printf("\n");
}

static void test_jayapura_apr(void) {
  printf("Test 32: Jayapura — 2026-04-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 4, 15,
                                                -0.5333, 140.45, 9.0);
  check_all_prayers(&t, "04:19", "05:33", "05:57", "11:41", "14:55",
                    "17:44", "18:54");
  printf("\n");
}

static void test_jayapura_jul(void) {
  printf("Test 33: Jayapura — 2026-07-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 7, 15,
                                                -0.5333, 140.45, 9.0);
  check_all_prayers(&t, "04:19", "05:38", "06:02", "11:46", "15:11",
                    "17:51", "19:05");
  printf("\n");
}

static void test_jayapura_oct(void) {
  printf("Test 34: Jayapura — 2026-10-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 10, 15,
                                                -0.5333, 140.45, 9.0);
  check_all_prayers(&t, "04:06", "05:19", "05:43", "11:26", "14:40",
                    "17:29", "18:39");
  printf("\n");
}

// ---------------------------------------------------------------------------
// 7. PONTIANAK (id=203)
//    Coordinates: 0d0'N 109d19'E => lat=0.0, lon=109.3167, tz=7.0
// ---------------------------------------------------------------------------

static void test_pontianak_jan(void) {
  printf("Test 35: Pontianak — 2026-01-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 1, 15,
                                                0.0, 109.3167, 7.0);
  check_all_prayers(&t, "04:27", "05:46", "06:10", "11:54", "15:19",
                    "17:57", "19:11");
  printf("\n");
}

static void test_pontianak_feb(void) {
  printf("Test 36: Pontianak — 2026-02-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 2, 15,
                                                0.0, 109.3167, 7.0);
  check_all_prayers(&t, "04:37", "05:51", "06:15", "11:59", "15:18",
                    "18:02", "19:13");
  printf("\n");
}

static void test_pontianak_apr(void) {
  printf("Test 37: Pontianak — 2026-04-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 4, 15,
                                                0.0, 109.3167, 7.0);
  check_all_prayers(&t, "04:24", "05:38", "06:02", "11:45", "15:00",
                    "17:48", "18:58");
  printf("\n");
}

static void test_pontianak_jul(void) {
  printf("Test 38: Pontianak — 2026-07-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 7, 15,
                                                0.0, 109.3167, 7.0);
  check_all_prayers(&t, "04:24", "05:43", "06:07", "11:51", "15:16",
                    "17:54", "19:08");
  printf("\n");
}

static void test_pontianak_oct(void) {
  printf("Test 39: Pontianak — 2026-10-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 10, 15,
                                                0.0, 109.3167, 7.0);
  check_all_prayers(&t, "04:10", "05:24", "05:48", "11:31", "14:44",
                    "17:34", "18:44");
  printf("\n");
}

// ---------------------------------------------------------------------------
// 8. MEDAN (id=151)
//    Coordinates: 3d35'N 98d39'E => lat=3.5833, lon=98.65, tz=7.0
// ---------------------------------------------------------------------------

static void test_medan_jan(void) {
  printf("Test 40: Medan — 2026-01-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 1, 15,
                                                3.5833, 98.65, 7.0);
  check_all_prayers(&t, "05:16", "06:34", "06:58", "12:36", "16:00",
                    "18:35", "19:48");
  printf("\n");
}

static void test_medan_feb(void) {
  printf("Test 41: Medan — 2026-02-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 2, 15,
                                                3.5833, 98.65, 7.0);
  check_all_prayers(&t, "05:23", "06:37", "07:01", "12:42", "16:02",
                    "18:42", "19:52");
  printf("\n");
}

static void test_medan_apr(void) {
  printf("Test 42: Medan — 2026-04-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 4, 15,
                                                3.5833, 98.65, 7.0);
  check_all_prayers(&t, "05:04", "06:18", "06:42", "12:28", "15:39",
                    "18:33", "19:43");
  printf("\n");
}

static void test_medan_jul(void) {
  printf("Test 43: Medan — 2026-07-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 7, 15,
                                                3.5833, 98.65, 7.0);
  check_all_prayers(&t, "05:01", "06:20", "06:44", "12:33", "15:59",
                    "18:43", "19:57");
  printf("\n");
}

static void test_medan_oct(void) {
  printf("Test 44: Medan — 2026-10-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 10, 15,
                                                3.5833, 98.65, 7.0);
  check_all_prayers(&t, "04:55", "06:08", "06:32", "12:13", "15:30",
                    "18:15", "19:24");
  printf("\n");
}

// ---------------------------------------------------------------------------
// 9. MENADO (id=153)
//    Coordinates: 1d29'N 124d52'E => lat=1.4833, lon=124.8667, tz=8.0
// ---------------------------------------------------------------------------

static void test_menado_jan(void) {
  printf("Test 45: Menado — 2026-01-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 1, 15,
                                                1.4833, 124.8667, 8.0);
  check_all_prayers(&t, "04:28", "05:46", "06:10", "11:52", "15:16",
                    "17:53", "19:07");
  printf("\n");
}

static void test_menado_feb(void) {
  printf("Test 46: Menado — 2026-02-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 2, 15,
                                                1.4833, 124.8667, 8.0);
  check_all_prayers(&t, "04:36", "05:51", "06:15", "11:57", "15:16",
                    "17:59", "19:09");
  printf("\n");
}

static void test_menado_apr(void) {
  printf("Test 47: Menado — 2026-04-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 4, 15,
                                                1.4833, 124.8667, 8.0);
  check_all_prayers(&t, "04:21", "05:35", "05:59", "11:43", "14:56",
                    "17:47", "18:57");
  printf("\n");
}

static void test_menado_jul(void) {
  printf("Test 48: Menado — 2026-07-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 7, 15,
                                                1.4833, 124.8667, 8.0);
  check_all_prayers(&t, "04:19", "05:38", "06:02", "11:48", "15:14",
                    "17:54", "19:09");
  printf("\n");
}

static void test_menado_oct(void) {
  printf("Test 49: Menado — 2026-10-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 10, 15,
                                                1.4833, 124.8667, 8.0);
  check_all_prayers(&t, "04:09", "05:22", "05:46", "11:29", "14:43",
                    "17:31", "18:40");
  printf("\n");
}

// ---------------------------------------------------------------------------
// 10. BANDUNG (id=14)
//     Coordinates: 6d57'S 107d34'E => lat=-6.95, lon=107.5667, tz=7.0
// ---------------------------------------------------------------------------

static void test_bandung_jan(void) {
  printf("Test 50: Bandung — 2026-01-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 1, 15,
                                                -6.95, 107.5667, 7.0);
  check_all_prayers(&t, "04:22", "05:42", "06:06", "12:01", "15:26",
                    "18:15", "19:31");
  printf("\n");
}

static void test_bandung_jul(void) {
  printf("Test 51: Bandung — 2026-07-15\n");
  struct PrayerTimes t = calculate_prayer_times(2026, 7, 15,
                                                -6.95, 107.5667, 7.0);
  check_all_prayers(&t, "04:42", "06:01", "06:25", "11:58", "15:19",
                    "17:50", "19:04");
  printf("\n");
}

int main(void) {
  printf("=== prayertimes.h unit tests ===\n");
  printf("=== Reference: jadwalsholat.org (Kemenag method) ===\n");
  printf("=== All times include 2-min ihtiyat safety margin ===\n\n");

  // Jakarta Pusat (7 tests)
  test_jakarta_jan();
  test_jakarta_feb();
  test_jakarta_apr();
  test_jakarta_jun();
  test_jakarta_jul();
  test_jakarta_oct();
  test_jakarta_dec();

  // Surabaya (7 tests)
  test_surabaya_jan();
  test_surabaya_feb();
  test_surabaya_apr();
  test_surabaya_jun();
  test_surabaya_jul();
  test_surabaya_oct();
  test_surabaya_dec();

  // Makassar (5 tests)
  test_makassar_jan();
  test_makassar_feb();
  test_makassar_apr();
  test_makassar_jul();
  test_makassar_oct();

  // Banda Aceh (5 tests)
  test_banda_aceh_jan();
  test_banda_aceh_feb();
  test_banda_aceh_apr();
  test_banda_aceh_jul();
  test_banda_aceh_oct();

  // Denpasar (5 tests)
  test_denpasar_jan();
  test_denpasar_feb();
  test_denpasar_apr();
  test_denpasar_jul();
  test_denpasar_oct();

  // Jayapura (5 tests)
  test_jayapura_jan();
  test_jayapura_feb();
  test_jayapura_apr();
  test_jayapura_jul();
  test_jayapura_oct();

  // Pontianak (5 tests)
  test_pontianak_jan();
  test_pontianak_feb();
  test_pontianak_apr();
  test_pontianak_jul();
  test_pontianak_oct();

  // Medan (5 tests)
  test_medan_jan();
  test_medan_feb();
  test_medan_apr();
  test_medan_jul();
  test_medan_oct();

  // Menado (5 tests)
  test_menado_jan();
  test_menado_feb();
  test_menado_apr();
  test_menado_jul();
  test_menado_oct();

  // Bandung (2 tests)
  test_bandung_jan();
  test_bandung_jul();

  printf("=== Summary ===\n");
  printf("Total checks: %d\n", total);
  if (failures == 0) {
    printf("All tests passed.\n");
  } else {
    printf("%d check(s) FAILED out of %d.\n", failures, total);
  }

  return failures > 0 ? 1 : 0;
}
