# Windows Install PATH Design

## Goal

Improve the Windows install experience so a successful `install.ps1` makes the
`muslimtify` command available in future terminals without requiring the user to
manually edit `PATH`.

## Scope

This change applies to the Windows installer flow only.

It should update:
- `install.ps1`
- `README.md`
- `CHANGELOG.md`

It should not:
- modify machine-wide `PATH`
- require Administrator privileges
- change the install prefix

## Target Behavior

After a successful Windows install, `install.ps1` should:
- add `%LOCALAPPDATA%\Programs\Muslimtify\bin` to the user `PATH` if it is not
  already present
- leave `PATH` unchanged if the entry already exists
- tell the user to restart PowerShell or open a new terminal before running
  `muslimtify`

The script should keep the current install summary that shows the full installed
binary path.

## Requirements

### PATH Update Rules

- Update only the user `PATH`.
- Avoid duplicate entries.
- Match the exact installed bin directory:

```text
%LOCALAPPDATA%\Programs\Muslimtify\bin
```

- If the PATH update fails, the installer should report the failure clearly
  instead of silently ignoring it.

### User Messaging

The installer output should say one of:
- the bin path was added to the user `PATH`
- the bin path was already present

It should also say that the user must restart PowerShell or open a new terminal
before `muslimtify` will be available as a plain command.

## Documentation

README should mention:
- Windows install adds the Muslimtify bin directory to the user `PATH`
- a new terminal is required before the command is available

CHANGELOG should note:
- the Windows installer now updates the user `PATH`

## Expected Outcome

After installation:
- the user can run `muslimtify` in new terminals without manual PATH editing
- repeated installs do not duplicate PATH entries
