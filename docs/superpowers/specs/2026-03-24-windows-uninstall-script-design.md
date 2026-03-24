# Windows Uninstall Script Design

## Goal

Add a user-level `uninstall.ps1` for Windows that removes Muslimtify entirely
from the current user profile.

## Scope

This change adds a Windows uninstall companion to `install.ps1`.

It should remove:
- the scheduled task
- installed binaries and assets
- config data
- cache data

It should not add machine-wide uninstall logic.

## Target Behavior

The script must be destructive, but it should ask for confirmation before
making any changes.

If the user confirms, the script should remove:
- scheduled task: `muslimtify`
- install prefix: `%LOCALAPPDATA%\Programs\Muslimtify`
- config directory: `%APPDATA%\muslimtify`
- cache directory: `%LOCALAPPDATA%\muslimtify`

If the user does not confirm, the script should exit without deleting anything.

## Uninstall Flow

1. Resolve the target paths from the current user environment.
2. Print a short summary of what will be removed.
3. Ask the user for confirmation with a clear destructive prompt.
4. If confirmed:
   - run `muslimtify.exe daemon uninstall` if the installed binary exists
   - continue even if the binary is missing
   - remove install/config/cache directories if they exist
5. Print a short completion summary.

## Error Handling

- Missing files or directories should not fail the whole uninstall.
- Missing installed binary should not block scheduled-task cleanup.
- The script should remain user-level and avoid requiring Administrator
  privileges.
- Output should stay plain and concise.

## Documentation

Update:
- `README.md`
- `CHANGELOG.md`

README should mention:
- Windows install with `.\install.ps1`
- Windows uninstall with `.\uninstall.ps1`

CHANGELOG should note:
- the new Windows uninstall script
- that it removes installed files and user data after confirmation

## Tone

The script prompt should be explicit about destructive behavior without being
verbose.

Example shape:
- show exact paths
- show the scheduled task name
- require `yes` to continue

## Expected Outcome

After implementation, a Windows user should be able to run one command,
confirm the prompt, and fully remove Muslimtify from their user profile.
