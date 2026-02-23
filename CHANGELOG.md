# Changelog

All notable changes to Muslimtify will be documented in this file.

---

## [0.1.0] - 2026-02-23

### Added
- **Daemon management CLI** — `muslimtify daemon [install|uninstall|status]`
  - `install` — resolves binary path, writes systemd unit files, enables and starts timer
  - `uninstall` — stops, disables, and removes unit files
  - `status` — shows timer state and next trigger time
- **Colored terminal output** for the prayer times table:
  - Bold title and header row
  - Green for enabled, dim for disabled rows
  - Bold yellow + `▶` indicator for the next upcoming prayer
  - Respects `NO_COLOR` env var; plain text when piped/redirected
- **`install.sh`** — one-command installer: builds in release mode, installs binary
  and icons to `/usr/local`, then sets up the systemd user timer
- **`uninstall.sh`** — full uninstaller; `--purge` also removes config directory;
  includes a manual uninstall reference

### Changed
- Dhuha disabled by default
- Fixed column alignment in prayer table — replaced multi-byte `✓`/`✗` symbols
  (which broke `printf` width calculations) with plain `Enabled`/`Disabled` text
- Replaced `install-systemd.sh` and `uninstall-systemd.sh` with `install.sh`
  and `uninstall.sh`

### Fixed
- Stack buffer overflow in `reminders` buffer (`display.c`) — was 24 bytes with
  unbounded `strcat`; now 80 bytes built with `snprintf`
- Null dereference after `getpwuid()` (`config.c`)
- Null dereference after `localtime()` — all three call sites in `cli.c` and
  `display.c`
- `ftell()` error value (`-1`) not checked before `malloc` (`config.c`)
- `fread()` return value ignored — now used for correct null termination
- `strncpy` missing explicit null termination in `config.c`, `location.c`,
  `notification.c`
- Integer overflow in curl write callback (`size * nmemb`) in `location.c`
- `malloc(1)` return not checked in `location_fetch`
- Coordinate range not validated after `atof()` in `location_fetch`
- `tm_wday`/`tm_mon` not bounds-checked before array index in `display.c`
- Removed pointless `fopen("/proc/self/exe")` in `notification.c`

---

## [0.0.1] - 2026-02-23

### Added — Initial Release

#### Core Features
- Prayer time calculation using Kemenag Indonesia method
- 7 prayer times: Fajr, Sunrise, Dhuha, Dhuhr, Asr, Maghrib, Isha
- Automatic location detection via ipinfo.io
- Desktop notifications using libnotify
- Custom mosque icon (`assets/muslimtify.png`)
- Customizable reminders per prayer (e.g., 30, 15, 5 minutes before)
- Per-prayer enable/disable controls
- Sunrise disabled by default

#### CLI
- `muslimtify` / `show` / `show --format json`
- `muslimtify next` — countdown to next prayer
- `muslimtify check` — notify if it's prayer time (called by systemd every minute)
- `muslimtify location [auto|show|set|clear]`
- `muslimtify enable/disable <prayer>`
- `muslimtify list` / `reminder` / `config` / `version` / `help`

#### Infrastructure
- JSON config at `~/.config/muslimtify/config.json`
- CMake build system, C23, pkg-config
- Systemd user-mode timer
- Dependencies: libnotify, libcurl

---

## Planned Features

### v0.2.0
- [ ] Multiple calculation methods (MWL, ISNA, Egypt, etc.)
- [ ] Config file icon path support

### v1.0.0 — First packaged release
- [ ] AUR package (Arch Linux)
- [ ] `.deb` package (Debian/Ubuntu)
- [ ] `.rpm` package (Fedora/RHEL)

### v1.1.0
- [ ] Monthly prayer calendar
- [ ] Hijri date display
- [ ] Qibla direction

### v2.0.0
- [ ] GUI configuration tool (optional)

---

## Version Numbering

Muslimtify follows [Semantic Versioning](https://semver.org/):
- **MAJOR** — major milestone or breaking change (v1.0.0 = first packaged release)
- **MINOR** — new features, backward compatible
- **PATCH** — bug fixes

---

**Current Version:** 0.1.0
**Release Date:** February 23, 2026
**Status:** Source distribution only
