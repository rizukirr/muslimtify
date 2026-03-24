# Windows PATH Symmetry Design

## Goal

Make the Windows install and uninstall scripts handle the Muslimtify user `PATH`
entry symmetrically.

## Scope

This change applies to:
- `install.ps1`
- `uninstall.ps1`
- related README and changelog wording if needed

It does not change:
- install prefix
- machine-wide PATH
- task registration design

## Target Behavior

### Install

After a successful install, `install.ps1` should ensure the Muslimtify `bin`
directory is present in the user `PATH` without creating duplicate or
canonical-equivalent entries.

### Uninstall

During uninstall, `uninstall.ps1` should remove Muslimtify `bin` entries from
the user `PATH`.

Removal should cover:
- exact matches
- duplicate entries
- canonical-equivalent entries that resolve to the same Muslimtify `bin` path

## PATH Target

The target path is:

```text
%LOCALAPPDATA%\Programs\Muslimtify\bin
```

## Uninstall Failure Handling

If PATH cleanup fails during uninstall:
- do not block task/file/config/cache removal
- do not treat the whole uninstall as failed solely because of PATH cleanup
- print a clear manual cleanup message
- include the exact Muslimtify `bin` path the user should remove manually

## Messaging

Install should say:
- path added, or
- path already present

Uninstall should say:
- PATH entry removed, or
- PATH entry already absent, or
- PATH cleanup failed and must be done manually

## Expected Outcome

After these changes:
- install does not keep adding duplicate PATH entries
- uninstall removes existing Muslimtify PATH entries when possible
- users have a clear manual fallback if PATH cleanup cannot be completed
