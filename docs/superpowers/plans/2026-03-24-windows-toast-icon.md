# Windows Toast Icon Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Muslimtify icon branding to Windows toast notifications while preserving the current exact-prayer reminder scenario behavior.

**Architecture:** Keep the change local to the Windows notification backend. Add executable-relative icon lookup and optional image XML generation in `src/notification_win.c`, then cover the path-resolution behavior with Windows-only tests so installed and development layouts are both exercised.

**Tech Stack:** C99, Win32 path APIs, WinRT toast XML, CMake, existing Windows unit test targets

---

## File Structure

### Existing files to modify

- `src/notification_win.c`
  - Add icon lookup helpers and inject the optional toast image node.
- `CMakeLists.txt`
  - Register a Windows-only test target for the new icon resolution tests.

### New files to create

- `tests/test_notification_win.c`
  - Windows-only tests for executable-relative icon resolution and fallback behavior.

## Task 1: Add Windows Icon Resolution Helpers

**Files:**
- Modify: `src/notification_win.c`

- [ ] **Step 1: Add a narrow file-exists helper for wide paths**

Implementation requirements:
- Use a Win32 API that works with Unicode paths.
- Return a simple boolean-style result.
- Keep the helper file-local.

- [ ] **Step 2: Add an executable-relative path builder**

Implementation requirements:
- Start from the current executable directory.
- Build candidate paths for:
  - `..\share\icons\hicolor\128x128\apps\muslimtify.png`
  - `..\share\pixmaps\muslimtify.png`
  - `..\assets\muslimtify.png`
  - `assets\muslimtify.png`
- Avoid hardcoded user-profile paths.

- [ ] **Step 3: Add icon resolution logic with ordered fallback**

Implementation requirements:
- Try candidates in the exact order from the spec.
- Return the first existing path.
- If nothing exists, return an empty result so the caller can omit the image.

- [ ] **Step 4: Commit**

```bash
git add src/notification_win.c
git commit -m "refactor: add windows toast icon lookup"
```

## Task 2: Add Optional Toast Image XML

**Files:**
- Modify: `src/notification_win.c`

- [ ] **Step 1: Reuse the existing XML escaping path for image payload data**

Implementation requirements:
- Do not change the public notification API.
- Keep escaping local to the backend.
- Ensure a resolved path can be safely embedded in XML.

- [ ] **Step 2: Extend the toast XML builder to include an optional image node**

Implementation requirements:
- When an icon path exists, include an image node inside `ToastGeneric`.
- When no icon path exists, keep the current text-only payload.
- Preserve both payload forms:
  - `scenario="reminder"` for exact prayer notifications
  - default scenario for reminders and generic sends

- [ ] **Step 3: Keep exact-prayer urgency behavior unchanged**

Implementation requirements:
- `minutes_before == 0` must still map to `scenario="reminder"`.
- `minutes_before > 0` must remain normal toast behavior by default.
- No Linux code should change.

- [ ] **Step 4: Build the Windows targets**

Run:

```powershell
cmake --build build --config Debug --target muslimtify muslimtify-service
```

Expected:
- build succeeds
- no linker or compile errors in `src/notification_win.c`

- [ ] **Step 5: Commit**

```bash
git add src/notification_win.c
git commit -m "feat: add icon to windows toast notifications"
```

## Task 3: Add Windows Notification Backend Tests

**Files:**
- Create: `tests/test_notification_win.c`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add a test seam for icon resolution**

Implementation requirements:
- Expose only the minimal internal helper(s) needed by the test.
- Keep them Windows-only.
- Avoid exposing broader backend internals than necessary.

- [ ] **Step 2: Write tests for installed-layout resolution**

Test requirements:
- simulate an executable directory
- verify the installed icon path is preferred:
  - `..\share\icons\hicolor\128x128\apps\muslimtify.png`
  - then `..\share\pixmaps\muslimtify.png`

- [ ] **Step 3: Write tests for development-layout resolution**

Test requirements:
- verify fallback to:
  - `..\assets\muslimtify.png`
  - then `assets\muslimtify.png`

- [ ] **Step 4: Write tests for no-icon fallback**

Test requirements:
- verify the resolver returns an empty/not-found result
- verify this case is non-fatal to the caller contract

- [ ] **Step 5: Register the test target in CMake**

Implementation requirements:
- Windows-only target
- add it to CTest
- follow the existing style used for `test_cmd_daemon_win`

- [ ] **Step 6: Run the new test directly**

Run:

```powershell
cmake --build build --config Debug --target test_notification_win
.\build\bin\Debug\test_notification_win.exe
```

Expected:
- test executable builds
- test process exits successfully

- [ ] **Step 7: Commit**

```bash
git add tests/test_notification_win.c CMakeLists.txt src/notification_win.c
git commit -m "test: cover windows toast icon resolution"
```

## Task 4: Verify Windows Behavior

**Files:**
- Modify: touched files only if verification reveals a real issue

- [ ] **Step 1: Run the Windows cross-platform test subset**

Run:

```powershell
ctest --test-dir build -C Debug --output-on-failure -R "notification_win|cmd_daemon_win|platform|prayertimes|json|string_util"
```

Expected:
- all selected tests pass

- [ ] **Step 2: Run the toast smoke test**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\test_notification.ps1
```

Expected:
- the script runs successfully if the host allows toast delivery
- if the environment blocks toast delivery, capture the real error instead of claiming success

- [ ] **Step 3: Review worktree state**

Run:

```powershell
git status --short
git log --oneline -6
```

Expected:
- worktree clean
- commits are narrow and reviewable

## Notes For Execution

- Keep this change Windows-only.
- Do not change the public notification API unless tests prove it is necessary.
- Do not attempt to solve Windows packaged-app identity in this plan.
- The toast image is supplemental branding, not a replacement for the shell-owned app identity icon.
