# Windows PATH Symmetry Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the Windows install and uninstall scripts manage the Muslimtify user `PATH` entry symmetrically.

**Architecture:** Keep the behavior user-level only. `install.ps1` should continue to add the installed `bin` directory to the user `PATH` without duplicates, while `uninstall.ps1` should remove all canonical-equivalent entries for that same directory and fall back to a clear manual message if PATH cleanup cannot be completed.

**Tech Stack:** PowerShell, existing Windows install layout, README/CHANGELOG documentation if wording needs adjustment

---

## File Structure

### Existing files to modify

- `install.ps1`
  - Keep the add-to-PATH behavior aligned with the final canonical path rules.
- `uninstall.ps1`
  - Remove Muslimtify PATH entries during uninstall.
- `README.md`
  - Update wording if needed so install/uninstall PATH behavior is accurate.
- `CHANGELOG.md`
  - Update wording if needed so uninstall PATH cleanup is documented.

## Task 1: Add PATH Cleanup To Windows Uninstall

**Files:**
- Modify: `uninstall.ps1`

- [ ] **Step 1: Resolve the installed bin path used for PATH cleanup**

Implementation requirements:
- Target the same Muslimtify bin directory used by `install.ps1`:

```text
%LOCALAPPDATA%\Programs\Muslimtify\bin
```

- [ ] **Step 2: Reuse or add canonical path comparison logic**

Implementation requirements:
- Remove entries that are:
  - exact matches
  - duplicate entries
  - canonical-equivalent entries that resolve to the same location
- Keep unrelated user PATH entries untouched.

- [ ] **Step 3: Remove PATH entries during uninstall**

Implementation requirements:
- Perform PATH cleanup as part of the uninstall flow.
- Do not block task/file/config/cache removal if PATH cleanup fails.
- If PATH cleanup succeeds, print whether the entry was removed or already absent.
- If PATH cleanup fails, print a clear manual cleanup message with the exact path to remove.

- [ ] **Step 4: Verify non-destructive uninstall behavior still works**

Run:

```powershell
cmd /c "echo no|powershell -ExecutionPolicy Bypass -File .\uninstall.ps1"
```

Expected:
- the destructive summary still appears
- the script still aborts cleanly without changes when the answer is not `yes`

- [ ] **Step 5: Commit**

```bash
git add uninstall.ps1
git commit -m "feat: remove muslimtify from windows path"
```

## Task 2: Reconcile Install And Uninstall PATH Rules

**Files:**
- Modify: `install.ps1`

- [ ] **Step 1: Review canonical path handling in install.ps1**

Implementation requirements:
- Confirm install-side duplicate detection still matches the uninstall-side cleanup rules.
- Adjust only if needed so both scripts agree on what counts as the same PATH entry.

- [ ] **Step 2: Keep install messaging accurate**

Implementation requirements:
- Preserve:
  - path added
  - path already present
  - restart/new-terminal note
- Do not add machine-wide PATH logic.

- [ ] **Step 3: Verify script syntax**

Run a narrow verification such as:

```powershell
powershell -NoProfile -Command "[scriptblock]::Create((Get-Content .\install.ps1 -Raw)) > $null; [scriptblock]::Create((Get-Content .\uninstall.ps1 -Raw)) > $null; 'PARSE_OK'"
```

Expected:
- `PARSE_OK`

- [ ] **Step 4: Commit**

```bash
git add install.ps1 uninstall.ps1
git commit -m "fix: align windows path handling"
```

## Task 3: Update Docs If Needed

**Files:**
- Modify: `README.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Update README wording if uninstall PATH cleanup is user-visible**

Implementation requirements:
- Keep wording concise.
- Mention uninstall behavior only if the current README would otherwise be misleading.

- [ ] **Step 2: Update CHANGELOG wording if needed**

Implementation requirements:
- Note that uninstall now also removes the Muslimtify user PATH entry when possible.

- [ ] **Step 3: Verify doc references**

Run:

```powershell
rg "install\.ps1|uninstall\.ps1|PATH|LOCALAPPDATA|PowerShell|terminal" README.md CHANGELOG.md
```

Expected:
- docs stay aligned with the actual install/uninstall behavior

- [ ] **Step 4: Commit**

```bash
git add README.md CHANGELOG.md
git commit -m "docs: update windows path cleanup notes"
```

## Task 4: Verify The End State

**Files:**
- Modify touched files only if verification reveals a real issue

- [ ] **Step 1: Review worktree state**

Run:

```powershell
git status --short
git log --oneline -8
```

Expected:
- worktree clean
- commits are narrow and reviewable

- [ ] **Step 2: Keep uninstall verification honest**

Implementation requirements:
- Do not claim destructive PATH removal was executed unless it was actually run with confirmation.
- It is acceptable to verify the non-destructive abort path and script parsing if destructive verification is not practical.

## Notes For Execution

- Keep all PATH changes user-level only.
- Do not add machine-wide PATH logic.
- Uninstall PATH cleanup failure should not block the rest of uninstall.
- Prefer symmetry between install and uninstall canonical path handling.
