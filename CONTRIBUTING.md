# Contributing to Muslimtify

Thanks for your interest in contributing! This guide will help you get started.

## Development Setup

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install cmake pkg-config libnotify-dev libcurl4-openssl-dev

# Build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Run the binary
./bin/muslimtify
```

## Project Structure

```
src/
  cli.c              # Top-level dispatch table and entry point
  cmd_show.c         # show, check commands
  cmd_next.c         # next command and sub-handlers
  cmd_config.c       # config sub-commands
  cmd_location.c     # location sub-commands
  cmd_prayer.c       # enable, disable, list, reminder
  cmd_daemon.c       # systemd daemon management
  config.c           # JSON config load/save
  display.c          # Terminal output (tables, colors, JSON)
  location.c         # IP geolocation
  notification.c     # libnotify wrapper
  prayer_checker.c   # Prayer time matching
  prayertimes.h      # Kemenag calculation (header-only)
  libjson.h          # JSON parser (header-only)
include/             # Public headers
tests/               # Test suites
```

## Code Style

- C23 standard
- 2-space indentation
- Opening brace on same line
- Run `clang-format` before committing (config provided in `.clang-format`)

## Commit Messages

Follow this format:

```
type: short description

Optional longer explanation.
```

Types: `feat`, `fix`, `refactor`, `test`, `chore`, `docs`

Examples:
- `feat: add Muhammadiyah calculation method`
- `fix: handle midnight crossover in reminder times`
- `test: add display output tests`

## Pull Request Process

1. Fork the repo and create a branch from `main`
2. Make your changes
3. Ensure all tests pass: `ctest --output-on-failure`
4. Ensure no compiler warnings (the project uses `-Wall -Wextra -Wpedantic -Wshadow -Wformat=2`)
5. Run `clang-format -i` on changed files
6. Open a PR with a clear description of what and why

## Adding New Features

- New source files go in `src/` with a matching header in `include/`
- Add new `.c` files to the `muslimtify_lib` OBJECT library in `CMakeLists.txt`
- New test files follow the existing pattern: `add_executable` + `add_test` + link `muslimtify_lib`
- Tests use simple pass/fail counters (no external framework)

## Prayer Calculation

The Kemenag method is documented in `docs/KEMENAG_METHOD.md`. If adding a new calculation method, please include reference data for validation.

## Questions?

Open an issue if something is unclear.
