# Windows Port Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make muslimtify build and run on Windows with full feature parity — platform abstraction, Task Scheduler daemon, libcurl bundling, and Windows CI.

**Architecture:** New `platform.h` header with `platform_linux.c` / `platform_win.c` backends. Existing consumers refactored to use platform API. New `cmd_daemon_win.c` for Windows Task Scheduler. libcurl via FetchContent on Windows. Test guards for POSIX-only tests.

**Tech Stack:** C99, MSVC, Win32 API, WinRT (existing), CMake FetchContent, schtasks.exe

**Spec:** `docs/superpowers/specs/2026-03-22-windows-port-design.md`

---

## File Structure

| Action | File | Responsibility |
|--------|------|----------------|
| Create | `include/platform.h` | Platform abstraction API |
| Create | `src/platform_linux.c` | Linux/POSIX implementation |
| Create | `src/platform_win.c` | Windows implementation |
| Create | `src/cmd_daemon_win.c` | Windows Task Scheduler daemon |
| Modify | `src/config.c` | Use platform API instead of POSIX |
| Modify | `src/cache.c` | Use platform API instead of POSIX |
| Modify | `src/display.c` | Use platform API instead of POSIX |
| Modify | `src/notification.c` | Use platform API instead of POSIX |
| Modify | `src/cli.c` | Remove `_GNU_SOURCE`, platform-conditional help text |
| Modify | `CMakeLists.txt` | Platform sources, FetchContent curl, test guards, link targets |
| Modify | `.github/workflows/ci.yml` | Add Windows build job |

---

### Task 1: Create `include/platform.h`

**Files:**
- Create: `include/platform.h`

- [ ] **Step 1: Create the platform header**

```c
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#define PLATFORM_PATH_MAX 260
#define PLATFORM_PATH_SEP '\\'
#else
#include <limits.h>
#ifdef PATH_MAX
#define PLATFORM_PATH_MAX PATH_MAX
#else
#define PLATFORM_PATH_MAX 4096
#endif
#define PLATFORM_PATH_SEP '/'
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the config directory path (e.g., ~/.config/muslimtify or %APPDATA%\muslimtify).
 * Creates the directory if it doesn't exist. Returns cached static buffer. No trailing separator.
 */
const char *platform_config_dir(void);

/**
 * Returns the cache directory path (e.g., ~/.cache/muslimtify or %LOCALAPPDATA%\muslimtify).
 * Creates the directory if it doesn't exist. Returns cached static buffer. No trailing separator.
 */
const char *platform_cache_dir(void);

/**
 * Returns the user's home directory. Returns cached static buffer. No trailing separator.
 */
const char *platform_home_dir(void);

/**
 * Returns the directory containing the running executable. No trailing separator.
 */
const char *platform_exe_dir(void);

/**
 * Recursively create directories (like mkdir -p). Returns 0 on success, -1 on failure.
 */
int platform_mkdir_p(const char *path);

/**
 * Check if a file exists. Returns 1 if exists, 0 otherwise.
 */
int platform_file_exists(const char *path);

/**
 * Delete a file. Returns 0 on success, -1 on failure.
 */
int platform_file_delete(const char *path);

/**
 * Atomically rename a file (replaces destination if it exists).
 * Returns 0 on success, -1 on failure.
 */
int platform_atomic_rename(const char *src, const char *dst);

/**
 * Thread-safe localtime. Wraps localtime_r (POSIX) or localtime_s (MSVC).
 */
void platform_localtime(const time_t *t, struct tm *result);

/**
 * Check if a FILE stream is a terminal. Returns 1 if tty, 0 otherwise.
 */
int platform_isatty(FILE *stream);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_H */
```

- [ ] **Step 2: Commit**

```bash
git add include/platform.h
git commit -m "feat: add platform abstraction header"
```

---

### Task 2: Create `src/platform_linux.c`

**Files:**
- Create: `src/platform_linux.c`

- [ ] **Step 1: Create the Linux platform implementation**

```c
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "../include/platform.h"
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char config_dir_buf[PLATFORM_PATH_MAX] = {0};
static char cache_dir_buf[PLATFORM_PATH_MAX] = {0};
static char home_dir_buf[PLATFORM_PATH_MAX] = {0};
static char exe_dir_buf[PLATFORM_PATH_MAX] = {0};

const char *platform_home_dir(void) {
  if (home_dir_buf[0] != '\0')
    return home_dir_buf;

  const char *home = getenv("HOME");
  if (!home) {
    struct passwd *pw = getpwuid(getuid());
    if (pw)
      home = pw->pw_dir;
  }
  if (home)
    snprintf(home_dir_buf, sizeof(home_dir_buf), "%s", home);

  return home_dir_buf;
}

const char *platform_config_dir(void) {
  if (config_dir_buf[0] != '\0')
    return config_dir_buf;

  const char *xdg = getenv("XDG_CONFIG_HOME");
  if (xdg) {
    snprintf(config_dir_buf, sizeof(config_dir_buf), "%s/muslimtify", xdg);
  } else {
    const char *home = platform_home_dir();
    if (home[0] != '\0')
      snprintf(config_dir_buf, sizeof(config_dir_buf), "%s/.config/muslimtify", home);
  }

  return config_dir_buf;
}

const char *platform_cache_dir(void) {
  if (cache_dir_buf[0] != '\0')
    return cache_dir_buf;

  const char *xdg = getenv("XDG_CACHE_HOME");
  if (xdg) {
    snprintf(cache_dir_buf, sizeof(cache_dir_buf), "%s/muslimtify", xdg);
  } else {
    const char *home = platform_home_dir();
    if (home[0] != '\0')
      snprintf(cache_dir_buf, sizeof(cache_dir_buf), "%s/.cache/muslimtify", home);
  }

  return cache_dir_buf;
}

const char *platform_exe_dir(void) {
  if (exe_dir_buf[0] != '\0')
    return exe_dir_buf;

  char exe_path[PLATFORM_PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
  if (len > 0) {
    exe_path[len] = '\0';
    /* Strip filename to get directory */
    char *last_slash = strrchr(exe_path, '/');
    if (last_slash) {
      *last_slash = '\0';
      snprintf(exe_dir_buf, sizeof(exe_dir_buf), "%s", exe_path);
    }
  }

  return exe_dir_buf;
}

int platform_mkdir_p(const char *path) {
  char tmp[PLATFORM_PATH_MAX];
  snprintf(tmp, sizeof(tmp), "%s", path);

  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
        return -1;
      *p = '/';
    }
  }
  if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
    return -1;

  return 0;
}

int platform_file_exists(const char *path) {
  return access(path, F_OK) == 0 ? 1 : 0;
}

int platform_file_delete(const char *path) {
  return unlink(path) == 0 ? 0 : -1;
}

int platform_atomic_rename(const char *src, const char *dst) {
  return rename(src, dst) == 0 ? 0 : -1;
}

void platform_localtime(const time_t *t, struct tm *result) {
  localtime_r(t, result);
}

int platform_isatty(FILE *stream) {
  return isatty(fileno(stream));
}
```

- [ ] **Step 2: Commit**

```bash
git add src/platform_linux.c
git commit -m "feat: add Linux platform implementation"
```

---

### Task 3: Create `src/platform_win.c`

**Files:**
- Create: `src/platform_win.c`

- [ ] **Step 1: Create the Windows platform implementation**

```c
#include "../include/platform.h"
#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static char config_dir_buf[PLATFORM_PATH_MAX] = {0};
static char cache_dir_buf[PLATFORM_PATH_MAX] = {0};
static char home_dir_buf[PLATFORM_PATH_MAX] = {0};
static char exe_dir_buf[PLATFORM_PATH_MAX] = {0};

const char *platform_home_dir(void) {
  if (home_dir_buf[0] != '\0')
    return home_dir_buf;

  const char *home = getenv("USERPROFILE");
  if (home)
    snprintf(home_dir_buf, sizeof(home_dir_buf), "%s", home);

  return home_dir_buf;
}

const char *platform_config_dir(void) {
  if (config_dir_buf[0] != '\0')
    return config_dir_buf;

  const char *appdata = getenv("APPDATA");
  if (appdata) {
    snprintf(config_dir_buf, sizeof(config_dir_buf), "%s\\muslimtify", appdata);
  } else {
    const char *home = platform_home_dir();
    if (home[0] != '\0')
      snprintf(config_dir_buf, sizeof(config_dir_buf), "%s\\AppData\\Roaming\\muslimtify", home);
  }

  return config_dir_buf;
}

const char *platform_cache_dir(void) {
  if (cache_dir_buf[0] != '\0')
    return cache_dir_buf;

  const char *localappdata = getenv("LOCALAPPDATA");
  if (localappdata) {
    snprintf(cache_dir_buf, sizeof(cache_dir_buf), "%s\\muslimtify", localappdata);
  } else {
    const char *home = platform_home_dir();
    if (home[0] != '\0')
      snprintf(cache_dir_buf, sizeof(cache_dir_buf), "%s\\AppData\\Local\\muslimtify", home);
  }

  return cache_dir_buf;
}

const char *platform_exe_dir(void) {
  if (exe_dir_buf[0] != '\0')
    return exe_dir_buf;

  char exe_path[PLATFORM_PATH_MAX];
  DWORD len = GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
  if (len > 0 && len < sizeof(exe_path)) {
    /* Strip filename to get directory */
    char *last_sep = strrchr(exe_path, '\\');
    if (!last_sep)
      last_sep = strrchr(exe_path, '/');
    if (last_sep) {
      *last_sep = '\0';
      snprintf(exe_dir_buf, sizeof(exe_dir_buf), "%s", exe_path);
    }
  }

  return exe_dir_buf;
}

int platform_mkdir_p(const char *path) {
  char tmp[PLATFORM_PATH_MAX];
  snprintf(tmp, sizeof(tmp), "%s", path);

  for (char *p = tmp + 1; *p; p++) {
    if (*p == '\\' || *p == '/') {
      char saved = *p;
      *p = '\0';
      if (CreateDirectoryA(tmp, NULL) == 0) {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
          return -1;
      }
      *p = saved;
    }
  }
  if (CreateDirectoryA(tmp, NULL) == 0) {
    if (GetLastError() != ERROR_ALREADY_EXISTS)
      return -1;
  }

  return 0;
}

int platform_file_exists(const char *path) {
  DWORD attrs = GetFileAttributesA(path);
  return attrs != INVALID_FILE_ATTRIBUTES ? 1 : 0;
}

int platform_file_delete(const char *path) {
  return DeleteFileA(path) != 0 ? 0 : -1;
}

int platform_atomic_rename(const char *src, const char *dst) {
  return MoveFileExA(src, dst, MOVEFILE_REPLACE_EXISTING) != 0 ? 0 : -1;
}

void platform_localtime(const time_t *t, struct tm *result) {
  localtime_s(result, t);
}

int platform_isatty(FILE *stream) {
  return _isatty(_fileno(stream));
}
```

- [ ] **Step 2: Commit**

```bash
git add src/platform_win.c
git commit -m "feat: add Windows platform implementation"
```

---

### Task 4: Add platform sources to CMake

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add platform source to object library**

In the `add_library(muslimtify_lib OBJECT ...)` block, add after the notification line:

```cmake
    $<IF:$<BOOL:${WIN32}>,src/platform_win.c,src/platform_linux.c>
```

- [ ] **Step 2: Verify Linux build and tests**

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc) && cd build && ctest --output-on-failure
```
Expected: all tests pass.

- [ ] **Step 3: Commit**

```bash
git add CMakeLists.txt
git commit -m "chore: add platform source to CMake build"
```

---

### Task 5: Refactor `config.c` to use platform API

**Files:**
- Modify: `src/config.c`

- [ ] **Step 1: Replace includes and path resolution**

Replace the includes at the top:
```c
#define JSON_IMPLEMENTATION
#include "../include/config.h"
#include "json.h"
#include <ctype.h>
#include <errno.h>
#include <linux/limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
```

With:
```c
#define JSON_IMPLEMENTATION
#include "../include/config.h"
#include "../include/platform.h"
#include "json.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
```

- [ ] **Step 2: Replace `config_get_path()`**

Replace the entire `config_get_path()` function with:
```c
const char *config_get_path(void) {
  static char config_path[PLATFORM_PATH_MAX] = {0};
  if (config_path[0] != '\0')
    return config_path;

  const char *dir = platform_config_dir();
  if (dir[0] != '\0')
    snprintf(config_path, sizeof(config_path), "%s%cconfig.json", dir, PLATFORM_PATH_SEP);

  return config_path;
}
```

Also remove the file-level `static char config_path[512] = {0};` declaration since it moves into the function.

- [ ] **Step 3: Replace `ensure_config_dir()`**

Replace the entire `ensure_config_dir()` function with:
```c
static int ensure_config_dir(void) {
  const char *dir = platform_config_dir();
  if (dir[0] == '\0')
    return -1;
  if (platform_mkdir_p(dir) != 0) {
    fprintf(stderr, "Error: Cannot create config directory '%s'\n", dir);
    return -1;
  }
  return 0;
}
```

- [ ] **Step 4: Replace `rename()` with `platform_atomic_rename()`**

In `config_save()`, replace:
```c
  if (rename(tmp_path, path) != 0) {
```
With:
```c
  if (platform_atomic_rename(tmp_path, path) != 0) {
```

- [ ] **Step 5: Replace `PATH_MAX` with `PLATFORM_PATH_MAX`**

Replace all occurrences of `PATH_MAX` in config.c with `PLATFORM_PATH_MAX`.

- [ ] **Step 6: Replace `access()` with `platform_file_exists()`**

In `config_load()`, replace:
```c
  if (access(path, F_OK) != 0) {
```
With:
```c
  if (!platform_file_exists(path)) {
```

- [ ] **Step 7: Verify build and tests**

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc) && cd build && ctest --output-on-failure
```
Expected: all tests pass (especially `config` test).

- [ ] **Step 8: Commit**

```bash
git add src/config.c
git commit -m "refactor: use platform API in config.c"
```

---

### Task 6: Refactor `cache.c` to use platform API

**Files:**
- Modify: `src/cache.c`

- [ ] **Step 1: Replace includes**

Remove `<pwd.h>`, `<sys/stat.h>`, `<unistd.h>`. Add `"../include/platform.h"`.

- [ ] **Step 2: Replace path resolution**

Replace `cache_get_path()` internals to use `platform_cache_dir()`, same pattern as config.c. Replace the home dir resolution (`getenv("XDG_CACHE_HOME")`, `getenv("HOME")`, `getpwuid(getuid())`) with `platform_cache_dir()`.

- [ ] **Step 3: Replace `ensure_cache_dir()`**

Replace the mkdir loop with `platform_mkdir_p(platform_cache_dir())`.

- [ ] **Step 4: Replace `rename()`, `unlink()`, `PATH_MAX`**

- `rename(tmp_path, path)` → `platform_atomic_rename(tmp_path, path)`
- `unlink(path)` → `platform_file_delete(path)`
- `PATH_MAX` → `PLATFORM_PATH_MAX`

- [ ] **Step 5: Verify build and tests**

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc) && cd build && ctest --output-on-failure
```
Expected: all tests pass (especially `cache` test).

- [ ] **Step 6: Commit**

```bash
git add src/cache.c
git commit -m "refactor: use platform API in cache.c"
```

---

### Task 7: Refactor `display.c` to use platform API

**Files:**
- Modify: `src/display.c`

- [ ] **Step 1: Replace includes and defines**

Remove `#define _POSIX_C_SOURCE 200809L` and `#include <unistd.h>`. Add `#include "../include/platform.h"`.

- [ ] **Step 2: Replace `isatty(STDOUT_FILENO)`**

In `use_colors()`, replace:
```c
    result = (isatty(STDOUT_FILENO) && (no_color == NULL || no_color[0] == '\0')) ? 1 : 0;
```
With:
```c
    result =
        (platform_isatty(stdout) && (no_color == NULL || no_color[0] == '\0')) ? 1 : 0;
```

- [ ] **Step 3: Replace `localtime_r()`**

Replace both occurrences of:
```c
    struct tm *now_tm = localtime_r(&now_t, &now_buf);
```
With:
```c
    platform_localtime(&now_t, &now_buf);
    struct tm *now_tm = &now_buf;
```

- [ ] **Step 4: Verify build and tests**

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc) && cd build && ctest --output-on-failure
```

- [ ] **Step 5: Commit**

```bash
git add src/display.c
git commit -m "refactor: use platform API in display.c"
```

---

### Task 8: Refactor `notification.c` to use platform API

**Files:**
- Modify: `src/notification.c`

- [ ] **Step 1: Replace includes and defines**

Remove `#define _GNU_SOURCE`, `#include <libgen.h>`, `#include <linux/limits.h>`, `#include <unistd.h>`. Add `#include "../include/platform.h"`.

Keep `#include <libnotify/notify.h>` — this file is Linux-only (Windows uses `notification_win.c`).

- [ ] **Step 2: Replace `get_icon_path()` internals**

Replace `readlink("/proc/self/exe")` + `dirname()` block with `platform_exe_dir()`:

```c
  // Try relative to binary location
  char assets_path[PLATFORM_PATH_MAX];
  {
    const char *exe = platform_exe_dir();
    if (exe[0] != '\0') {
      snprintf(assets_path, sizeof(assets_path), "%s/../assets/muslimtify.png", exe);
      possible_paths[3] = assets_path;
    }
  }
```

Replace `access(possible_paths[i], R_OK) == 0` with `platform_file_exists(possible_paths[i])`.

Replace `PATH_MAX` with `PLATFORM_PATH_MAX`.

Replace `getcwd(cwd, sizeof(cwd))` — keep it but it's fine on Linux since this file is Linux-only. No change needed here.

- [ ] **Step 3: Verify build and tests**

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc) && cd build && ctest --output-on-failure
```

- [ ] **Step 4: Commit**

```bash
git add src/notification.c
git commit -m "refactor: use platform API in notification.c"
```

---

### Task 9: Clean up `cli.c`

**Files:**
- Modify: `src/cli.c`

- [ ] **Step 1: Remove `_GNU_SOURCE`**

Remove line 1: `#define _GNU_SOURCE` — this is no longer needed since the POSIX features it enabled have moved to `platform_linux.c`.

- [ ] **Step 2: Add platform-conditional daemon help text**

Find the help text that mentions "systemd daemon" and wrap it:

```c
#ifdef _WIN32
    {"daemon", "Manage scheduled task", ...},
#else
    {"daemon", "Manage systemd daemon", ...},
#endif
```

(The exact location depends on how the dispatch table is structured. Find the `"daemon"` entry and add the `#ifdef`.)

- [ ] **Step 3: Verify build and tests**

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc) && cd build && ctest --output-on-failure
```

- [ ] **Step 4: Commit**

```bash
git add src/cli.c
git commit -m "refactor: remove _GNU_SOURCE from cli.c, add platform-conditional help text"
```

---

### Task 10: Create `src/cmd_daemon_win.c`

**Files:**
- Create: `src/cmd_daemon_win.c`

- [ ] **Step 1: Create the Windows daemon implementation**

```c
#include "../include/platform.h"
#include "cli_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static int run_schtasks(const char *args) {
  char cmd[PLATFORM_PATH_MAX * 2];
  snprintf(cmd, sizeof(cmd), "schtasks.exe %s", args);

  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    DWORD err = GetLastError();
    if (err == ERROR_ACCESS_DENIED) {
      fprintf(stderr,
              "Error: Access denied. Try running as administrator.\n");
    } else {
      fprintf(stderr, "Error: Failed to run schtasks (error %lu)\n", err);
    }
    return -1;
  }

  WaitForSingleObject(pi.hProcess, INFINITE);

  DWORD exit_code;
  GetExitCodeProcess(pi.hProcess, &exit_code);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return (int)exit_code;
}

int daemon_install_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  const char *exe = platform_exe_dir();
  char exe_path[PLATFORM_PATH_MAX];
  snprintf(exe_path, sizeof(exe_path), "%s\\muslimtify.exe", exe);

  if (!platform_file_exists(exe_path)) {
    fprintf(stderr, "Error: Cannot find muslimtify.exe at '%s'\n", exe_path);
    return 1;
  }

  char args[PLATFORM_PATH_MAX * 2];
  snprintf(args, sizeof(args),
           "/create /tn \"muslimtify\" /tr \"\\\"%s\\\" check\" /sc minute /mo 1 /f",
           exe_path);

  int result = run_schtasks(args);
  if (result == 0) {
    printf("Scheduled task 'muslimtify' created successfully.\n");
    printf("Prayer times will be checked every minute.\n");
  }
  return result;
}

int daemon_uninstall_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  int result = run_schtasks("/delete /tn \"muslimtify\" /f");
  if (result == 0) {
    printf("Scheduled task 'muslimtify' removed.\n");
  }
  return result;
}

int daemon_status_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  return run_schtasks("/query /tn \"muslimtify\"");
}
```

- [ ] **Step 2: Commit**

```bash
git add src/cmd_daemon_win.c
git commit -m "feat: add Windows Task Scheduler daemon implementation"
```

---

### Task 11: Add daemon and curl platform guards to CMake

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add cmd_daemon platform selection**

In the `add_library(muslimtify_lib OBJECT ...)` block, replace `src/cmd_daemon.c` with:

```cmake
    $<IF:$<BOOL:${WIN32}>,src/cmd_daemon_win.c,src/cmd_daemon.c>
```

- [ ] **Step 2: Add FetchContent for curl on Windows**

Replace the current dependency section with:

```cmake
# ── Dependencies ──────────────────────────────────────────────────────────────

if(WIN32)
    include(FetchContent)
    FetchContent_Declare(curl
        URL https://github.com/curl/curl/releases/download/curl-8_11_1/curl-8.11.1.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(curl)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBNOTIFY REQUIRED libnotify)
    pkg_check_modules(LIBCURL REQUIRED libcurl)
endif()
```

- [ ] **Step 3: Unify curl link targets**

Replace all `${LIBCURL_LIBRARIES}` references with a conditional. For the main executable:

```cmake
if(WIN32)
    target_link_libraries(muslimtify
        muslimtify_lib ole32 runtimeobject CURL::libcurl)
else()
    target_link_libraries(muslimtify
        muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
endif()
```

Similarly for `muslimtify_lib` include dirs:

```cmake
if(WIN32)
    target_include_directories(muslimtify_lib PRIVATE ${curl_SOURCE_DIR}/include)
else()
    target_include_directories(muslimtify_lib PRIVATE
        ${LIBNOTIFY_INCLUDE_DIRS} ${LIBCURL_INCLUDE_DIRS})
endif()
```

- [ ] **Step 4: Add test guards for POSIX-only tests**

Wrap `test_cli`, `test_config`, `test_cache` in `if(NOT WIN32)`:

```cmake
if(NOT WIN32)
    add_executable(test_cli tests/test_cli.c)
    muslimtify_set_target_defaults(test_cli)
    target_include_directories(test_cli PRIVATE ${LIBNOTIFY_INCLUDE_DIRS} ${LIBCURL_INCLUDE_DIRS})
    target_link_libraries(test_cli muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
    add_test(NAME cli COMMAND test_cli)

    add_executable(test_prayer_checker tests/test_prayer_checker.c)
    muslimtify_set_target_defaults(test_prayer_checker)
    target_include_directories(test_prayer_checker PRIVATE ${LIBNOTIFY_INCLUDE_DIRS} ${LIBCURL_INCLUDE_DIRS})
    target_link_libraries(test_prayer_checker muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
    add_test(NAME prayer_checker COMMAND test_prayer_checker)

    add_executable(test_config tests/test_config.c)
    muslimtify_set_target_defaults(test_config)
    target_include_directories(test_config PRIVATE ${LIBNOTIFY_INCLUDE_DIRS} ${LIBCURL_INCLUDE_DIRS})
    target_link_libraries(test_config muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
    add_test(NAME config COMMAND test_config)

    add_executable(test_cache tests/test_cache.c)
    muslimtify_set_target_defaults(test_cache)
    target_include_directories(test_cache PRIVATE ${LIBNOTIFY_INCLUDE_DIRS} ${LIBCURL_INCLUDE_DIRS})
    target_link_libraries(test_cache muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
    add_test(NAME cache COMMAND test_cache)
endif()

# Cross-platform tests (no POSIX dependencies)
add_executable(test_prayertimes tests/test_prayertimes.c)
muslimtify_set_target_defaults(test_prayertimes)
target_link_libraries(test_prayertimes m)
add_test(NAME prayertimes COMMAND test_prayertimes)

add_executable(test_json tests/test_json.c)
muslimtify_set_target_defaults(test_json)
target_link_libraries(test_json m)
add_test(NAME json COMMAND test_json)
```

- [ ] **Step 5: Verify Linux build and tests**

```bash
rm -rf build && cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc) && cd build && ctest --output-on-failure
```
Expected: all tests pass on Linux. The CMake changes are purely additive for the Linux path.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt
git commit -m "feat: add Windows daemon, curl FetchContent, and test guards to CMake"
```

---

### Task 12: Add Windows CI job

**Files:**
- Modify: `.github/workflows/ci.yml`

- [ ] **Step 1: Add the Windows build job**

Add after the existing `build-and-test` job:

```yaml
  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4

      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: cmake --build build --config Release

      - name: Test
        working-directory: build
        run: ctest --output-on-failure -C Release
```

- [ ] **Step 2: Commit**

```bash
git add .github/workflows/ci.yml
git commit -m "ci: add Windows build and test job"
```

---

### Task 13: Verify full Linux build is unaffected

**Files:** None modified — verification only.

- [ ] **Step 1: Clean rebuild**

```bash
rm -rf build && cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)
```
Expected: builds successfully.

- [ ] **Step 2: Run all tests**

```bash
cd build && ctest --output-on-failure
```
Expected: all tests pass.

- [ ] **Step 3: Verify Windows-only files are excluded**

```bash
grep -E "platform_win|cmd_daemon_win|notification_win" build/compile_commands.json
```
Expected: no output (these files are not compiled on Linux).
