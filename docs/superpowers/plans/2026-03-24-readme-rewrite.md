# README Rewrite Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rewrite `README.md` into a concise, production-ready user guide for installing and configuring Muslimtify on Linux and Windows.

**Architecture:** Keep the work scoped to `README.md`. Replace the current long-form mixed audience document with a shorter operations-first structure centered on Overview, Installation, Configuration, Troubleshooting, Contributing, License, and Support, while preserving all necessary user-facing install paths and configuration details.

**Tech Stack:** Markdown, existing CLI/install behavior, current config paths and install flows

---

## File Structure

### Existing files to modify

- `README.md`
  - Rewrite the document for end users, keeping only the sections and details required by the approved spec.

## Task 1: Rewrite README Structure And Core User Content

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Replace the current introduction with a concise cross-platform overview**

Implementation requirements:
- State that Muslimtify calculates prayer times locally.
- State that Linux and Windows are both supported.
- State that configuration is stored in the user profile.
- Mention that periodic background checks are used to deliver notifications.
- Do not include a warning banner, feature-marketing list, or large sample output block.

- [ ] **Step 2: Rewrite the installation section**

Implementation requirements:
- Keep these install methods:
  - Arch Linux (AUR)
  - Fedora (COPR)
  - Debian/Ubuntu (PPA)
  - Linux source install
  - Windows source install
- Ensure every install flow ends with `muslimtify daemon install`.
- In the Windows source install subsection, add a short beta note:
  - Windows support is currently beta.
  - Users should expect rough edges and occasional breaking changes.

- [ ] **Step 3: Replace quick-start/command-catalog style content with a focused configuration section**

Implementation requirements:
- Include both configuration approaches:
  - command line
  - manual JSON editing
- Document:
  - Linux config path: `~/.config/muslimtify/config.json`
  - Windows config path: `%APPDATA%\\muslimtify\\config.json`
  - Linux cache path: `~/.cache/muslimtify`
  - Windows cache path: `%LOCALAPPDATA%\\muslimtify`
- Include only the most useful configuration commands:
  - `muslimtify location auto`
  - `muslimtify location set <lat> <lon>`
  - `muslimtify reminder all 30,15,5`
  - `muslimtify config show`
  - `muslimtify config validate`
  - `muslimtify config reset`
- Add a short note explaining that manual JSON editing is useful for precise control.

- [ ] **Step 4: Add the default config example in a collapsible section**

Implementation requirements:
- Use a `<details>` block with a short summary label.
- Include the default `config.json`.
- Keep the example readable and valid JSON.

- [ ] **Step 5: Rewrite troubleshooting and footer sections**

Implementation requirements:
- Keep troubleshooting short and practical.
- Cover:
  - notifications not appearing
  - location detection issues
  - config validation/reset
  - Windows toast delivery note about local system settings
- Replace the old development section with a short `Contributing` section pointing to `CONTRIBUTING.md`.
- Keep separate `License` and `Support` sections.
- Preserve the MIT reference and the existing support link if still present.

- [ ] **Step 6: Verify the rewritten README content**

Run:

```powershell
rg "^## |muslimtify daemon install|APPDATA|LOCALAPPDATA|CONTRIBUTING|MIT|ko-fi|Windows support is currently beta" README.md
```

Expected:
- section structure matches the spec
- installation includes `muslimtify daemon install`
- config paths are present
- Windows beta note is present
- Contributing, License, and Support sections exist

- [ ] **Step 7: Review the rendered flow manually**

Check in the file that:
- there is no standalone `How It Works` section
- there is no standalone `Quick Start` section
- there is no oversized CLI command catalog
- the document reads as a user guide rather than a developer reference

- [ ] **Step 8: Commit**

```bash
git add README.md
git commit -m "docs: rewrite readme for users"
```

## Notes For Execution

- Keep this rewrite scoped to `README.md` only.
- Do not change CLI semantics or invent new commands.
- Prefer concise prose over exhaustive command listings.
- If existing local user changes exist in `README.md`, perform the work in an isolated worktree before editing.
