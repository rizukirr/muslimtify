# MSVC Warning Cleanup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate MSVC C4200/C4116/C4996/C4244 warnings by refactoring the JSON arena, introducing safe string helpers, and updating all call sites/tests.

**Architecture:** Replace the arena’s zero-length member with pointer-managed blocks, introduce named helper structs in `json.h`, add a shared `string_util` module providing bounded copy/parse helpers (plus tests), and refactor all consumers to use the new APIs while updating verification/tests.

**Tech Stack:** C99, MSVC/GCC/Clang, CMake, CTest, libcurl, Win32 APIs.

---

### File Map

- `src/json.h`: Update Region struct, helper typedefs, function signatures.
- `src/string_util.h` / `src/string_util.c`: New shared helpers (`copy_string`, `append_string`, `bounded_strlen`, `errno_string`, `copy_wstring`, `parse_tokens`).
- `src/config.c`, `src/cache.c`, `src/location.c`, `src/notification_win.c`: Replace unsafe string calls, integrate helpers, adjust logging.
- `tests/test_string_util.c`: New helper-focused tests (compiled cross-platform, guard wide-string checks).
- `tests/test_prayertimes.c`: Wrap `sscanf` via macro.
- `CMakeLists.txt`: Add new helper/test files to builds/targets.
- `docs/CHANGELOG.md` (if present) to mention warning cleanup.

---

### Task 1: Refactor JSON Arena & Helper Typedefs

**Files:**
- Modify: `src/json.h`
- Tests: `tests/test_json.c`

- [ ] **Step 1:** Update `Region` struct to store `uint8_t *data` pointer; adjust allocation logic to malloc `sizeof(Region) + block_size` and set pointer, plus propagate pointer usage through allocation helpers.
- [ ] **Step 2:** Introduce named typedefs (`AlignProbe`, `JsonSlice`, etc.) replacing anonymous structs/macros; update function signatures and static helpers accordingly.
- [ ] **Step 3:** Review dependent code (e.g., `json_alloc_align`, `json_extract_value`, `find_matching_bracket`) to ensure new typedefs compile; rebuild header consumers (`cmake --build build --config Debug`).
- [ ] **Step 4:** Run `ctest --test-dir build -R json --output-on-failure` to confirm parser behavior unchanged.

### Task 2: Add Shared String Utility Module

**Files:**
- Create: `src/string_util.c`, `src/string_util.h`
- Modify: `CMakeLists.txt`, `src/CMakeLists.txt` (if applicable)

- [ ] **Step 1:** Define helper prototypes in `src/string_util.h` (bounded copies, `bounded_strlen`, `errno_string`, `_WIN32` `copy_wstring`, `parse_tokens` & callback type) with documented contracts.
- [ ] **Step 2:** Implement helpers in `src/string_util.c`, including platform branches for `strerror_s` vs `strerror_r`, and `strtok_s` vs `strtok_r`. Ensure no plain `strerror` usage and add defensive argument checks/logging via `fprintf(stderr, ...)`.
- [ ] **Step 3:** Update root `CMakeLists.txt` / module CMake to compile `string_util.c` into `muslimtify_lib` and install header.
- [ ] **Step 4:** Build Debug config to ensure new files compile on current platform.

### Task 3: Introduce Helper Tests

**Files:**
- Create: `tests/test_string_util.c`
- Modify: `tests/CMakeLists.txt` (or root CMake)

- [ ] **Step 1:** Write tests covering `copy_string`, `append_string`, `bounded_strlen`, `errno_string` (mock error), `parse_tokens` (success, truncation, callback stop), and `_WIN32` `copy_wstring` (guard with `#ifdef`). Use asserts verifying output buffers and return codes.
- [ ] **Step 2:** Register test target in CMake/CTest, linking against `muslimtify_lib`.
- [ ] **Step 3:** Build and run `ctest --test-dir build -R string_util --output-on-failure`.

### Task 4: Refactor Config Module

**Files:**
- Modify: `src/config.c`
- Tests: `ctest -R config` (Linux) + manual testing Windows; `tests/test_prayertimes.c`

- [ ] **Step 1:** Replace every `strcpy`/`strncpy`/`strncat`/`strtok`/`strerror`/`sscanf` usage with calls to the new helpers. Introduce logging per design when truncation occurs. Add `config_trunc_logged` static flag if needed.
- [ ] **Step 2:** Update `config_format_reminders` to stop appending once `append_string` reports truncation (leave buffer as-is after first failure) in line with the spec.
- [ ] **Step 3:** Update `config_parse_reminders` to use `parse_tokens` with the described callback struct; ensure return codes map to existing semantics.
- [ ] **Step 4:** Apply the MSVC C4244 fix in `config_get_prayer` (store `tolower` output in `int tmp` before casting to `char`).
- [ ] **Step 5:** Add PARSE_TIME macro to `tests/test_prayertimes.c`, switch `time_to_minutes` to use it, and guard `sscanf_s` usage for MSVC.
- [ ] **Step 6:** Build + run `ctest --test-dir build -R config --output-on-failure` (Linux) and `ctest --test-dir build -R prayertimes --output-on-failure`; on Windows, run `build/bin/Debug/muslimtify.exe config show` (or similar) to sanity-check.

### Task 5: Refactor Cache & Location Modules

**Files:**
- Modify: `src/cache.c`, `src/location.c`

- [ ] **Step 1:** Replace `strncpy`/manual buffer writes with `copy_string`/`append_string` as applicable; add module-level truncation flag/logging per design.
- [ ] **Step 2:** Update any repeated path-building logic to use helpers; ensure `cache_get_path` copies into buffers safely.
- [ ] **Step 3:** Build project, then run `ctest --test-dir build -R cache --output-on-failure` (Linux) or manual CLI (`muslimtify cache` flows) on Windows.
- [ ] **Step 4:** Execute `muslimtify location auto` (`build/bin/muslimtify location auto` on Linux, `build/bin/Debug/muslimtify.exe location auto` on Windows) to exercise the location refactor per the spec.

### Task 6: Refactor Windows Notification Module

**Files:**
- Modify: `src/notification_win.c`

- [ ] **Step 1:** Include `string_util.h`, replace all `wcscpy` usages with `copy_wstring` (and log/abort on failure as per design).
- [ ] **Step 2:** Ensure any wide-string buffer lengths passed to helper account for terminators; adjust constants as needed.
- [ ] **Step 3:** Build Windows Debug & Release configs (`cmake --build build --config Debug`, `cmake --build build --config Release`). Trigger a toast via CLI to verify runtime behavior.

### Task 7: Update CMake/Docs & Verify Build Warnings

**Files:**
- Modify: `CMakeLists.txt`, `tests/CMakeLists.txt`, `docs/CHANGELOG.md`, possibly `README.md`

- [ ] **Step 1:** Ensure new files/tests are added to appropriate targets; rerun `cmake --build build --config Debug` and `--config Release` on Windows.
- [ ] **Step 2:** Run full test suites relevant to platform (json, string_util, prayertimes, config/cache where supported).
- [ ] **Step 3:** Inspect MSVC build logs to confirm absence of C4200/C4116/C4996/C4244 warnings (fail build if present). Document changes in `docs/CHANGELOG.md`.
- [ ] **Step 4:** Prepare for commits (per-task or logical grouping) with descriptive messages (`refactor: add string util helpers`, etc.).
