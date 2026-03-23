# Config Module Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Refactor the config module to rely on the shared string helper layer, resolve MSVC warnings, and keep prayer time tests portable.

**Architecture:** `src/config.c` will include `string_util.h`, add a single truncation logger helper, and route every string copy/append/tokenization path through the helper API. Reminder formatting/parsing adopts the new semantics, and the test suite gains a cross-toolchain `PARSE_TIME` macro. No new files are created; only behavior-preserving refactors occur.

**Tech Stack:** C99, CMake, muslimtify_lib, string_util helpers, ctest, MSVC/MinGW toolchains.

---

### Task 1: Wire config.c into string helpers

**Files:**
- Modify: `src/config.c:1-215`
- Test: `build/bin/Debug/muslimtify.exe config show` (manual), `ctest --test-dir build -C Debug -R config --output-on-failure` (when on Linux)

- [ ] **Step 1: Introduce helper include and truncation logger**

```c
#include "string_util.h"

static bool config_trunc_logged = false;

static void log_truncation(const char *key) {
  if (!config_trunc_logged) {
    fprintf(stderr, "config: value '%s' truncated\n", key);
    config_trunc_logged = true;
  }
}
```

- [ ] **Step 2: Replace `strcpy` defaults with `copy_string`**

Update `config_default` so timezone, notification fields, calculation method, etc., call `copy_string`. When a call returns `false`, invoke `log_truncation("timezone")` (matching the field name) but keep the truncated value.

- [ ] **Step 3: Swap `strncpy` usage in `config_load`**

For every location/notification/calculation string assignment, call `copy_string`. On truncation, call `log_truncation` with the JSON key (`"timezone"`, `"city"`, etc.).

- [ ] **Step 4: Handle reminder arrays without manual copying**

When parsing JSON reminders, zero the count before filling, and leave the existing numeric parsing intact. No string helpers here, but make sure the truncation logger is available for future steps.

- [ ] **Step 5: Replace `strerror` chains with `errno_string`**

Add a local buffer (e.g., `char errbuf[128];`) in `config_save` before each `fprintf`. Call `errno_string(errno, errbuf, sizeof(errbuf));` and pass `errbuf` to `fprintf`. Ignore the boolean return beyond optionally logging when it fails (existing logger covers truncation only). Command reference: `errno_string` already lives in `string_util.c`.

- [ ] **Step 6: Verify build for compilation errors**

Run `cmake --build build --config Debug` (or `cmake --build build` on Linux) to ensure the helper usage compiles cleanly.

### Task 2: Refactor reminder formatting

**Files:**
- Modify: `src/config.c:505-525`

- [ ] **Step 1: Replace `strncpy/strncat` calls**

When no reminders exist, switch to `copy_string(buffer, bufsize, "none")` and log truncation if it returns false.

- [ ] **Step 2: Append reminders via helper**

Loop through reminders, writing into a scratch `temp` string via `snprintf` as today. Call `append_string(buffer, bufsize, temp)` and stop iterating immediately if it returns false. After each numeric append (except the last), call `append_string(buffer, bufsize, ",")` and likewise stop on failure. Reuse `log_truncation("reminders")` whenever truncation occurs.

### Task 3: Switch reminder parsing to `parse_tokens`

**Files:**
- Modify: `src/config.c:474-503`

- [ ] **Step 1: Define callback payload struct**

Add a local struct inside `config_parse_reminders`:

```c
typedef struct {
  int *reminders;
  int max;
  int count;
} ReminderCtx;
```

- [ ] **Step 2: Implement callback**

Add a static helper that trims leading spaces, calls `strtol`, validates `0 < value <= 1440`, and appends to the array. When the array is full, return `false` so `parse_tokens` yields `-2`. Return `true` otherwise.

- [ ] **Step 3: Call `parse_tokens`**

Reuse the existing 256-byte buffer as `scratch`. For delimiters, pass `", "`. Interpret return values: `-1` ⇒ invalid input (return `-1`), `-2` ⇒ array filled (return `ctx.count`), `0` ⇒ success (return `ctx.count`).

### Task 4: Resolve MSVC C4244 in `config_get_prayer`

**Files:**
- Modify: `src/config.c:444-471`

- [ ] **Step 1: Capture `tolower` output in `int tmp`**

During the loop, assign `int tmp = tolower((unsigned char)name_lower[i]);` then set `name_lower[i] = (char)tmp;` to avoid implicit narrowing warnings. Include the explicit cast to `unsigned char` for correctness.

### Task 5: Harden `tests/test_prayertimes.c`

**Files:**
- Modify: `tests/test_prayertimes.c:1-20`
- Test: `ctest --test-dir build -C Debug -R prayertimes --output-on-failure`

- [ ] **Step 1: Introduce PARSE_TIME macro**

Insert:

```c
#if defined(_MSC_VER)
#define PARSE_TIME(h, m, str) sscanf_s((str), "%d:%d", &(h), &(m))
#else
#define PARSE_TIME(h, m, str) sscanf((str), "%d:%d", &(h), &(m))
#endif
```

- [ ] **Step 2: Update `time_to_minutes`**

Replace the direct `sscanf` call with `PARSE_TIME`. Optionally assert or ignore the return value per existing behavior.

- [ ] **Step 3: Run targeted tests**

`cmake --build build --config Debug && ctest --test-dir build -C Debug -R prayertimes --output-on-failure`

### Task 6: Manual + automated verification

**Files:**
- Test only

- [ ] **Step 1: Linux config test run (if available)**

`ctest --test-dir build -C Debug -R config --output-on-failure`

- [ ] **Step 2: Windows sanity command (always run here)**

Run `build/bin/Debug/muslimtify.exe config show` to confirm the CLI still prints reminder data and truncated strings look correct.

---

Plan review checklist:

- Confirm truncation logging uses a single static flag.
- Ensure `parse_tokens` result handling matches spec semantics.
- Validate test commands cover both reminder parsing and prayer-time parsing paths.
