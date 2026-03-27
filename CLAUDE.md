# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Muslimtify is a minimalist prayer time notification daemon for Linux and Windows desktops, written in C99. It supports 21 international calculation methods (default: Kemenag/Indonesia) and sends desktop notifications via libnotify (Linux) or WinRT toast notifications (Windows). Runs as a systemd user timer (Linux) or a hidden-window background process (Windows) that checks every minute.

Prayer time calculation logic originated here and was extracted into [libmuslim](https://github.com/rizukirr/libmuslim).

## Build & Test

```bash
# Linux — configure (Debug enables AddressSanitizer + UBSan on GCC/Clang)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Windows — MSVC multi-config generator
cmake -B build
cmake --build build --config Release

# Run all tests
cd build && ctest --output-on-failure          # Linux
ctest --test-dir build --output-on-failure -C Release  # Windows

# Run a single test
cd build && ctest -R prayertimes --output-on-failure

# Format code (CI enforces this on both src/ and tests/)
clang-format -i src/*.c src/*.h include/*.h tests/*.c
```

## CI

GitHub Actions (`ci.yml`) runs on push/PR to `main`:
- **lint:** clang-format `--dry-run --Werror` on all source files, plus informational clang-tidy
- **build-and-test (Linux):** matrix of GCC × Clang × Debug × Release
- **build-windows:** MSVC Release build + tests

## Dependencies

**Linux:** CMake 3.22+, pkg-config, libnotify, libcurl
**Windows:** CMake 3.22+, MSVC, ole32, runtimeobject (WinRT); libcurl is fetched automatically via FetchContent

## Architecture

**Entry flow:** `muslimtify.c` (init curl) → `cli.c` (command dispatch table) → `cmd_*.c` (handlers)

**Headers:** `include/` contains public module headers; `src/` contains internal headers (`json.h`, `prayertimes.h`, `string_util.h` — header-only implementations).

**Core modules:**
- `src/prayertimes.h` — Header-only Kemenag prayer time calculator (pure astronomical formulas)
- `src/json.h` — Header-only JSON parser (arena-based, no malloc per token)
- `src/config.c` — JSON config management (`~/.config/muslimtify/config.json` on Linux, `%APPDATA%\muslimtify\config.json` on Windows)
- `src/cache.c` — Daily prayer time cache (`~/.cache/muslimtify` / `%LOCALAPPDATA%\muslimtify`)
- `src/display.c` — Unicode table rendering, colored terminal output, JSON formatting
- `src/prayer_checker.c` / `src/check_cycle.c` — Matches current time against prayer times + reminders
- `src/location.c` — IP-based geolocation via libcurl (ipinfo.io)
- `src/string_util.c` — Small string helpers

**Platform-split sources** (CMake selects via generator expressions):
- `src/notification.c` (Linux/libnotify) vs `src/notification_win.c` (Windows/WinRT toast)
- `src/cmd_daemon.c` (Linux/systemd) vs `src/cmd_daemon_win.c` (Windows hidden-window loop)
- `src/platform_linux.c` vs `src/platform_win.c` — path resolution, platform abstractions

**Windows service:** `src/muslimtify_service_win.c` builds a separate `muslimtify-service` WIN32 GUI executable (no console window).

**Commands** (each in `src/cmd_*.c`): show, check, next, config, location, enable/disable, reminder, daemon, list, method, notification

**Systemd integration:** `systemd/muslimtify.timer` triggers `muslimtify.service` (oneshot, runs `muslimtify check`) every minute.

## Tests

Tests use a custom pass/fail counter framework (no external test lib). Test availability varies by platform:

- **Cross-platform:** `test_prayertimes`, `test_json`, `test_string_util`, `test_platform`
- **Linux-only:** `test_cli`, `test_prayer_checker`, `test_config`, `test_cache`
- **Windows-only:** `test_cmd_daemon_win`, `test_notification_win`

`test_prayertimes.c` validates against reference CSV data with 1-3 minute tolerance.

## Code Standards

- C99 standard, compiled with `-Wall -Wextra -Wpedantic -Wshadow -Wformat=2` (GCC/Clang) or `/W4` (MSVC)
- clang-format (LLVM-based, 2-space indent, 100-char column limit; config in `.clang-format`)
- New source files go in `src/` with a matching header in `include/`; add to the `muslimtify_lib` OBJECT library in `CMakeLists.txt`

## Commit Convention

Format: `type: short description`
Types: feat, fix, refactor, test, chore, docs
Note: Don't ever use co-authored commits
