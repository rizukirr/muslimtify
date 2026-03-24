# Windows Service Helper Design

## Summary

Replace the current Windows scheduled-task action that launches the console `muslimtify.exe` with
a Windows-only helper executable, `muslimtify-service.exe`, built without a console window.

The helper will run the same one-shot notification check logic as the existing `check` command, but
without flashing a terminal or depending on PowerShell/VBScript wrappers.

## Goals

- Eliminate the visible console or PowerShell flash on Windows scheduled runs.
- Keep `muslimtify.exe` as the normal user-facing CLI binary.
- Keep Linux behavior unchanged.
- Reuse the same check-and-notify logic for CLI and scheduled execution.
- Make the Windows scheduled-task action point directly to a muslimtify-owned executable.

## Non-goals

- Implementing a long-running Windows Service.
- Adding a visible GUI or tray application.
- Changing Linux daemon behavior or packaging.
- Changing notification timing or prayer-matching behavior.

## Design

### Executables

- `muslimtify.exe`
  - Existing CLI binary.
  - Remains the only user-facing Windows command.
- `muslimtify-service.exe`
  - Windows-only internal helper.
  - Used by Task Scheduler for background check execution.
  - Built as a Windows GUI-subsystem executable so no console window is created.

### Shared check-cycle boundary

Extract the current body of `handle_check()` into a shared function that performs one complete
notification check cycle:

1. Load config.
2. Ensure location data exists.
3. Resolve the current time.
4. Load or rebuild the day cache.
5. Fire notifications matching the current minute.
6. Persist cache updates when notifications are sent.

`handle_check()` becomes a thin CLI wrapper around this shared function.

`muslimtify-service.exe` also calls the same shared function so notification behavior stays aligned
between manual CLI invocation and scheduled background execution.

### Windows background entrypoint

Add a Windows-only source file for the service helper entrypoint, using `WinMain`:

- initialize curl
- call the shared check-cycle function
- clean up curl
- return the result code

The helper must not create any window or console output under normal success conditions.

### Windows daemon registration

Update the Windows daemon install path so Task Scheduler registers:

```text
<exe-dir>\muslimtify-service.exe
```

instead of:

```text
<exe-dir>\muslimtify.exe check
```

This removes the need for the temporary PowerShell hidden-launch wrapper.

### Path resolution

The scheduled-task registration should resolve `muslimtify-service.exe` from the installed
executable directory rather than hardcoding a separate install path.

If the helper binary cannot be found during `daemon install`, the command should fail clearly with
an actionable error.

## Build system changes

On Windows, CMake should build both:

- `muslimtify.exe`
- `muslimtify-service.exe`

The helper target should:

- link the same shared object library and Windows dependencies needed for notification/config/cache
  work
- use the Windows subsystem so it runs without a console window

Linux continues to build only the existing CLI binary.

## Testing

### Automated

- Keep the Windows unit test around scheduled-task action construction, but update it to expect
  `muslimtify-service.exe`.
- Add or update tests around the extracted shared check-cycle boundary if practical without forcing
  Linux-only dependencies into Windows test targets.
- Ensure Windows build/test jobs include the new helper target.

### Manual

1. Build Windows Debug or Release.
2. Run `muslimtify daemon uninstall`.
3. Run `muslimtify daemon install`.
4. Wait for a scheduled minute tick.
5. Confirm there is no visible console or PowerShell flash.
6. Confirm notifications still appear when the schedule matches a prayer trigger.

## Risks

- Refactoring `handle_check()` can accidentally change notification behavior if the extraction is
  not kept narrow.
- The helper executable introduces another Windows artifact, so packaging and install assumptions
  need to stay explicit.
- Task Scheduler path quoting still matters if the executable directory contains spaces.

## Recommendation

Implement `muslimtify-service.exe` as a Windows-only internal helper and move the scheduled-task
registration to that binary. This is the cleanest production-ready way to remove the visible window
flash while preserving the current one-shot scheduling model.
