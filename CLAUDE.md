# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Muslimtify is a minimalist prayer time notification daemon for Linux and Windows desktops, written in C99. It calculates Islamic prayer times using the Kemenag (Indonesian Ministry of Religious Affairs) astronomical method and sends desktop notifications via libnotify (Linux) or WinRT toast notifications (Windows). Runs as a systemd user timer (Linux) that checks every minute.

## Build & Test

```bash
# Configure (Debug enables AddressSanitizer + UBSan)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j$(nproc)

# Run all tests
cd build && ctest --output-on-failure

# Run a single test
cd build && ctest -R test_prayertimes --output-on-failure

# Format code
clang-format -i src/*.c src/*.h include/*.h
```

## Dependencies

**Linux:** CMake 3.22+, pkg-config, libnotify, libcurl
**Windows:** CMake 3.22+, MSVC, ole32, runtimeobject (WinRT)

## Architecture

**Entry flow:** `muslimtify.c` (init curl) → `cli.c` (command dispatch table) → `cmd_*.c` (handlers)

**Core modules:**
- `src/prayertimes.h` — Header-only Kemenag prayer time calculator (pure astronomical formulas)
- `src/libjson.h` — Header-only JSON parser/generator
- `src/config.c` — JSON config management (`~/.config/muslimtify/config.json`)
- `src/display.c` — Unicode table rendering, colored terminal output, JSON formatting
- `src/prayer_checker.c` — Matches current time against prayer times + reminders
- `src/notification.c` — libnotify wrapper (Linux)
- `src/notification_win.c` — WinRT toast notifications (Windows)
- `src/location.c` — IP-based geolocation via libcurl

**Commands** (each in `src/cmd_*.c`): show, check, next, config, location, enable/disable, reminder, daemon, list

**Systemd integration:** `systemd/muslimtify.timer` triggers `muslimtify.service` (oneshot, runs `muslimtify check`) every minute.

## Code Standards

- C99 standard, compiled with `-Wall -Wextra -Wpedantic -Wshadow -Wformat=2` (GCC/Clang) or `/W4` (MSVC)
- clang-format (LLVM-based, 2-space indent, 100-char column limit)
- Tests use a custom simple pass/fail counter framework (no external test lib)
- `test_prayertimes.c` validates against reference CSV data with 1-3 minute tolerance

## Commit Convention

Format: `type: short description`
Types: feat, fix, refactor, test, chore, docs
