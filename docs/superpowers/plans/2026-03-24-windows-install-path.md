# Windows Install PATH Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Update the Windows installer so it adds Muslimtify's installed `bin` directory to the user `PATH` and tells the user to restart PowerShell or open a new terminal.

**Architecture:** Keep the change user-level and localized to the installer. `install.ps1` should append `%LOCALAPPDATA%\Programs\Muslimtify\bin` to the user `PATH` only if it is missing, leave existing entries untouched, and print clear post-install messaging. README and CHANGELOG should document the new behavior.

**Tech Stack:** PowerShell, existing Windows install layout, README/CHANGELOG documentation

---

## File Structure

### Existing files to modify

- `install.ps1`
  - Add user `PATH` management after successful install.
- `README.md`
  - Mention that the Windows installer adds the bin directory to the user `PATH`.
- `CHANGELOG.md`
  - Record the new PATH update behavior.

## Task 1: Update The Windows Installer PATH Behavior

**Files:**
- Modify: `install.ps1`

- [ ] **Step 1: Resolve the installed bin directory from the existing install prefix**

Implementation requirements:
- Reuse the current install-path variables in the script.
- Target:

```text
$env:LOCALAPPDATA\Programs\Muslimtify\bin
```

- [ ] **Step 2: Read and inspect the current user PATH**

Implementation requirements:
- Read the user `PATH`, not the machine `PATH`.
- Detect whether the Muslimtify bin path is already present.
- Avoid duplicate entries.

- [ ] **Step 3: Update the user PATH only when needed**

Implementation requirements:
- Append the bin directory to the user `PATH` only if it is missing.
- Keep the change user-level only.
- If the PATH update fails, report the failure clearly.

- [ ] **Step 4: Update installer output**

Implementation requirements:
- Print whether the bin directory was added or already present.
- Tell the user to restart PowerShell or open a new terminal before using `muslimtify`.
- Keep the existing install summary with the full installed binary path.

- [ ] **Step 5: Verify the script behavior**

Run the narrowest safe verification available, for example:

```powershell
powershell -ExecutionPolicy Bypass -File .\install.ps1
```

Expected:
- install still succeeds
- the user PATH gains the Muslimtify bin path if it was absent
- output includes the restart/new-terminal note

If full install verification is not practical in the current environment, state the exact limitation honestly.

- [ ] **Step 6: Commit**

```bash
git add install.ps1
git commit -m "feat: add muslimtify to windows path"
```

## Task 2: Document The PATH Behavior

**Files:**
- Modify: `README.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Update README**

Implementation requirements:
- In the Windows install section, note that the installer adds the Muslimtify bin directory to the user `PATH`.
- Note that the user must restart PowerShell or open a new terminal before `muslimtify` is available as a plain command.
- Keep wording concise.

- [ ] **Step 2: Update CHANGELOG**

Implementation requirements:
- Add a note that the Windows installer now updates the user `PATH`.

- [ ] **Step 3: Verify doc references**

Run:

```powershell
rg "install\.ps1|PATH|LOCALAPPDATA|PowerShell|terminal" README.md CHANGELOG.md
```

Expected:
- README and CHANGELOG both mention the PATH behavior
- README includes the restart/new-terminal note

- [ ] **Step 4: Commit**

```bash
git add README.md CHANGELOG.md
git commit -m "docs: document windows path setup"
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

- [ ] **Step 2: Keep verification claims honest**

Implementation requirements:
- Do not claim immediate shell availability unless a new terminal was actually used.
- Do not claim PATH updates succeeded unless the user PATH value was really checked after install.

## Notes For Execution

- Keep the change user-level only.
- Do not add machine-wide PATH logic.
- Do not create duplicate PATH entries.
- Prefer exact matching of the installed bin directory over broad substring checks.
