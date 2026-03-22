# Prayer Time Cache Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Cache prayer trigger times to `~/.cache/muslimtify/next_prayer.json` so `handle_check` avoids recalculating astronomical math every minute.

**Architecture:** New `cache.c`/`cache.h` module handles reading, writing, and invalidating a JSON cache file. `handle_check` reads cache first; if valid (same date, triggers remaining), it does a simple integer minute comparison. When a trigger fires or the cache is invalid, it recalculates from current time + fresh config and rebuilds the cache. Config-changing commands (`enable`, `disable`, `reminder`) delete the cache file to force recalculation.

**Tech Stack:** C99, json.h (existing header-only JSON parser), POSIX filesystem APIs

---

## File Structure

| Action | File | Responsibility |
|--------|------|----------------|
| Create | `include/cache.h` | Cache struct definitions and public API |
| Create | `src/cache.c` | Cache load/save/invalidate, trigger list builder |
| Create | `tests/test_cache.c` | Unit tests for cache module |
| Modify | `src/cmd_show.c:54-95` | `handle_check` uses cache |
| Modify | `src/cmd_prayer.c` | `handle_enable`, `handle_disable`, `handle_reminder` invalidate cache |
| Modify | `CMakeLists.txt` | Add `cache.c` to build, add `test_cache` target |

---

### Task 1: Create cache header with types and API

**Files:**
- Create: `include/cache.h`

- [ ] **Step 1: Write cache header**

```c
#ifndef CACHE_H
#define CACHE_H

#include "config.h"
#include "prayer_checker.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_TRIGGERS 64

typedef struct {
    char prayer[16];       // Prayer name (e.g., "fajr")
    int minute;            // Absolute minute of day (hour*60 + min)
    int minutes_before;    // 0 = exact time, >0 = reminder
    double prayer_time;    // Prayer time in decimal hours
} CacheTrigger;

typedef struct {
    char date[16];                   // "YYYY-MM-DD"
    CacheTrigger triggers[MAX_TRIGGERS];
    int trigger_count;
} PrayerCache;

/**
 * Get cache file path (~/.cache/muslimtify/next_prayer.json)
 */
const char *cache_get_path(void);

/**
 * Load cache from disk.
 * Returns: 0 on success, -1 on error (missing/corrupt/wrong date)
 */
int cache_load(PrayerCache *cache);

/**
 * Save cache to disk.
 * Returns: 0 on success, -1 on error
 */
int cache_save(const PrayerCache *cache);

/**
 * Delete cache file (invalidate).
 */
void cache_invalidate(void);

/**
 * Build trigger list from current time and prayer times.
 * Only includes triggers at or after current_minute.
 * Returns: number of triggers added
 */
int cache_build_triggers(PrayerCache *cache,
                         const Config *cfg,
                         const struct PrayerTimes *times,
                         int current_minute,
                         const char *date_str);

/**
 * Remove a trigger by index (caller must call cache_save afterward).
 */
void cache_remove_trigger(PrayerCache *cache, int index);

/**
 * Reset cached path (for testing with different XDG_CACHE_HOME).
 */
void cache_reset_path(void);

#ifdef __cplusplus
}
#endif

#endif // CACHE_H
```

- [ ] **Step 2: Commit**

```bash
git add include/cache.h
git commit -m "feat: add cache header with trigger types and API"
```

---

### Task 2: Write failing tests for cache module

**Files:**
- Create: `tests/test_cache.c`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write test file**

```c
#include "cache.h"
#include "config.h"
#include "prayer_checker.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

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

// Jakarta prayer times (same as test_prayer_checker.c)
static struct PrayerTimes jakarta_times(void) {
    return (struct PrayerTimes){
        .fajr = 4.0 + 26.0 / 60.0,
        .sunrise = 5.0 + 46.0 / 60.0,
        .dhuha = 6.0 + 14.0 / 60.0,
        .dhuhr = 12.0 + 4.0 / 60.0,
        .asr = 15.0 + 29.0 / 60.0,
        .maghrib = 18.0 + 17.0 / 60.0,
        .isha = 19.0 + 32.0 / 60.0,
    };
}

static Config test_config(void) {
    Config cfg = config_default();
    cfg.latitude = -6.2088;
    cfg.longitude = 106.8456;
    cfg.timezone_offset = 7.0;
    cfg.auto_detect = false;
    return cfg;
}

static void test_build_triggers_includes_future(void) {
    printf("  build triggers includes future only...\n");
    Config cfg = test_config();
    struct PrayerTimes times = jakarta_times();
    PrayerCache cache = {0};

    // At 12:00 (minute 720), should include dhuhr (12:04=724) and later,
    // plus their reminders. Should NOT include fajr, sunrise, dhuha.
    int count = cache_build_triggers(&cache, &cfg, &times, 720, "2026-03-22");

    check_bool("has triggers", count > 0);
    check_bool("date set", strcmp(cache.date, "2026-03-22") == 0);

    // First trigger should be at or after minute 720
    for (int i = 0; i < cache.trigger_count; i++) {
        check_bool("trigger >= current",
                   cache.triggers[i].minute >= 720);
    }

    // Should include dhuhr exact (minute 724 = ceil(12.0667*60))
    bool found_dhuhr = false;
    for (int i = 0; i < cache.trigger_count; i++) {
        if (strcmp(cache.triggers[i].prayer, "Dhuhr") == 0 &&
            cache.triggers[i].minutes_before == 0) {
            found_dhuhr = true;
        }
    }
    check_bool("includes dhuhr exact", found_dhuhr);
}

static void test_build_triggers_sorted(void) {
    printf("  build triggers sorted ascending...\n");
    Config cfg = test_config();
    struct PrayerTimes times = jakarta_times();
    PrayerCache cache = {0};

    cache_build_triggers(&cache, &cfg, &times, 0, "2026-03-22");

    for (int i = 1; i < cache.trigger_count; i++) {
        check_bool("sorted ascending",
                   cache.triggers[i].minute >= cache.triggers[i - 1].minute);
    }
}

static void test_build_triggers_skips_disabled(void) {
    printf("  build triggers skips disabled prayers...\n");
    Config cfg = test_config();
    cfg.fajr.enabled = false;
    struct PrayerTimes times = jakarta_times();
    PrayerCache cache = {0};

    cache_build_triggers(&cache, &cfg, &times, 0, "2026-03-22");

    for (int i = 0; i < cache.trigger_count; i++) {
        check_bool("no fajr trigger",
                   strcmp(cache.triggers[i].prayer, "Fajr") != 0);
    }
}

static void test_build_triggers_includes_reminders(void) {
    printf("  build triggers includes reminders...\n");
    Config cfg = test_config();
    struct PrayerTimes times = jakarta_times();
    PrayerCache cache = {0};

    // At minute 0, should include fajr reminders (30, 15, 5 min before)
    cache_build_triggers(&cache, &cfg, &times, 0, "2026-03-22");

    int fajr_reminder_count = 0;
    for (int i = 0; i < cache.trigger_count; i++) {
        if (strcmp(cache.triggers[i].prayer, "Fajr") == 0 &&
            cache.triggers[i].minutes_before > 0) {
            fajr_reminder_count++;
        }
    }
    check_bool("fajr has 3 reminders", fajr_reminder_count == 3);
}

static void test_remove_trigger(void) {
    printf("  remove trigger...\n");
    PrayerCache cache = {0};
    strcpy(cache.date, "2026-03-22");
    cache.trigger_count = 3;
    strcpy(cache.triggers[0].prayer, "Fajr");
    cache.triggers[0].minute = 236;
    strcpy(cache.triggers[1].prayer, "Fajr");
    cache.triggers[1].minute = 251;
    strcpy(cache.triggers[2].prayer, "Dhuhr");
    cache.triggers[2].minute = 724;

    cache_remove_trigger(&cache, 0);

    check_bool("count decremented", cache.trigger_count == 2);
    check_bool("shifted correctly",
               strcmp(cache.triggers[0].prayer, "Fajr") == 0 &&
               cache.triggers[0].minute == 251);
}

static void test_save_load_roundtrip(void) {
    printf("  save/load roundtrip...\n");

    // Redirect cache to a temp directory via XDG_CACHE_HOME
    char tmpdir[] = "/tmp/muslimtify_test_XXXXXX";
    if (!mkdtemp(tmpdir)) {
        fprintf(stderr, "FAIL [mkdtemp]\n");
        failed++;
        return;
    }
    setenv("XDG_CACHE_HOME", tmpdir, 1);
    cache_reset_path();

    // Build a cache
    PrayerCache original = {0};
    strcpy(original.date, "2026-03-22");
    original.trigger_count = 2;
    strcpy(original.triggers[0].prayer, "Fajr");
    original.triggers[0].minute = 266;
    original.triggers[0].minutes_before = 0;
    original.triggers[0].prayer_time = 4.4333;
    strcpy(original.triggers[1].prayer, "Dhuhr");
    original.triggers[1].minute = 724;
    original.triggers[1].minutes_before = 0;
    original.triggers[1].prayer_time = 12.0667;

    // Save and reload
    int save_ok = cache_save(&original);
    check_bool("save succeeds", save_ok == 0);

    PrayerCache loaded = {0};
    int load_ok = cache_load(&loaded);
    check_bool("load succeeds", load_ok == 0);
    check_bool("date matches", strcmp(loaded.date, "2026-03-22") == 0);
    check_bool("count matches", loaded.trigger_count == 2);
    check_bool("prayer[0] matches",
               strcmp(loaded.triggers[0].prayer, "Fajr") == 0);
    check_bool("minute[0] matches", loaded.triggers[0].minute == 266);
    check_bool("prayer[1] matches",
               strcmp(loaded.triggers[1].prayer, "Dhuhr") == 0);
    check_bool("prayer_time[1] close",
               fabs(loaded.triggers[1].prayer_time - 12.0667) < 0.01);

    // Cleanup
    cache_invalidate();
    cache_reset_path();
    char rm_cmd[512];
    snprintf(rm_cmd, sizeof(rm_cmd), "rm -rf %s", tmpdir);
    (void)system(rm_cmd);
    unsetenv("XDG_CACHE_HOME");
}

int main(void) {
    printf("Running cache tests...\n");

    test_build_triggers_includes_future();
    test_build_triggers_sorted();
    test_build_triggers_skips_disabled();
    test_build_triggers_includes_reminders();
    test_remove_trigger();
    test_save_load_roundtrip();

    printf("\nResults: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
```

- [ ] **Step 2: Add test_cache target to CMakeLists.txt**

After the `test_json` block (around line 156), add:

```cmake
    add_executable(test_cache tests/test_cache.c)
    muslimtify_set_target_defaults(test_cache)
    if(NOT WIN32)
        target_include_directories(test_cache PRIVATE ${LIBNOTIFY_INCLUDE_DIRS})
    endif()
    target_include_directories(test_cache PRIVATE ${LIBCURL_INCLUDE_DIRS})
    if(WIN32)
        target_link_libraries(test_cache muslimtify_lib ole32 runtimeobject ${LIBCURL_LIBRARIES} m)
    else()
        target_link_libraries(test_cache muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
    endif()
    add_test(NAME cache COMMAND test_cache)
```

- [ ] **Step 3: Build and verify tests fail (cache.c doesn't exist yet)**

```bash
cd build && cmake .. && cmake --build . -j$(nproc) 2>&1
```

Expected: linker errors for undefined `cache_build_triggers`, `cache_remove_trigger`, etc.

- [ ] **Step 4: Commit**

```bash
git add tests/test_cache.c CMakeLists.txt
git commit -m "test: add failing tests for prayer cache module"
```

---

### Task 3: Implement cache module

**Files:**
- Create: `src/cache.c`
- Modify: `CMakeLists.txt:65-78` — add `src/cache.c` to `muslimtify_lib`

- [ ] **Step 1: Write cache.c**

```c
#include "../include/cache.h"
#include "json.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

static char cache_path_buf[512] = {0};

const char *cache_get_path(void) {
    if (cache_path_buf[0] != '\0') {
        return cache_path_buf;
    }

    const char *xdg_cache = getenv("XDG_CACHE_HOME");
    const char *home = getenv("HOME");

    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw)
            home = pw->pw_dir;
    }

    if (xdg_cache) {
        snprintf(cache_path_buf, sizeof(cache_path_buf),
                 "%s/muslimtify/next_prayer.json", xdg_cache);
    } else if (home) {
        snprintf(cache_path_buf, sizeof(cache_path_buf),
                 "%s/.cache/muslimtify/next_prayer.json", home);
    }

    return cache_path_buf;
}

static int ensure_cache_dir(void) {
    char dir_path[512];
    const char *path = cache_get_path();

    snprintf(dir_path, sizeof(dir_path), "%s", path);
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
    }

    for (char *p = dir_path + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(dir_path, 0755) != 0 && errno != EEXIST)
                return -1;
            *p = '/';
        }
    }
    if (mkdir(dir_path, 0755) != 0 && errno != EEXIST)
        return -1;

    return 0;
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0) { fclose(f); return NULL; }
    fseek(f, 0, SEEK_SET);

    char *content = malloc((size_t)size + 1);
    if (!content) { fclose(f); return NULL; }

    size_t n = fread(content, 1, (size_t)size, f);
    content[n] = '\0';
    fclose(f);
    return content;
}

int cache_load(PrayerCache *cache) {
    if (!cache) return -1;

    const char *path = cache_get_path();
    char *content = read_file(path);
    if (!content) return -1;

    JsonContext *ctx = json_begin();
    if (!ctx) { free(content); return -1; }

    memset(cache, 0, sizeof(*cache));

    char *date_str = get_value(ctx, "date", content);
    if (!date_str) { json_end(ctx); free(content); return -1; }
    strncpy(cache->date, date_str, sizeof(cache->date) - 1);

    char *triggers = get_value(ctx, "triggers", content);
    if (!triggers || triggers[0] != '[') {
        json_end(ctx); free(content); return -1;
    }

    // Parse trigger array manually
    // Format: [{"prayer":"X","minute":N,"minutes_before":N,"prayer_time":F}, ...]
    char *p = triggers + 1; // skip '['
    cache->trigger_count = 0;

    while (*p && *p != ']' && cache->trigger_count < MAX_TRIGGERS) {
        // Find next '{'
        while (*p && *p != '{') p++;
        if (!*p) break;

        // Find matching '}'
        char *obj_start = p;
        int depth = 0;
        char *obj_end = NULL;
        for (char *q = p; *q; q++) {
            if (*q == '{') depth++;
            else if (*q == '}') {
                depth--;
                if (depth == 0) { obj_end = q; break; }
            }
        }
        if (!obj_end) break;

        // Null-terminate the object temporarily
        char saved = *(obj_end + 1);
        *(obj_end + 1) = '\0';

        CacheTrigger *t = &cache->triggers[cache->trigger_count];

        char *prayer = get_value(ctx, "prayer", obj_start);
        if (prayer) {
            strncpy(t->prayer, prayer, sizeof(t->prayer) - 1);
        }

        char *minute_str = get_value(ctx, "minute", obj_start);
        if (minute_str) t->minute = atoi(minute_str);

        char *mb_str = get_value(ctx, "minutes_before", obj_start);
        if (mb_str) t->minutes_before = atoi(mb_str);

        char *pt_str = get_value(ctx, "prayer_time", obj_start);
        if (pt_str) t->prayer_time = atof(pt_str);

        cache->trigger_count++;

        *(obj_end + 1) = saved;
        p = obj_end + 1;
    }

    json_end(ctx);
    free(content);
    return 0;
}

int cache_save(const PrayerCache *cache) {
    if (!cache) return -1;
    if (ensure_cache_dir() != 0) return -1;

    const char *path = cache_get_path();
    char tmp_path[520];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    FILE *f = fopen(tmp_path, "w");
    if (!f) return -1;

    fprintf(f, "{\n");
    fprintf(f, "  \"date\": \"%s\",\n", cache->date);
    fprintf(f, "  \"triggers\": [\n");

    for (int i = 0; i < cache->trigger_count; i++) {
        const CacheTrigger *t = &cache->triggers[i];
        fprintf(f, "    {\"prayer\": \"%s\", \"minute\": %d, "
                   "\"minutes_before\": %d, \"prayer_time\": %.4f}%s\n",
                t->prayer, t->minute, t->minutes_before, t->prayer_time,
                i < cache->trigger_count - 1 ? "," : "");
    }

    fprintf(f, "  ]\n");
    fprintf(f, "}\n");

    if (ferror(f) || fflush(f) != 0 || fclose(f) != 0) {
        remove(tmp_path);
        return -1;
    }

    if (rename(tmp_path, path) != 0) {
        remove(tmp_path);
        return -1;
    }

    return 0;
}

void cache_invalidate(void) {
    const char *path = cache_get_path();
    unlink(path);
}

void cache_reset_path(void) {
    cache_path_buf[0] = '\0';
}

static int compare_triggers(const void *a, const void *b) {
    const CacheTrigger *ta = (const CacheTrigger *)a;
    const CacheTrigger *tb = (const CacheTrigger *)b;
    return ta->minute - tb->minute;
}

int cache_build_triggers(PrayerCache *cache,
                         const Config *cfg,
                         const struct PrayerTimes *times,
                         int current_minute,
                         const char *date_str) {
    if (!cache || !cfg || !times || !date_str) return 0;

    memset(cache, 0, sizeof(*cache));
    strncpy(cache->date, date_str, sizeof(cache->date) - 1);

    PrayerType prayer_types[] = {
        PRAYER_FAJR, PRAYER_SUNRISE, PRAYER_DHUHA,
        PRAYER_DHUHR, PRAYER_ASR, PRAYER_MAGHRIB, PRAYER_ISHA
    };

    for (int i = 0; i < 7; i++) {
        PrayerType type = prayer_types[i];
        if (!prayer_is_enabled(cfg, type)) continue;

        double pt = prayer_get_time(times, type);
        int prayer_min = (int)ceil(pt * 60.0);
        const char *name = prayer_get_name(type);
        const PrayerConfig *pcfg = prayer_get_config(cfg, type);

        // Add exact prayer time
        if (prayer_min >= current_minute &&
            cache->trigger_count < MAX_TRIGGERS) {
            CacheTrigger *t = &cache->triggers[cache->trigger_count];
            strncpy(t->prayer, name, sizeof(t->prayer) - 1);
            t->minute = prayer_min;
            t->minutes_before = 0;
            t->prayer_time = pt;
            cache->trigger_count++;
        }

        // Add reminders
        for (int j = 0; j < pcfg->reminder_count; j++) {
            int reminder_min = prayer_min - pcfg->reminders[j];
            if (reminder_min < 0) reminder_min += 24 * 60;

            if (reminder_min >= current_minute &&
                cache->trigger_count < MAX_TRIGGERS) {
                CacheTrigger *t = &cache->triggers[cache->trigger_count];
                strncpy(t->prayer, name, sizeof(t->prayer) - 1);
                t->minute = reminder_min;
                t->minutes_before = pcfg->reminders[j];
                t->prayer_time = pt;
                cache->trigger_count++;
            }
        }
    }

    // Sort triggers by minute ascending
    if (cache->trigger_count > 1) {
        qsort(cache->triggers, (size_t)cache->trigger_count,
              sizeof(CacheTrigger), compare_triggers);
    }

    return cache->trigger_count;
}

void cache_remove_trigger(PrayerCache *cache, int index) {
    if (!cache || index < 0 || index >= cache->trigger_count) return;

    for (int i = index; i < cache->trigger_count - 1; i++) {
        cache->triggers[i] = cache->triggers[i + 1];
    }
    cache->trigger_count--;
}
```

- [ ] **Step 2: Add cache.c to muslimtify_lib in CMakeLists.txt**

In `CMakeLists.txt` line 65-78, add `src/cache.c` to the `add_library(muslimtify_lib OBJECT ...)` list.

- [ ] **Step 3: Build and run tests**

```bash
cd build && cmake .. && cmake --build . -j$(nproc) && ctest -R cache --output-on-failure
```

Expected: All cache tests pass.

- [ ] **Step 4: Run full test suite**

```bash
cd build && ctest --output-on-failure
```

Expected: All tests pass (no regressions).

- [ ] **Step 5: Commit**

```bash
git add src/cache.c CMakeLists.txt
git commit -m "feat: implement prayer time cache module"
```

---

### Task 4: Integrate cache into handle_check

**Files:**
- Modify: `src/cmd_show.c:54-95`

- [ ] **Step 1: Rewrite handle_check to use cache**

Replace the existing `handle_check` function in `src/cmd_show.c`:

```c
int handle_check(int argc, char **argv) {
  (void)argc;
  (void)argv;

  Config cfg;
  if (config_load(&cfg) != 0) {
    fprintf(stderr, "Error: Failed to load config\n");
    return 1;
  }

  if (ensure_location(&cfg) != 0) {
    return 1;
  }

  time_t now = time(NULL);
  struct tm *tm_now = localtime(&now);
  if (!tm_now) {
    fprintf(stderr, "Error: Failed to get current time\n");
    return 1;
  }

  int current_min = tm_now->tm_hour * 60 + tm_now->tm_min;
  char today[16];
  snprintf(today, sizeof(today), "%04d-%02d-%02d",
           tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday);

  // Try to load cache
  PrayerCache cache;
  bool cache_valid = (cache_load(&cache) == 0 &&
                      strcmp(cache.date, today) == 0 &&
                      cache.trigger_count > 0);

  if (!cache_valid) {
    // Recalculate and build cache
    struct PrayerTimes times = calculate_prayer_times(
        tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
        cfg.latitude, cfg.longitude, cfg.timezone_offset);

    cache_build_triggers(&cache, &cfg, &times, current_min, today);
    cache_save(&cache);
  }

  // Fire all triggers matching current minute
  bool notified = false;
  int i = 0;
  while (i < cache.trigger_count) {
    if (cache.triggers[i].minute == current_min) {
      if (!notified) {
        if (!notify_init_once("Muslimtify")) {
          fprintf(stderr, "Error: Failed to initialize notification system\n");
          return 1;
        }
        notified = true;
      }

      char time_str[16];
      format_time_hm(cache.triggers[i].prayer_time, time_str, sizeof(time_str));
      notify_prayer(cache.triggers[i].prayer, time_str,
                    cache.triggers[i].minutes_before,
                    cfg.notification_urgency);

      cache_remove_trigger(&cache, i);
      // Don't increment i — next element shifted into current position
    } else {
      i++;
    }
  }

  if (notified) {
    notify_cleanup();
    cache_save(&cache);
  }

  return 0;
}
```

- [ ] **Step 2: Add cache.h include to cmd_show.c**

Add `#include "cache.h"` to the includes at the top of `src/cmd_show.c`.

- [ ] **Step 3: Build and run all tests**

```bash
cd build && cmake .. && cmake --build . -j$(nproc) && ctest --output-on-failure
```

Expected: All tests pass.

- [ ] **Step 4: Manual smoke test**

```bash
# Show prayer times to verify calculation still works
./build/bin/muslimtify show

# Run check — should create cache file
./build/bin/muslimtify check

# Verify cache was created
cat ~/.cache/muslimtify/next_prayer.json
```

Expected: Cache file exists with today's date and trigger list.

- [ ] **Step 5: Commit**

```bash
git add src/cmd_show.c
git commit -m "feat: integrate prayer cache into check command"
```

---

### Task 5: Invalidate cache on config changes

**Files:**
- Modify: `src/cmd_prayer.c`

- [ ] **Step 1: Add cache invalidation to handle_enable**

Add `#include "cache.h"` to the top of `src/cmd_prayer.c`. Then add `cache_invalidate();` right before each `printf("✓ ...")` success line in `handle_enable` (both the "all" branch and the single-prayer branch).

- [ ] **Step 2: Add cache invalidation to handle_disable**

Same pattern: add `cache_invalidate();` before each success printf in `handle_disable`.

- [ ] **Step 3: Add cache invalidation to handle_reminder**

Same pattern: add `cache_invalidate();` before each success printf in `handle_reminder` (the "all" branch, the single-prayer branch, and the "clear" paths).

- [ ] **Step 4: Build and run all tests**

```bash
cd build && cmake .. && cmake --build . -j$(nproc) && ctest --output-on-failure
```

Expected: All tests pass.

- [ ] **Step 5: Manual smoke test**

```bash
# Create cache
./build/bin/muslimtify check
ls ~/.cache/muslimtify/next_prayer.json  # should exist

# Disable a prayer — cache should be deleted
./build/bin/muslimtify disable fajr
ls ~/.cache/muslimtify/next_prayer.json  # should not exist

# Re-enable
./build/bin/muslimtify enable fajr
```

- [ ] **Step 6: Commit**

```bash
git add src/cmd_prayer.c
git commit -m "feat: invalidate prayer cache on config changes"
```

---

### Task 6: Invalidate cache on config set commands

**Files:**
- Modify: `src/cmd_config.c`

- [ ] **Step 1: Check if cmd_config.c modifies prayer-related settings**

Read `src/cmd_config.c` to identify if it can change location (latitude/longitude/timezone) which would affect prayer time calculations.

- [ ] **Step 2: Add cache invalidation after config saves that affect prayer times**

Add `#include "cache.h"` and `cache_invalidate();` after any `config_save()` call in `cmd_config.c` that modifies location or calculation settings.

- [ ] **Step 3: Similarly check cmd_location.c**

If `cmd_location.c` saves config after detecting/setting location, add `cache_invalidate()` there too.

- [ ] **Step 4: Build and run all tests**

```bash
cd build && cmake .. && cmake --build . -j$(nproc) && ctest --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/cmd_config.c src/cmd_location.c
git commit -m "feat: invalidate cache on location and config changes"
```
