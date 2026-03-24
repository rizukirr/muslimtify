# Windows Service Helper Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the flashing Windows scheduled-task console launch with a production-ready internal helper executable that runs the same check cycle without creating a visible window.

**Architecture:** Keep `muslimtify.exe` as the user-facing CLI and add a Windows-only `muslimtify-service.exe` built as a GUI-subsystem executable. Extract the current `check` behavior into a shared function so both the CLI and helper run the same notification flow, then update Windows daemon registration to schedule the helper directly.

**Tech Stack:** C99, CMake, Win32 subsystem settings, existing config/cache/notification modules, Task Scheduler

---

## File Structure

### Existing files to modify

- `CMakeLists.txt`
  - Build the new Windows helper target and keep Windows tests aligned.
- `src/cmd_show.c`
  - Remove the duplicated one-shot check implementation from `handle_check()` and call the shared function instead.
- `src/cmd_daemon_win.c`
  - Point Task Scheduler registration at `muslimtify-service.exe` instead of wrapper-based CLI execution.
- `src/cmd_daemon_win.h`
  - Update the task-action helper declaration to reflect the new scheduled helper path.
- `tests/test_cmd_daemon_win.c`
  - Update the Windows task-action unit test to expect `muslimtify-service.exe`.
- `README.md`
  - Update Windows daemon wording if the current text still implies direct CLI scheduling.
- `CHANGELOG.md`
  - Record the user-visible fix for flashing scheduled-task windows on Windows.

### New files to create

- `src/check_cycle.c`
  - Shared implementation for one notification check cycle.
- `include/check_cycle.h`
  - Public declaration for the shared check-cycle entrypoint.
- `src/muslimtify_service_win.c`
  - Windows-only background helper entrypoint using `WinMain`.

## Task 1: Extract The Shared Check Cycle

**Files:**
- Create: `include/check_cycle.h`
- Create: `src/check_cycle.c`
- Modify: `src/cmd_show.c`

- [ ] **Step 1: Write the failing integration test target expectation**

Document the intended seam before writing code:

- `handle_check()` should become a thin wrapper.
- The extracted function should return the same status codes as the current CLI path.
- The shared function should own the exact logic currently inside `handle_check()`, not broaden scope.

Verification command:

```powershell
rg -n "int handle_check|notify_init_once|cache_build_triggers|cache_save" src\cmd_show.c
```

Expected:
- the existing check-cycle logic still lives entirely in `src/cmd_show.c` before extraction

- [ ] **Step 2: Move the current `handle_check()` body into `run_check_cycle()`**

Implementation requirements:
- Add `int run_check_cycle(void);` to `include/check_cycle.h`.
- Implement `run_check_cycle()` in `src/check_cycle.c`.
- Copy the current `handle_check()` behavior exactly:
  - config load
  - location resolution
  - current time handling
  - cache load/rebuild
  - notification init/send/cleanup
  - cache persistence after notification delivery
- Keep output and error strings unchanged unless the refactor requires a precise adjustment.

- [ ] **Step 3: Make `handle_check()` a thin wrapper**

Implementation target:

```c
int handle_check(int argc, char **argv) {
  (void)argc;
  (void)argv;
  return run_check_cycle();
}
```

- [ ] **Step 4: Build the Windows CLI target**

Run:

```powershell
cmake --build build --config Debug --target muslimtify
```

Expected:
- `muslimtify.exe` builds successfully

- [ ] **Step 5: Commit**

```bash
git add include/check_cycle.h src/check_cycle.c src/cmd_show.c
git commit -m "refactor: extract shared check cycle"
```

## Task 2: Add The Windows Service Helper Executable

**Files:**
- Create: `src/muslimtify_service_win.c`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the helper entrypoint**

Implementation requirements:
- Use `WinMain` so the helper builds as a Windows GUI-subsystem executable.
- Initialize curl, call `run_check_cycle()`, clean up curl, and return the same result code.
- Avoid console output on successful runs.

Suggested shape:

```c
int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev, LPSTR cmd, int show) {
  (void)instance;
  (void)prev;
  (void)cmd;
  (void)show;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  int result = run_check_cycle();
  curl_global_cleanup();
  return result;
}
```

- [ ] **Step 2: Add the helper target in CMake**

Implementation requirements:
- Build `muslimtify-service` only on Windows.
- Link the same Windows dependencies already required by the main executable.
- Set the target so it uses the Windows subsystem rather than the console subsystem.
- Keep Linux build behavior unchanged.

- [ ] **Step 3: Build the helper target**

Run:

```powershell
cmake --build build --config Debug --target muslimtify-service
```

Expected:
- `build\bin\Debug\muslimtify-service.exe` is produced successfully

- [ ] **Step 4: Commit**

```bash
git add CMakeLists.txt src/muslimtify_service_win.c
git commit -m "feat: add windows service helper binary"
```

## Task 3: Update Windows Daemon Registration

**Files:**
- Modify: `src/cmd_daemon_win.c`
- Modify: `src/cmd_daemon_win.h`
- Modify: `tests/test_cmd_daemon_win.c`

- [ ] **Step 1: Update the task-action builder test first**

Adjust the Windows unit test so it expects:
- direct scheduling of `muslimtify-service.exe`
- no PowerShell wrapper text

Run:

```powershell
cmake --build build --config Debug --target test_cmd_daemon_win
& .\build\bin\Debug\test_cmd_daemon_win.exe
```

Expected:
- FAIL because the production code still builds the old action string

- [ ] **Step 2: Replace the task-action builder implementation**

Implementation requirements:
- Resolve the helper path from the executable directory.
- Build the scheduled-task action for `muslimtify-service.exe`.
- Remove the temporary PowerShell hidden-launch logic.
- Fail clearly if the helper executable cannot be found.

- [ ] **Step 3: Re-run the Windows daemon unit test**

Run:

```powershell
cmake --build build --config Debug --target test_cmd_daemon_win
& .\build\bin\Debug\test_cmd_daemon_win.exe
```

Expected:
- PASS

- [ ] **Step 4: Commit**

```bash
git add src/cmd_daemon_win.c src/cmd_daemon_win.h tests/test_cmd_daemon_win.c
git commit -m "fix: schedule windows service helper"
```

## Task 4: Verify Windows Build And Test Coverage

**Files:**
- Modify: `CMakeLists.txt` if test/build wiring still needs correction

- [ ] **Step 1: Build the Windows targets**

Run:

```powershell
cmake --build build --config Debug --target muslimtify muslimtify-service test_cmd_daemon_win
```

Expected:
- all requested targets build successfully

- [ ] **Step 2: Run the Windows-available test subset**

Run:

```powershell
ctest --test-dir build -C Debug --output-on-failure -R "cmd_daemon_win|platform|prayertimes|json|string_util"
```

Expected:
- all selected tests pass

- [ ] **Step 3: Verify the CLI help still looks right**

Run:

```powershell
& .\build\bin\Debug\muslimtify.exe help
```

Expected:
- daemon help still describes scheduled-task management correctly

- [ ] **Step 4: Commit any remaining build/test wiring**

```bash
git add CMakeLists.txt .github/workflows/ci.yml
git commit -m "test: cover windows service helper flow"
```

## Task 5: Document The Production Windows Fix

**Files:**
- Modify: `README.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Update README**

Document that on Windows:
- `muslimtify daemon install` registers a scheduled task
- the task runs `muslimtify-service.exe`
- this helper is an internal background executable used to avoid visible console flashing

- [ ] **Step 2: Update CHANGELOG**

Add a user-visible note that Windows scheduled checks no longer launch through a flashing console or
PowerShell wrapper.

- [ ] **Step 3: Verify wording**

Run:

```powershell
rg "muslimtify-service|scheduled task|PowerShell|flash|windows" README.md CHANGELOG.md
```

Expected:
- docs reflect the new helper-based behavior consistently

- [ ] **Step 4: Commit**

```bash
git add README.md CHANGELOG.md
git commit -m "docs: describe windows service helper"
```

## Task 6: Final Manual Validation

**Files:**
- Modify: none

- [ ] **Step 1: Reinstall the Windows scheduled task**

Run:

```powershell
& .\build\bin\Debug\muslimtify.exe daemon uninstall
& .\build\bin\Debug\muslimtify.exe daemon install
```

Expected:
- the scheduled task is recreated successfully

- [ ] **Step 2: Inspect the registered task action**

Run:

```powershell
schtasks /query /tn "muslimtify" /v /fo list
```

Expected:
- the task action references `muslimtify-service.exe`

- [ ] **Step 3: Manual runtime check**

Wait for the next scheduled minute tick and confirm:
- no visible console or PowerShell flash appears
- notification behavior still works when triggers match

- [ ] **Step 4: Review worktree**

Run:

```powershell
git status --short
git log --oneline -8
```

Expected:
- worktree clean
- commits are narrow and reviewable

## Notes For Execution

- Do not change Linux daemon behavior.
- Do not turn this into a persistent Windows Service.
- Keep `muslimtify-service.exe` internal; `muslimtify.exe` remains the public CLI command.
- Reuse the shared check-cycle logic instead of duplicating notification flow in the helper.
