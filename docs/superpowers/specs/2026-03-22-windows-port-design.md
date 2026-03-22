# Windows Port — Full Platform Support

## Summary

Make muslimtify build and run on Windows with full feature parity. Four sub-projects: platform abstraction layer, daemon/scheduler, libcurl bundling, and test guards.

## Starting state

Some Windows work is already done:
- `notification_win.c` exists with WinRT toast implementation
- CMakeLists.txt has partial `if(WIN32)` guards for notification source, link libs, and dependencies
- C standard is C99 (MSVC-compatible)
- MSVC compiler flag guards (`/W4`) are in place
- Target compiler: **MSVC only** (MinGW not supported)

## Sub-project 1: Platform Abstraction Layer

### New files

- `include/platform.h` — platform API declarations
- `src/platform_linux.c` — Linux/POSIX implementation
- `src/platform_win.c` — Windows implementation

### API

```c
const char *platform_config_dir(void);
const char *platform_cache_dir(void);
const char *platform_home_dir(void);
const char *platform_exe_dir(void);       // returns path WITHOUT trailing separator
int         platform_mkdir_p(const char *path);
int         platform_file_exists(const char *path);
int         platform_file_delete(const char *path);
int         platform_atomic_rename(const char *src, const char *dst);
void        platform_localtime(const time_t *t, struct tm *result);
int         platform_isatty(FILE *stream);
```

### Platform mapping

| Function | Linux | Windows |
|----------|-------|---------|
| `platform_config_dir()` | `$XDG_CONFIG_HOME/muslimtify` or `~/.config/muslimtify` | `%APPDATA%\muslimtify` |
| `platform_cache_dir()` | `$XDG_CACHE_HOME/muslimtify` or `~/.cache/muslimtify` | `%LOCALAPPDATA%\muslimtify` |
| `platform_home_dir()` | `$HOME` or `getpwuid(getuid())->pw_dir` | `%USERPROFILE%` |
| `platform_exe_dir()` | `readlink("/proc/self/exe")` + dirname | `GetModuleFileNameA()` + strip filename |
| `platform_mkdir_p(path)` | `mkdir(path, 0755)` loop on `/` | `CreateDirectoryA()` loop on `\` |
| `platform_file_exists(path)` | `access(path, F_OK)` | `GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES` |
| `platform_localtime(t, result)` | `localtime_r(t, result)` | `localtime_s(result, t)` (note: args reversed) |
| `platform_file_delete(path)` | `unlink(path)` | `DeleteFileA(path)` |
| `platform_atomic_rename(src, dst)` | `rename(src, dst)` | `MoveFileExA(src, dst, MOVEFILE_REPLACE_EXISTING)` |
| `platform_isatty(stream)` | `isatty(fileno(stream))` | `_isatty(_fileno(stream))` |

All directory-returning functions return cached paths (static buffers, no trailing separator) — no allocation needed by callers. `platform_mkdir_p` returns 0 on success, -1 on failure (silent — callers emit error messages with context). `platform_file_delete` and `platform_atomic_rename` return 0 on success, -1 on failure.

### Refactoring

Consumers drop direct POSIX calls and use `platform_*` instead:

| File | What changes |
|------|-------------|
| `config.c` | Remove `<linux/limits.h>`, `<pwd.h>`, `<unistd.h>`, `<sys/stat.h>`. Replace `config_get_path()` internals with `platform_config_dir()`. Replace `ensure_config_dir()` with `platform_mkdir_p()`. Replace `access()` with `platform_file_exists()`. Replace `rename()` with `platform_atomic_rename()`. |
| `cache.c` | Same pattern — replace path resolution and mkdir with `platform_cache_dir()` and `platform_mkdir_p()`. Replace `unlink()` with `platform_file_delete()`. Replace `rename()` with `platform_atomic_rename()`. |
| `display.c` | Remove `<unistd.h>`, `_POSIX_C_SOURCE`. Replace `localtime_r()` with `platform_localtime()`. Replace `isatty(STDOUT_FILENO)` with `platform_isatty(stdout)`. |
| `notification.c` | Replace `readlink("/proc/self/exe")` + `dirname()` with `platform_exe_dir()`. Replace `getcwd()` with platform-safe equivalent (MSVC provides `_getcwd()`; use `#ifdef _WIN32` inline for this one-liner). Remove `<linux/limits.h>`, `<libgen.h>`, `<unistd.h>`. Replace `access()` with `platform_file_exists()`. |
| `cmd_daemon.c` | **Linux-only file** — not compiled on Windows (replaced by `cmd_daemon_win.c`). No refactoring needed, but can optionally use `platform_home_dir()` and `platform_mkdir_p()` for consistency. |
| `cli.c` | Remove `_GNU_SOURCE`. |

`_GNU_SOURCE` and `_POSIX_C_SOURCE` defines move into `platform_linux.c`.

### PATH_MAX

`<linux/limits.h>` is used for `PATH_MAX`. Replace with:

```c
// in platform.h
#ifdef _WIN32
#define PLATFORM_PATH_MAX 260  // MAX_PATH on Windows
#else
#include <limits.h>
#ifdef PATH_MAX
#define PLATFORM_PATH_MAX PATH_MAX
#else
#define PLATFORM_PATH_MAX 4096
#endif
#endif
```

### CMake

```cmake
$<IF:$<BOOL:${WIN32}>,src/platform_win.c,src/platform_linux.c>
```

Added to the object library source list.

## Sub-project 2: Daemon/Scheduler

### New file

- `src/cmd_daemon_win.c` — Windows Task Scheduler implementation

### Same function signatures as `cmd_daemon.c`:

```c
int daemon_install_handler(int argc, char **argv);
int daemon_uninstall_handler(int argc, char **argv);
int daemon_status_handler(int argc, char **argv);
```

### Implementation

All three use `CreateProcessA()` + `WaitForSingleObject()` to shell out to `schtasks.exe`:

| Command | schtasks invocation |
|---------|-------------------|
| `muslimtify daemon install` | `schtasks /create /tn "muslimtify" /tr "<exe_path> check" /sc minute /mo 1 /f` |
| `muslimtify daemon uninstall` | `schtasks /delete /tn "muslimtify" /f` |
| `muslimtify daemon status` | `schtasks /query /tn "muslimtify"` |

`<exe_path>` is obtained via `platform_exe_dir()` + `\muslimtify.exe`.

No `fork()`/`execvp()`/`waitpid()` — those are Linux-only. The Windows implementation uses `CreateProcessA()` which is the native equivalent.

**Elevation note:** `schtasks /sc minute` may require admin privileges on some Windows configurations. The implementation should handle `ERROR_ACCESS_DENIED` gracefully with a clear error message suggesting the user run as administrator.

### CMake

```cmake
$<IF:$<BOOL:${WIN32}>,src/cmd_daemon_win.c,src/cmd_daemon.c>
```

### CLI help text

In `cli.c`, change the daemon help text from "Manage systemd daemon" to a platform-generic string. Use `#ifdef _WIN32`:

```c
#ifdef _WIN32
"Manage scheduled task"
#else
"Manage systemd daemon"
#endif
```

## Sub-project 3: libcurl on Windows

### CMake changes only

No source code changes. `location.c` uses `<curl/curl.h>` which works on both platforms.

```cmake
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
    pkg_check_modules(LIBCURL REQUIRED libcurl)
endif()
```

Key options:
- `BUILD_SHARED_LIBS OFF` — static link, no DLL needed alongside the .exe
- `BUILD_CURL_EXE OFF` — don't build the curl command-line tool
- `CURL_USE_SCHANNEL ON` — use Windows native TLS (no OpenSSL dependency)

**Link target unification:** All references to `${LIBCURL_LIBRARIES}` and `${LIBCURL_INCLUDE_DIRS}` throughout CMakeLists.txt (main exe + all test targets) must be replaced with platform-conditional expressions:

```cmake
# Link: use CURL::libcurl on Windows, ${LIBCURL_LIBRARIES} on Linux
$<IF:$<BOOL:${WIN32}>,CURL::libcurl,${LIBCURL_LIBRARIES}>

# Include dirs: CURL::libcurl handles this via target properties on Windows
# On Linux, still need ${LIBCURL_INCLUDE_DIRS}
```

This affects: `muslimtify` exe, `muslimtify_lib`, and all test targets that link libcurl.

## Sub-project 4: Test portability

### CMake guards only

Tests using POSIX functions are guarded with `if(NOT WIN32)`:

```cmake
if(NOT WIN32)
    # These tests use mkdtemp(), setenv(), system("rm -rf") — POSIX only
    add_executable(test_cli tests/test_cli.c)
    ...
    add_executable(test_config tests/test_config.c)
    ...
    add_executable(test_cache tests/test_cache.c)
    ...
endif()

# These tests are pure computation — work on all platforms
add_executable(test_prayertimes tests/test_prayertimes.c)
...
add_executable(test_json tests/test_json.c)
...
```

### CI update

Add a Windows build job to `.github/workflows/ci.yml`:

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

This runs `test_prayertimes` and `test_json` on Windows. The POSIX-only tests are excluded by the CMake guards.

## Execution order

1. **Platform abstraction** — must be first, everything depends on it
2. **Daemon/scheduler** — depends on platform layer for `platform_exe_dir()`, `platform_home_dir()`, `platform_mkdir_p()`
3. **libcurl bundling** — independent, can be done anytime
4. **Test guards + Windows CI** — last, validates everything works

## Out of scope

- Custom AUMID registration for toast notifications (existing PowerShell AUMID workaround is sufficient)
- Windows installer (.msi / NSIS) — separate future work
- Icon support in Windows toasts
- Making POSIX-only tests portable to Windows (deferred)
