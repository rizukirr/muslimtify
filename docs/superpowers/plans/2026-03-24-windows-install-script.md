# Windows Install Script Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a Windows user-level `install.ps1` that builds Muslimtify from source in `Release`, installs via `cmake --install`, and registers the scheduled task from the installed binary.

**Architecture:** Keep the installer script thin and let CMake own file placement. The PowerShell script should configure/build/install into `%LOCALAPPDATA%\Programs\Muslimtify`, then call the installed `muslimtify.exe daemon install` so Task Scheduler points at the installed `muslimtify-service.exe` helper rather than the build tree.

**Tech Stack:** PowerShell, CMake, existing install rules, Windows Task Scheduler, README/CHANGELOG documentation

---

## File Structure

### Existing files to modify

- `README.md`
  - Add a short Windows source-install section that uses `install.ps1`.
- `CHANGELOG.md`
  - Record the new Windows installer workflow.

### New files to create

- `install.ps1`
  - Windows-only user-level installer that builds from source, installs with `cmake --install`, and registers the scheduled task.

## Task 1: Add The Windows Installer Script

**Files:**
- Create: `install.ps1`

- [ ] **Step 1: Define the installer inputs and paths**

Implementation requirements:
- Resolve the repository root from the script location.
- Set a release build directory, for example `build-release`.
- Set the default install prefix to:

```text
$env:LOCALAPPDATA\Programs\Muslimtify
```

- Surface the runtime data paths in script output:
  - `%APPDATA%\muslimtify`
  - `%LOCALAPPDATA%\muslimtify`

- [ ] **Step 2: Configure and build from source**

Run these commands from the script:

```powershell
cmake -S $RepoRoot -B $BuildDir -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$InstallPrefix
cmake --build $BuildDir --config Release
```

Expected:
- release build completes
- both `muslimtify.exe` and `muslimtify-service.exe` are produced

- [ ] **Step 3: Install with CMake**

Run from the script:

```powershell
cmake --install $BuildDir --config Release
```

Expected:
- binaries land under `$InstallPrefix\bin`
- installer does not manually copy files that CMake already installs

- [ ] **Step 4: Register the daemon from the installed binary**

Run from the script:

```powershell
& "$InstallPrefix\bin\muslimtify.exe" daemon install
```

Expected:
- scheduled task is registered against the installed helper path

- [ ] **Step 5: Print post-install summary**

Output should include:
- install prefix
- installed binary path
- config path
- cache path
- follow-up commands:
  - `muslimtify.exe help`
  - `muslimtify.exe location auto`
  - `muslimtify.exe daemon status`

Keep output plain and avoid emoticon-style symbols.

- [ ] **Step 6: Commit**

```bash
git add install.ps1
git commit -m "feat: add windows install script"
```

## Task 2: Document The Windows Installer

**Files:**
- Modify: `README.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Update README**

Add a short Windows source-install section showing:

```powershell
.\install.ps1
```

Document:
- default install prefix: `%LOCALAPPDATA%\Programs\Muslimtify`
- config path: `%APPDATA%\muslimtify`
- cache path: `%LOCALAPPDATA%\muslimtify`
- the installer registers the scheduled task after install

- [ ] **Step 2: Update CHANGELOG**

Add a note describing the new Windows user-level source installer and its use of the installed
helper-based daemon flow.

- [ ] **Step 3: Verify doc wording**

Run:

```powershell
rg "install.ps1|LOCALAPPDATA|APPDATA|muslimtify-service|daemon install" README.md CHANGELOG.md
```

Expected:
- docs reference the installer and Windows paths consistently

- [ ] **Step 4: Commit**

```bash
git add README.md CHANGELOG.md
git commit -m "docs: document windows install script"
```

## Task 3: Verify The Installer Workflow

**Files:**
- Modify: `install.ps1` only if verification reveals a real issue

- [ ] **Step 1: Run the installer**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\install.ps1
```

Expected:
- configure/build/install steps succeed
- installed binaries exist under `%LOCALAPPDATA%\Programs\Muslimtify\bin`
- `daemon install` runs from the installed binary

- [ ] **Step 2: Verify installed files**

Run:

```powershell
Get-ChildItem "$env:LOCALAPPDATA\Programs\Muslimtify\bin"
```

Expected:
- `muslimtify.exe`
- `muslimtify-service.exe`

- [ ] **Step 3: Verify daemon status**

Run:

```powershell
& "$env:LOCALAPPDATA\Programs\Muslimtify\bin\muslimtify.exe" daemon status
```

Expected:
- scheduled task exists or returns the expected scheduler status output

- [ ] **Step 4: Review worktree**

Run:

```powershell
git status --short
git log --oneline -6
```

Expected:
- worktree clean
- commits are narrow and reviewable

## Notes For Execution

- Keep this installer user-level only.
- Do not add machine-wide installation logic.
- Reuse `cmake --install`; do not manually duplicate CMake install rules.
- The installer should use the installed `muslimtify.exe` to register the daemon, not a build-tree binary.
