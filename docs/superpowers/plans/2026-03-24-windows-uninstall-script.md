# Windows Uninstall Script Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a user-level `uninstall.ps1` that removes Muslimtify completely from the current Windows user profile after explicit confirmation.

**Architecture:** Keep the uninstall flow script-based and user-level, parallel to `install.ps1`. The script should summarize what it will delete, require a `yes` confirmation, uninstall the scheduled task if possible, and then remove the install/config/cache directories without failing on missing paths.

**Tech Stack:** PowerShell, existing Windows install layout, Task Scheduler CLI through installed `muslimtify.exe`, README/CHANGELOG documentation

---

## File Structure

### Existing files to modify

- `README.md`
  - Mention the new Windows uninstall command alongside `install.ps1`.
- `CHANGELOG.md`
  - Record the new Windows uninstall workflow.

### New files to create

- `uninstall.ps1`
  - Windows user-level uninstaller with confirmation prompt and full user-data cleanup.

## Task 1: Add The Windows Uninstall Script

**Files:**
- Create: `uninstall.ps1`

- [ ] **Step 1: Define the target paths and task name**

Implementation requirements:
- Resolve:
  - install prefix: `%LOCALAPPDATA%\Programs\Muslimtify`
  - installed binary: `%LOCALAPPDATA%\Programs\Muslimtify\bin\muslimtify.exe`
  - config directory: `%APPDATA%\muslimtify`
  - cache directory: `%LOCALAPPDATA%\muslimtify`
  - scheduled task name: `muslimtify`
- Keep the script user-level only.

- [ ] **Step 2: Print the destructive summary and ask for confirmation**

Implementation requirements:
- Show the exact task name and paths that will be removed.
- Require a simple `yes` confirmation before continuing.
- Exit without changes for any other input.

- [ ] **Step 3: Uninstall the scheduled task if possible**

Implementation requirements:
- If the installed `muslimtify.exe` exists, run:

```powershell
& $MuslimtifyExe daemon uninstall
```

- If the binary is missing, continue without failing the whole script.
- Missing task or uninstall failure should be reported clearly but should not block file cleanup.

- [ ] **Step 4: Remove install, config, and cache directories**

Implementation requirements:
- Remove each path only if it exists.
- Missing paths should not fail the script.
- Keep output concise and explicit about what was removed or skipped.

- [ ] **Step 5: Print completion summary**

Implementation requirements:
- Confirm that uninstall is complete.
- Summarize which locations were removed.

- [ ] **Step 6: Verify the script syntax**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\uninstall.ps1
```

Expected:
- the script prints the confirmation prompt
- exiting without typing `yes` leaves the system unchanged

- [ ] **Step 7: Commit**

```bash
git add uninstall.ps1
git commit -m "feat: add windows uninstall script"
```

## Task 2: Document The Windows Uninstall Flow

**Files:**
- Modify: `README.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Update README**

Implementation requirements:
- Add `.\uninstall.ps1` to the Windows install/uninstall guidance.
- Keep README wording concise and user-focused.

- [ ] **Step 2: Update CHANGELOG**

Implementation requirements:
- Note that Windows now has a user-level uninstall script.
- Mention that it removes installed files and user data after confirmation.

- [ ] **Step 3: Verify documentation references**

Run:

```powershell
rg "install\.ps1|uninstall\.ps1|LOCALAPPDATA|APPDATA" README.md CHANGELOG.md
```

Expected:
- README and CHANGELOG both mention the Windows uninstall flow

- [ ] **Step 4: Commit**

```bash
git add README.md CHANGELOG.md
git commit -m "docs: document windows uninstall script"
```

## Task 3: Verify The End State

**Files:**
- Modify touched files only if verification reveals a real issue

- [ ] **Step 1: Review worktree state**

Run:

```powershell
git status --short
git log --oneline -6
```

Expected:
- worktree clean
- commits are narrow and reviewable

- [ ] **Step 2: Keep runtime verification honest**

Implementation requirements:
- If you do not actually run the destructive uninstall path, state that clearly in the final handoff.
- Do not claim end-to-end uninstall succeeded unless it was really executed.

## Notes For Execution

- Keep this uninstall script destructive by default, but confirmation-gated.
- Do not add machine-wide uninstall logic.
- Do not fail the whole script on missing paths.
- Avoid broad refactoring; keep the change limited to the script and the two docs files.
