# Windows Toast Test Script Design

## Summary

Add a developer-only PowerShell script, `test_notification.ps1`, to verify Windows toast delivery
independently of the muslimtify application binary.

This script is intentionally **not** a muslimtify CLI feature and does **not** test
`src/notification_win.c` directly. Its purpose is narrower: confirm that Windows toast delivery is
working on the local machine during development.

## Goals

- Provide a simple Windows-native script for manually testing toast delivery.
- Keep the script developer-facing only.
- Avoid adding new user-facing muslimtify commands.
- Mirror the intent of `test_notification.sh`: send a few representative sample notifications and
  print troubleshooting guidance.

## Non-goals

- Testing muslimtify's WinRT notification backend implementation.
- Adding a new CLI command such as `muslimtify test-notification`.
- Adding a new compiled helper binary.
- Changing application behavior or notification APIs.

## Design

### File

- `test_notification.ps1`

### Invocation

Run from PowerShell in the repository root:

```powershell
.\test_notification.ps1
```

### Behavior

The script should:

1. Print a short header describing that it tests Windows toast delivery only.
2. Check that it is running on Windows PowerShell or PowerShell on Windows.
3. Send 2-3 sample toast notifications with short pauses between them.
4. Print success/troubleshooting guidance after dispatching them.

### Sample payloads

Use payloads parallel to the Linux script:

- `Prayer Reminder: Fajr` / `Fajr prayer in 30 minutes`
- `Prayer Reminder: Dhuhr` / `Dhuhr prayer in 15 minutes`
- `Prayer Time: Maghrib` / `It's time for Maghrib prayer`

### Delivery mechanism

Use direct PowerShell/Windows toast delivery rather than muslimtify code paths.

The implementation should stay script-only and avoid external dependencies beyond what is typically
available on Windows development machines. If a Windows Runtime toast path is awkward in pure
PowerShell, the script may use a well-scoped fallback such as `System.Windows.Forms.NotifyIcon`
only if that remains clearly documented as a Windows notification-delivery smoke test.

## UX Notes

- Make the script explicit that it verifies **Windows toast delivery**, not muslimtify backend
  correctness.
- Keep output concise and practical.
- Include troubleshooting notes such as:
  - notifications may be blocked by Focus Assist / Do Not Disturb
  - notifications may be suppressed by OS notification settings
  - running in a non-interactive session can prevent visible toasts

## Documentation

Update `README.md` with a brief developer note that Windows developers can run
`.\test_notification.ps1` to test toast delivery.

Update `CHANGELOG.md` because this is a new developer-facing utility.

## Risks

- A successful script run does not prove muslimtify's own notification backend works.
- PowerShell/Windows toast behavior can vary by host session and OS settings.
- Some toast techniques may require a shell-hosted interactive desktop session.

## Recommendation

Implement `test_notification.ps1` as a developer-only Windows smoke test and document its scope
clearly so it is not mistaken for an end-to-end muslimtify notification test.
