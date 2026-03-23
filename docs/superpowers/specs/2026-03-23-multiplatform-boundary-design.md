# Multiplatform Boundary Design

## Summary

Restructure muslimtify for future multiplatform support using a minimal-first approach. Keep existing Linux
behavior and current CLI semantics stable while tightening the boundary between shared application logic and
OS-specific integration code.

The goal of this design is not a large file move. The first step is to define and enforce clearer boundaries so
new platform work can be added without spreading `#ifdef _WIN32` and POSIX assumptions through shared modules.

## Goals

- Preserve existing Linux behavior, especially the current systemd integration flow.
- Keep the current CLI surface stable across platforms.
- Separate OS-specific integrations from shared application logic.
- Minimize churn in the first refactor so regressions are easy to isolate.
- Create a structure that can support future platforms beyond Linux and Windows.

## Non-goals

- Large directory reorganization in the first pass.
- Rewriting working Linux code to mimic Windows behavior.
- Introducing a broad, abstract "kernel" layer with unclear ownership.
- Changing user-facing command names or command semantics during the first refactor.

## Current State

The repository already has a partial multiplatform shape:

- Shared logic lives mostly in `src/config.c`, `src/cache.c`, `src/location.c`, `src/prayer_checker.c`,
  `src/display.c`, `src/string_util.c`, and `src/prayertimes.h`.
- Windows-specific daemon code already exists in `src/cmd_daemon_win.c`.
- Linux-specific daemon code already exists in `src/cmd_daemon.c`.
- Windows-specific notification code already exists in `src/notification_win.c`.
- Linux-specific notification code already exists in `src/notification.c`.
- A platform utility boundary already exists in `include/platform.h` with `src/platform_linux.c` and
  `src/platform_win.c`.
- CMake already selects several platform-specific files with generator expressions.

This means the first refactor should strengthen existing boundaries rather than replace them with a new
architecture all at once.

## Architecture

### Layer 1: CLI layer

Files:

- `src/cli.c`
- `src/cmd_*.c`

Responsibilities:

- Parse commands and arguments.
- Preserve a platform-consistent command surface.
- Dispatch to shared logic or platform-specific handlers.

Constraints:

- Avoid embedding raw Windows or Linux API calls directly in shared CLI files.
- Keep platform-specific command implementations in separate translation units when behavior depends on the OS.

### Layer 2: Shared core

Files in scope for the first pass:

- `src/config.c`
- `src/cache.c`
- `src/location.c`
- `src/prayer_checker.c`
- `src/display.c`
- `src/string_util.c`
- `src/prayertimes.h`

Responsibilities:

- Prayer-time and reminder behavior.
- Configuration and cache data handling.
- Location retrieval logic.
- User-facing formatting that is not tied to an OS integration API.

Constraints:

- Shared modules may depend on `include/platform.h` for narrow system services.
- Shared modules must not call Linux-only or Windows-only APIs directly.
- Shared modules must not own scheduler, notification backend, or shell-out policy.

### Layer 3: Platform adapters

Files:

- `src/platform_linux.c`
- `src/platform_win.c`
- `src/notification.c`
- `src/notification_win.c`
- `src/cmd_daemon.c`
- `src/cmd_daemon_win.c`

Responsibilities:

- Filesystem and environment path conventions.
- Time and terminal wrappers where libc usage differs by platform.
- Native notification backend integration.
- Native scheduler/daemon registration.

Constraints:

- Platform adapters should expose narrow interfaces.
- They should not absorb shared business logic.
- Differences between systemd and Windows Task Scheduler remain separate implementation details behind the same
  CLI command surface.

## Boundary Rules

The first refactor should follow these rules:

1. Shared core may call `platform.h`, but not raw platform APIs.
2. Notification backends remain separate per platform.
3. Daemon/scheduler registration remains separate per platform.
4. Avoid broad `#ifdef` blocks in shared source files.
5. Prefer compile-time source selection in CMake over mixed-platform code in one file.
6. Do not move files purely for aesthetics before the interfaces are stable.

## Minimal-First Refactor Scope

### Keep stable now

- Existing Linux daemon behavior and systemd integration.
- Existing command names:
  - `muslimtify daemon install`
  - `muslimtify daemon uninstall`
  - `muslimtify daemon status`
- Existing notification API shape in `include/notification.h`.
- Existing CMake platform selection pattern.

### Tighten now

- Audit shared modules for direct POSIX or Win32 usage and route those calls through `platform.h`.
- Keep daemon handlers split by platform.
- Keep notification backends split by platform.
- Document that the Windows daemon command registers a scheduled task rather than a systemd unit.

### Defer

- Moving files into new `src/core/`, `src/platform/`, or `src/cli/` directories.
- Generalizing process launching beyond the immediate needs of current platform code.
- Broader renaming of modules or headers.

## Daemon Strategy

The user-facing command remains:

- `muslimtify daemon install`
- `muslimtify daemon uninstall`
- `muslimtify daemon status`

Implementation remains platform-specific:

- Linux implementation manages systemd units and timers.
- Windows implementation manages a Task Scheduler entry using `schtasks.exe`.

This keeps the UX stable while allowing each platform to use the correct native scheduler.

## Platform Service Surface

The platform layer should stay narrow and concrete. Services currently appropriate for `platform.h` include:

- config directory lookup
- cache directory lookup
- home directory lookup
- executable directory or executable path lookup
- recursive directory creation
- file existence checks
- file deletion
- atomic rename
- local time wrapper
- terminal detection

Do not expand this into a catch-all abstraction unless multiple shared modules genuinely need the same new
service.

## CMake Direction

The existing source-selection pattern is the correct direction for the first pass:

- select Linux vs Windows daemon source at build time
- select Linux vs Windows notification source at build time
- select Linux vs Windows platform source at build time

Future cleanup may group sources by logical layer, but the first refactor should keep build changes focused on
boundary correctness rather than directory churn.

## Migration Plan

### Phase 1: Boundary hardening

- Keep the existing file layout.
- Strengthen `platform.h` usage in shared modules.
- Remove direct platform calls from shared modules where needed.
- Keep OS-specific integrations isolated in their current platform-specific files.
- Update documentation for Windows scheduler behavior.

### Phase 2: Organizational cleanup

After Linux and Windows behavior are stable under the boundary rules:

- optionally introduce `src/core/`
- optionally introduce `src/platform/linux/`
- optionally introduce `src/platform/windows/`
- optionally introduce `src/cli/`

This phase should be mostly mechanical and should not mix behavior changes with file moves.

### Phase 3: Future platform expansion

Add new platform adapters as needed while preserving the shared core. This makes future ports additive instead
of invasive.

## Risks

- Over-abstracting too early can create a vague platform layer that is harder to maintain than the current code.
- Moving too many files at once will make regressions harder to diagnose.
- Allowing shared modules to keep direct POSIX assumptions will block future ports even if Windows builds today.
- Excessive `#ifdef` use inside shared files will increase maintenance cost over time.

## Testing Strategy

Phase 1 verification should focus on behavior stability:

- Linux:
  - build successfully
  - run existing test suite
  - verify daemon command behavior remains unchanged
- Windows:
  - build successfully under MSVC
  - verify `muslimtify daemon install/status/uninstall`
  - verify `muslimtify check` can still trigger notifications

Cross-platform tests should continue to target shared logic, especially:

- prayertime calculations
- JSON handling
- string utilities

## Recommendation

Adopt the minimal-first boundary hardening plan now. Do not do a large directory move until the platform seams
have settled and both Linux and Windows behavior are verified.
