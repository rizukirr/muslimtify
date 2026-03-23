# Windows Toast Test Script Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a developer-only PowerShell script that sends sample Windows notifications to verify toast delivery on a development machine.

**Architecture:** Keep the change script-only from the app’s perspective: add `test_notification.ps1` as a Windows developer utility, document clearly that it tests Windows toast delivery only, and avoid any new muslimtify CLI command or compiled helper.

**Tech Stack:** PowerShell, existing repo scripts, README/CHANGELOG documentation

---

## File Structure

### Existing files to modify

- `README.md`
  - Add a short Windows developer note for running the toast-delivery smoke test.
- `CHANGELOG.md`
  - Record the new developer utility.

### New files to create

- `test_notification.ps1`
  - Windows-only developer script that sends sample notifications and prints troubleshooting guidance.

## Task 1: Add The Windows Toast Delivery Script

**Files:**
- Create: `test_notification.ps1`

- [ ] **Step 1: Write the script**

Implementation requirements:
- Print a short header stating this tests Windows toast delivery only.
- Verify the host is running on Windows.
- Send 2-3 sample notifications with prayer-style titles/messages.
- Pause briefly between notifications.
- Print troubleshooting notes after dispatching them.
- Keep the script self-contained and avoid changes to `muslimtify.exe`.

- [ ] **Step 2: Run the script to verify it executes**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\test_notification.ps1
```

Expected:
- script exits successfully
- sample notification messages are reported in the terminal
- Windows attempts to display the notifications

- [ ] **Step 3: Commit**

```bash
git add test_notification.ps1
git commit -m "feat: add windows toast test script"
```

## Task 2: Document The Script Clearly

**Files:**
- Modify: `README.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Update README**

Add a short note that Windows developers can run:

```powershell
.\test_notification.ps1
```

Make the scope explicit:
- it tests Windows toast delivery
- it does not prove muslimtify backend correctness

- [ ] **Step 2: Update CHANGELOG**

Add an entry describing the new Windows developer notification test script.

- [ ] **Step 3: Verify wording**

Run:

```powershell
rg "test_notification.ps1|toast|notification" README.md CHANGELOG.md
```

Expected:
- README and CHANGELOG mention the script consistently

- [ ] **Step 4: Commit**

```bash
git add README.md CHANGELOG.md
git commit -m "docs: document windows toast test script"
```

## Task 3: Final Verification

**Files:**
- Modify: none

- [ ] **Step 1: Re-run the script**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\test_notification.ps1
```

Expected:
- successful exit
- notifications dispatched

- [ ] **Step 2: Review worktree**

Run:

```powershell
git status --short
git log --oneline -5
```

Expected:
- worktree clean
- commits are narrow and reviewable

## Notes For Execution

- Do not add a new muslimtify CLI command.
- Do not add a compiled helper binary.
- Keep the script honest about its scope: Windows toast-delivery smoke test only.
