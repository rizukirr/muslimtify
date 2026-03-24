# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Muslimtify is a minimalist prayer time notification daemon for Linux and Windows desktops, written in C99. It calculates Islamic prayer times using the Kemenag (Indonesian Ministry of Religious Affairs) astronomical method and sends desktop notifications via libnotify (Linux) or WinRT toast notifications (Windows). The astronomical method is documented in `docs/KEMENAG_METHOD.md`.

## Build & Test

```bash
# Configure (Debug enables AddressSanitizer + UBSan)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Windows
cmake -S . -B build
cmake --build build --config Release

# Build (Linux)
cmake --build build -j$(nproc)

# Run all tests
ctest --test-dir build --output-on-failure

# Run a single test (CTest names: cli, prayertimes, json, config, cache,
# prayer_checker, platform, string_util, cmd_daemon_win, notification_win)
ctest --test-dir build -R prayertimes --output-on-failure

# Format code
clang-format -i src/*.c src/*.h include/*.h
```

## Dependencies

**Linux:** CMake 3.22+, pkg-config, libnotify, libcurl
**Windows:** CMake 3.22+, MSVC, ole32, runtimeobject (WinRT); libcurl vendored via FetchContent

## Architecture

**Entry flow:** `muslimtify.c` (init curl) → `cli.c` (command dispatch table) → `cmd_*.c` (handlers)

**Headers:** `include/` contains public module headers; `src/` contains internal headers (`json.h`, `prayertimes.h`, `string_util.h` — header-only implementations).

**Core modules:**
- `src/prayertimes.h` — Header-only Kemenag prayer time calculator (pure astronomical formulas)
- `src/json.h` — Header-only JSON parser (arena-based)
- `src/config.c` — JSON config management (`~/.config/muslimtify/config.json`)
- `src/display.c` — Unicode table rendering, colored terminal output, JSON formatting
- `src/prayer_checker.c` — Matches current time against prayer times + reminders
- `src/cache.c` — Caching of location and prayer time results
- `src/check_cycle.c` — Prevents duplicate notifications within the same minute
- `src/location.c` — IP-based geolocation via libcurl (ipinfo.io)
- `src/string_util.c` — String helpers (trim, compare)

**Platform-specific files** (selected at build time via CMake generator expressions):
- Linux only: `src/notification.c`, `src/cmd_daemon.c`, `src/platform_linux.c`
- Windows only: `src/notification_win.c`, `src/cmd_daemon_win.c`, `src/platform_win.c`

**Commands** (each in `src/cmd_*.c`): show, check, next, config, location, enable/disable, reminder, daemon, list

**Systemd integration:** `systemd/muslimtify.timer` triggers `muslimtify.service` (oneshot, runs `muslimtify check`) every minute.

## Code Standards

- C99 standard, compiled with `-Wall -Wextra -Wpedantic -Wshadow -Wformat=2` (GCC/Clang) or `/W4` (MSVC)
- clang-format (LLVM-based, 2-space indent, 100-char column limit, attach braces)
- Functions: `snake_case`, prefixed by module name (e.g. `config_load`, `prayer_checker_next`)
- Error handling: return enum status codes, not bools, when multiple failure modes exist
- No VLAs; use fixed-size arrays or heap allocations
- Use `const` for pointers you don't mutate; `size_t` for lengths/indices
- Tests use a custom simple pass/fail counter framework (no external test lib)
- `test_prayertimes.c` validates against reference CSV data with 1-3 minute tolerance
- Linux-only tests: cli, prayer_checker, config, cache. Cross-platform: prayertimes, json, string_util, platform

## Commit Convention

Format: `type: short description`
Types: feat, fix, refactor, test, chore, docs
