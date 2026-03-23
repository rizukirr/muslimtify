# Multiplatform Boundary Hardening Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Harden the shared-vs-platform boundary for Linux and Windows support without changing the current CLI surface or destabilizing existing Linux behavior.

**Architecture:** Keep the current file layout, preserve separate Linux and Windows daemon/notification modules, and route shared modules through `platform.h` for OS-dependent services. Phase 1 changes are limited to boundary cleanup, targeted tests, and Windows daemon documentation.

**Tech Stack:** C99, CMake 3.22+, Linux systemd/libnotify/libcurl, Windows Task Scheduler/WinRT/MSVC, existing custom C test binaries run through `ctest`

---

## File Structure

### Existing files to modify

- `include/platform.h`
  - Keep the platform service surface narrow and concrete.
  - Add any missing declarations only if they are needed by multiple shared modules.
- `src/platform_linux.c`
  - Own Linux-specific path, file, time, tty, and process helpers.
- `src/platform_win.c`
  - Own Windows-specific path, file, time, tty, and process helpers.
- `src/config.c`
  - Remove any remaining direct POSIX assumptions from shared config persistence code.
- `src/cache.c`
  - Remove any remaining direct POSIX assumptions from shared cache persistence code.
- `src/display.c`
  - Route time and tty-dependent behavior through `platform.h`.
- `src/notification.c`
  - Keep Linux notification behavior intact while using platform helpers where appropriate.
- `src/cmd_daemon.c`
  - Keep Linux systemd behavior, but move any generic path or directory logic to `platform.h` where it improves reuse.
- `src/cmd_daemon_win.c`
  - Keep Windows Task Scheduler behavior isolated and improve task naming/status UX only if it does not expand scope.
- `src/cli.c`
  - Keep the platform-specific daemon help text aligned with actual behavior.
- `CMakeLists.txt`
  - Keep compile-time platform source selection explicit and stable.
- `README.md`
  - Document the Windows scheduler flow using the same `muslimtify daemon ...` command surface.
- `CHANGELOG.md`
  - Record the user-visible Windows scheduling documentation/boundary cleanup.

### Existing tests to modify

- `tests/test_config.c`
  - Add regression coverage for shared config behavior that depends on platform path routing.
- `tests/test_cache.c`
  - Add regression coverage for shared cache behavior that depends on platform path routing.

### New tests to create

- `tests/test_platform.c`
  - Cover cross-platform-safe expectations for the narrow `platform.h` API that can be asserted in the current host environment.

## Task 1: Lock Down The Platform Boundary Surface

**Files:**
- Modify: `include/platform.h`
- Modify: `src/platform_linux.c`
- Modify: `src/platform_win.c`
- Test: `tests/test_platform.c`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing platform boundary test**

```c
#include "platform.h"
#include <stdio.h>
#include <string.h>

static int failed = 0;

static void check_bool(const char *name, int cond) {
  if (!cond) {
    failed++;
    fprintf(stderr, "FAIL [%s]\n", name);
  }
}

int main(void) {
  const char *config_dir = platform_config_dir();
  const char *cache_dir = platform_cache_dir();
  const char *home_dir = platform_home_dir();
  const char *exe_dir = platform_exe_dir();

  check_bool("config dir non-empty", config_dir != NULL && config_dir[0] != '\0');
  check_bool("cache dir non-empty", cache_dir != NULL && cache_dir[0] != '\0');
  check_bool("home dir non-empty", home_dir != NULL && home_dir[0] != '\0');
  check_bool("exe dir non-empty", exe_dir != NULL && exe_dir[0] != '\0');
  check_bool("config dir cached", platform_config_dir() == config_dir);
  check_bool("cache dir cached", platform_cache_dir() == cache_dir);
  check_bool("home dir cached", platform_home_dir() == home_dir);
  check_bool("exe dir cached", platform_exe_dir() == exe_dir);

  return failed ? 1 : 0;
}
```

- [ ] **Step 2: Run the new test to verify it fails**

Run: `cmake --build build --target test_platform`

Expected: target build fails because `tests/test_platform.c` is not added to `CMakeLists.txt` yet.

- [ ] **Step 3: Add the test target to CMake**

```cmake
add_executable(test_platform tests/test_platform.c)
muslimtify_set_target_defaults(test_platform)
target_link_libraries(test_platform muslimtify_lib)
add_test(NAME platform COMMAND test_platform)
```

- [ ] **Step 4: Implement the minimal platform fixes**

Implementation checklist:
- ensure all declared `platform.h` functions exist on both platforms
- keep return values and caching behavior consistent
- add a `platform_exe_path()` declaration and implementation only if the daemon or notification work needs the full executable path in more than one place
- do not broaden the API beyond current shared-module needs

If `platform_exe_path()` is added, use a narrow declaration:

```c
const char *platform_exe_path(void);
```

- [ ] **Step 5: Run the platform test**

Run: `ctest --test-dir build -R platform --output-on-failure`

Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add include/platform.h src/platform_linux.c src/platform_win.c tests/test_platform.c CMakeLists.txt
git commit -m "test: add platform boundary coverage"
```

## Task 2: Remove Direct OS Calls From Shared Persistence Modules

**Files:**
- Modify: `src/config.c`
- Modify: `src/cache.c`
- Test: `tests/test_config.c`
- Test: `tests/test_cache.c`

- [ ] **Step 1: Write failing regression checks for shared persistence behavior**

Add focused checks to existing tests:

```c
check_bool("config path uses platform dir", strstr(config_get_path(), "muslimtify") != NULL);
```

```c
check_bool("cache path uses platform dir", strstr(cache_get_path(), "muslimtify") != NULL);
```

Also add assertions that saving still works after calling any path-reset helper already present in the module.

- [ ] **Step 2: Run the targeted tests**

Run: `ctest --test-dir build -R "config|cache" --output-on-failure`

Expected: if shared modules still depend on direct POSIX path handling, one or both tests fail or expose inconsistent path construction.

- [ ] **Step 3: Refactor `src/config.c` to use `platform.h` exclusively for OS-dependent path/file operations**

Implementation checklist:
- replace direct home/config-dir resolution with `platform_config_dir()`
- replace direct existence checks with `platform_file_exists()`
- replace direct directory creation with `platform_mkdir_p()`
- replace direct rename/delete usage with `platform_atomic_rename()` / `platform_file_delete()` where applicable
- keep JSON/config semantics unchanged

- [ ] **Step 4: Refactor `src/cache.c` to use `platform.h` exclusively for OS-dependent path/file operations**

Implementation checklist:
- replace direct cache-dir resolution with `platform_cache_dir()`
- replace direct filesystem primitives with platform wrappers
- keep cache file format and cache invalidation behavior unchanged

- [ ] **Step 5: Run the targeted tests again**

Run: `ctest --test-dir build -R "config|cache" --output-on-failure`

Expected: PASS on Linux

- [ ] **Step 6: Commit**

```bash
git add src/config.c src/cache.c tests/test_config.c tests/test_cache.c
git commit -m "refactor: route persistence through platform helpers"
```

## Task 3: Remove Direct OS Calls From Shared Display Logic

**Files:**
- Modify: `src/display.c`
- Test: existing display-related behavior through current CLI-oriented tests where applicable

- [ ] **Step 1: Add a focused failing assertion in the nearest existing test**

If there is no dedicated display test, extend the nearest existing test that exercises formatted output without expanding scope. Prefer a small assertion around stable non-OS behavior, for example:

```c
check_bool("display path remains stable", some_output_buffer[0] != '\0');
```

If no useful automated assertion can be added without large harness work, explicitly document that this task is verified by build + existing CLI/display consumers instead of inventing a weak test.

- [ ] **Step 2: Refactor `src/display.c`**

Implementation checklist:
- replace `localtime_r()`/`localtime_s()` branching with `platform_localtime()`
- replace direct tty detection with `platform_isatty(stdout)`
- keep all formatting and Unicode output behavior unchanged

- [ ] **Step 3: Run the affected tests**

Run: `ctest --test-dir build -R "cli|prayer_checker" --output-on-failure`

Expected: PASS on Linux

- [ ] **Step 4: Commit**

```bash
git add src/display.c
git commit -m "refactor: route display system calls through platform layer"
```

## Task 4: Keep Daemon Implementations Separate While Reducing Duplicate Path Logic

**Files:**
- Modify: `src/cmd_daemon.c`
- Modify: `src/cmd_daemon_win.c`
- Modify: `include/platform.h`
- Modify: `src/platform_linux.c`
- Modify: `src/platform_win.c`

- [ ] **Step 1: Write a small failing regression test or static compile target for daemon path helpers**

Preferred route:
- if a narrow helper like `platform_exe_path()` is introduced, cover it in `tests/test_platform.c`
- avoid trying to fully integration-test systemd or `schtasks.exe` in unit tests

Example assertion:

```c
const char *exe_path = platform_exe_path();
check_bool("exe path has filename", exe_path != NULL && strstr(exe_path, "muslimtify") != NULL);
```

- [ ] **Step 2: Run the relevant test**

Run: `ctest --test-dir build -R platform --output-on-failure`

Expected: FAIL until the helper is added and wired up.

- [ ] **Step 3: Reduce duplicated executable-path logic**

Implementation checklist:
- keep `src/cmd_daemon.c` Linux-specific and `src/cmd_daemon_win.c` Windows-specific
- extract only reusable path lookup into `platform.h` if both modules need it
- do not merge systemd and Task Scheduler behavior into one source file
- keep CLI surface and install/uninstall/status semantics unchanged

- [ ] **Step 4: Run affected builds/tests**

Run: `cmake --build build`

Expected: build succeeds

- [ ] **Step 5: Commit**

```bash
git add include/platform.h src/platform_linux.c src/platform_win.c src/cmd_daemon.c src/cmd_daemon_win.c tests/test_platform.c
git commit -m "refactor: share daemon path helpers only"
```

## Task 5: Keep Linux Notification Backend Linux-Specific But Platform-Aware

**Files:**
- Modify: `src/notification.c`
- Test: manual or existing CLI/check-path validation

- [ ] **Step 1: Identify the remaining non-shared assumptions**

Current likely candidates:
- `getcwd()`
- raw `'/'` path joins
- direct path assumptions outside `platform.h`

- [ ] **Step 2: Refactor only the reusable parts**

Implementation checklist:
- use `platform_exe_dir()` or `platform_exe_path()` where appropriate
- use `PLATFORM_PATH_SEP` or helper-based joins only if it improves clarity
- leave libnotify-specific behavior in `src/notification.c`
- do not try to unify Linux and Windows notification code

- [ ] **Step 3: Verify the app still builds and notification call sites still work**

Run: `cmake --build build`

Expected: build succeeds

Manual verification on Linux:

```bash
build/bin/muslimtify check
```

Expected: command runs without regression; if the current time matches a prayer/reminder, a notification appears.

- [ ] **Step 4: Commit**

```bash
git add src/notification.c
git commit -m "refactor: tighten linux notification platform boundary"
```

## Task 6: Document Windows Scheduler Behavior Without Changing The CLI

**Files:**
- Modify: `README.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Write the documentation change**

Add a Windows subsection that explains:
- `muslimtify daemon install` creates a Task Scheduler job
- `muslimtify daemon status` queries that job
- `muslimtify daemon uninstall` removes that job
- some systems may require Administrator privileges

Suggested README snippet:

```md
### Windows Scheduler

On Windows, `muslimtify daemon install` does not create a background service.
It registers a Task Scheduler task that runs `muslimtify check` every minute.
```

- [ ] **Step 2: Verify docs stay aligned with the current CLI**

Run: `rg "daemon install|daemon status|daemon uninstall|systemd|Task Scheduler" README.md CHANGELOG.md src/cli.c src/cmd_daemon_win.c src/cmd_daemon.c`

Expected: wording is internally consistent.

- [ ] **Step 3: Commit**

```bash
git add README.md CHANGELOG.md
git commit -m "docs: clarify windows daemon behavior"
```

## Task 7: Final Verification

**Files:**
- Modify: none

- [ ] **Step 1: Configure and build**

Run: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`

Expected: configure succeeds

Run: `cmake --build build`

Expected: build succeeds

- [ ] **Step 2: Run Linux test suite**

Run: `ctest --test-dir build --output-on-failure`

Expected: all enabled tests pass on Linux

- [ ] **Step 3: Sanity-check daemon commands**

Run:

```bash
build/bin/muslimtify daemon status
build/bin/muslimtify help
```

Expected:
- help text still presents `daemon [install|uninstall|status]`
- status command still uses the Linux implementation on Linux

- [ ] **Step 4: Review worktree state**

Run: `git status --short`

Expected: only intentional plan-related or implementation-related changes remain

- [ ] **Step 5: Final commit or squash-by-topic only if needed**

```bash
git log --oneline -5
```

Expected: commits reflect narrow, reviewable boundary-hardening steps

## Notes For Execution

- Do not mix large file moves into this plan.
- Do not merge Linux and Windows daemon logic into one implementation.
- Do not invent a broad platform abstraction that current modules do not need.
- Respect unrelated local changes in `CMakeLists.txt` and `src/display.c`; integrate carefully instead of overwriting.
