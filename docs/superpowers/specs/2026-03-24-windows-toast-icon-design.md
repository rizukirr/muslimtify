# Windows Toast Icon Design

## Goal

Add Muslimtify branding to Windows toast notifications and keep exact prayer
notifications aligned with the current Linux default of high urgency.

## Scope

This change applies only to the Windows notification backend in
`src/notification_win.c`.

It does not change:
- Linux notification behavior
- CLI surface
- installer flow
- Task Scheduler behavior
- Windows app packaging or Store-style identity

## Current State

Windows toasts are text-only. The toast XML contains two `<text>` nodes and no
`<image>` node, so notifications do not display a Muslimtify icon in the toast
body.

Exact prayer notifications already map to the Windows toast
`scenario="reminder"` path, which is the closest current equivalent to Linux's
critical urgency behavior. Reminder notifications use the default scenario.

## Requirements

### Icon Behavior

- Windows toasts should include the Muslimtify icon when the icon file can be
  resolved at runtime.
- If the icon file cannot be found, the toast must still be shown without an
  image.
- The icon lookup must work for:
  - installed user-level layouts
  - development runs from the repo/build tree

### Urgency Behavior

- Exact prayer notifications (`minutes_before == 0`) should continue to use the
  highest-urgency Windows toast behavior currently used by the app:
  `scenario="reminder"`.
- Reminder notifications (`minutes_before > 0`) should remain normal toasts by
  default.
- No Linux urgency logic should be changed by this work.

## Runtime Lookup Rules

The Windows backend should resolve the icon in this order:

1. Relative to installed executable layout:
   - `../share/icons/hicolor/128x128/apps/muslimtify.png`
   - `../share/pixmaps/muslimtify.png`
2. Relative to development/build layouts:
   - `../assets/muslimtify.png`
   - `assets/muslimtify.png`
3. If no file is found, omit the image element and keep sending the toast.

The runtime should derive these paths from the current executable location
rather than hardcoding a user profile path.

## Implementation Design

### Notification Backend

Add narrow Windows-local helpers in `src/notification_win.c` to:
- resolve executable-relative icon paths
- check file existence
- XML-escape the image path
- build an optional image payload for the existing toast XML builder

The public notification API in `include/notification.h` should remain unchanged.

### Toast Payload

When an icon is resolved, the toast XML should include an image node inside the
`ToastGeneric` binding. The image is supplemental branding; the notification
must still render if the icon path is unavailable.

This work does not attempt to replace the Windows app identity icon shown by the
shell. It only adds Muslimtify branding to the toast content itself.

## Testing

Add Windows-focused tests for icon path resolution logic. Coverage should
include:
- installed-layout path resolution
- development-layout path resolution
- no-icon fallback behavior

Manual verification should continue to rely on a real Windows toast check after
build/install because unit tests cannot prove shell rendering.

## Risks

- Some Windows shells may render the toast image as body branding rather than as
  the shell-owned app identity icon.
- Because Muslimtify is still an unpackaged app, parts of the notification UI
  may continue to reflect Windows shell identity behavior outside the toast
  body.

## Expected Outcome

- Windows toast notifications show a Muslimtify image when the icon file is
  present.
- Exact prayer notifications keep the current reminder-scenario behavior.
- Missing icon assets do not break notification delivery.
