# Windows Install Script Design

## Summary

Add a Windows user-level installer script, `install.ps1`, that mirrors the role of `install.sh`
while staying native to the current Windows packaging model.

The script will build Muslimtify from source in `Release`, install via `cmake --install` into a
user-local prefix under `%LOCALAPPDATA%\Programs\Muslimtify`, and then register the scheduled task
through the installed `muslimtify.exe`.

## Goals

- Provide a one-command installer for Windows developers and users building from source.
- Keep installation user-level only with no Administrator requirement.
- Reuse the existing CMake install rules rather than manually copying binaries.
- Install both `muslimtify.exe` and `muslimtify-service.exe`.
- Register the Windows scheduled task after installation.

## Non-goals

- Machine-wide installation under `Program Files`.
- Implementing a Windows uninstaller script in this change.
- Replacing release artifacts or a future installer package.
- Changing Linux install behavior.

## Design

### Script

- New file: `install.ps1`

### Install prefix

Default Windows install prefix:

```text
%LOCALAPPDATA%\Programs\Muslimtify
```

This keeps the installation writable by the current user and avoids elevation.

### Build flow

The script should:

1. Resolve the repository root from the script location.
2. Use a dedicated release build directory, for example:
   - `build-release`
3. Configure CMake in `Release` mode with:
   - `-DCMAKE_INSTALL_PREFIX=<user-local-prefix>`
4. Build the project in `Release`.
5. Run `cmake --install`.

### Installed artifacts

The script relies on current CMake install rules to place:

- `muslimtify.exe`
- `muslimtify-service.exe`
- installed icon assets

The script should not manually duplicate the CMake install logic unless required for a small Windows
specific gap.

### Runtime data locations

The installer should document the Windows runtime paths separately from the install prefix:

- installed binaries:
  - `%LOCALAPPDATA%\Programs\Muslimtify\bin`
- config directory:
  - `%APPDATA%\muslimtify`
- cache directory:
  - `%LOCALAPPDATA%\muslimtify`

The script should not try to relocate config or cache into the install prefix. Those paths remain
owned by the existing platform layer and are created on first run when needed.

### Post-install daemon registration

After installation, the script should run:

```powershell
<install-prefix>\bin\muslimtify.exe daemon install
```

This ensures the installed scheduled task points at the installed helper binary rather than a build
tree path.

### UX

The script should print:

- current install prefix
- build directory
- install completion message
- installed binary path
- config path
- cache path
- short follow-up commands, such as:
  - `muslimtify.exe help`
  - `muslimtify.exe location auto`
  - `muslimtify.exe daemon status`

The script should avoid emoticon or symbol-heavy output.

## Error handling

- Fail fast if CMake is unavailable.
- Fail fast if build or install steps fail.
- Fail clearly if the installed binary cannot be found after `cmake --install`.
- Fail clearly if `muslimtify.exe daemon install` fails.

## Documentation

Update `README.md` with a short Windows source-install section showing:

```powershell
.\install.ps1
```

Update `CHANGELOG.md` because this adds a new user-facing installer workflow.

## Risks

- Scheduled-task registration still depends on local Windows policy and may require the user to
  rerun the script in a session with sufficient permissions.
- The script assumes the user has a supported CMake/MSVC environment available.
- If CMake install rules change later, the script must remain aligned with them.

## Recommendation

Implement `install.ps1` as a user-level Windows source installer that delegates file placement to
`cmake --install` and then registers the scheduled task from the installed binary.
