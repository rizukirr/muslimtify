# Review — GUI restructure

**Date:** 2026-05-25
**Spec:** docs/specs/2026-05-25-gui-restructure-design.md
**Plan:** docs/plans/2026-05-25-gui-restructure.md
**Verify report:** docs/verifications/2026-05-25-gui-restructure-verify.md (verdict: ready)
**Commits under review:** 8014d40..53cad2f (7 code commits) on vibe/gui-restructure
(plus docs commits cdbca42 verification report + this review; code surface ends at 53cad2f)

## Diff summary

- Files changed: 20 (12 added, 4 modified, 1 deleted, 1 renamed)
- Lines added: 676, removed: 636 (net +40)
- Code commits: 7 (+ Task-8 formatting is folded into 5321549)

## Findings

### Block
- None.

### Warn
- None.

### Nit
- `src/gui/app/assets.h:4` includes `"ccompose.h"`, but the struct's types come from `<raylib.h>`
  (`Texture2D`) and `<stdint.h>` (`int16_t`); `ccompose.h` is not used directly in this header
  (clangd `unused-includes`). Carried verbatim from the plan; harmless (the compiler's
  `-Wall -Wextra` does not flag it) and arguably defensible since ccompose transitively provides
  raylib. Not worth a separate commit.

## Pass-by-pass notes

- **Pass 1 (spec coverage):** all 7 goals + 4 non-goals + 3 constraints satisfied — see verify
  report. Non-goals respected: no `screens/` dir, `colors.h`/`fonts.h` diff is 0 lines, all
  component entry points are parameterless (accessor-based, not context-threaded).
- **Pass 2 (plan fidelity):** every plan task's "Files" matches the diff; all 7 code-commit subjects
  are byte-identical to the plan's commit messages; commit order follows task order (rename → assets
  → topbar → navigation → dashboard → remove-legacy → entry layer).
- **Pass 3 (code quality):** the two icon resolvers (`NavIconTexture`, `PrayerIconTexture`) share a
  switch-over-enum shape but operate on different enums and different `Assets` fields — not
  extractable duplication. No exports without callers (12 `App_Assets()` call sites; every component
  entry point is called from `app.c` or `dashboard_content.c`).
- **Pass 4 (simplicity):** largest new file is `navigation.c` (157 lines), which is the verbatim
  relocation of the prior header-only implementation plus the enum/resolver mandated by the Assets
  refactor. No abstraction with a single caller that should be inlined; no speculative
  configurability. Nothing a senior engineer could halve without dropping required behavior.
- **Pass 5 (surgical-diff):** dispatched read-only audit returned `clean`, 0 orphans (recorded in
  verify report). Every hunk traces to a task.

## Self-critique (three risks the tests would not catch)

1. **Icon-id → texture mis-mapping.** A `switch` case could return the wrong field (e.g. FAJR →
   dhuhr texture); since any valid `Texture2D` renders, the GUI would still draw but show the wrong
   icon, and there are no GUI snapshot tests. *Mitigation:* per-task Gate-2 review verified each case
   maps its enum to the matching field, the enum ordering mirrors the original pointer assignments,
   and a manual launch rendered the nav + dashboard with the expected icons. *Residual follow-up:* a
   pixel-snapshot test would close this — out of scope for a no-behavior-change refactor.
2. **Default global font dropped.** `App_Run()` must keep `CC_LoadGlobalFont(FONT_INTER_REGULAR, 48)`
   / `CC_SetGlobalFontColor(...)`; losing it would silently change unstyled text. *Mitigation:*
   Gate-2 confirmed both calls are present in `app.c:31-32`; launch shows text rendering normally.
   Mitigated.
3. **Daily-schedule / nav state drift during the struct rewrite.** Converting the tables from raw
   `Texture2D*` to an icon-id enum could have perturbed the `isPast`/`isCurrent` flags or item
   order. *Mitigation:* Gate-2 confirmed the table's data fields (isPast/isCurrent/title/time) are
   preserved verbatim and the enum values align 1:1 with the original icon assignments. Mitigated.

## Diff

Full diff is large (676/636 across 20 files); review it with:

```
git diff 8014d40..53cad2f
```

Per-file summary (`git diff --name-status 8014d40..53cad2f`):

```
M  src/gui/CMakeLists.txt
A  src/gui/app/app.c
A  src/gui/app/app.h
A  src/gui/app/assets.c
A  src/gui/app/assets.h
A  src/gui/components/calculation_profile_card.c
A  src/gui/components/calculation_profile_card.h
A  src/gui/components/daily_schedule.c
A  src/gui/components/daily_schedule.h
A  src/gui/components/dashboard_content.c
M  src/gui/components/dashboard_content.h
A  src/gui/components/navigation.c
M  src/gui/components/navigation.h
A  src/gui/components/prayer_card.c
A  src/gui/components/prayer_card.h
A  src/gui/components/topbar.c
M  src/gui/components/topbar.h
D  src/gui/themes/app_assets.h
R095  src/gui/themes/assets.h -> src/gui/themes/asset_paths.h
M  src/muslimtify_gui.c
```

## Sign-off

- [ ] User reviewed findings.
- [ ] User reviewed diff.
- [ ] User approves proceeding to finish-branch.
