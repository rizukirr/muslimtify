Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Write-Section {
    param([string]$Text)
    Write-Host ""
    Write-Host $Text
}

function Escape-XmlText {
    param([string]$Text)

    return [System.Security.SecurityElement]::Escape($Text)
}

function Send-Toast {
    param(
        [string]$AppId,
        [string]$Title,
        [string]$Message
    )

    $xml = @"
<toast duration="short">
  <visual>
    <binding template="ToastGeneric">
      <text>$(Escape-XmlText $Title)</text>
      <text>$(Escape-XmlText $Message)</text>
    </binding>
  </visual>
</toast>
"@

    $document = New-Object Windows.Data.Xml.Dom.XmlDocument
    $document.LoadXml($xml)

    $toast = [Windows.UI.Notifications.ToastNotification]::new($document)
    $notifier = [Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier($AppId)
    $notifier.Show($toast)
}

Write-Host 'Muslimtify Windows toast delivery smoke test'
Write-Host 'This tests Windows toast delivery only, not muslimtify backend correctness.'

if ($env:OS -ne 'Windows_NT') {
    throw 'This script must be run on Windows.'
}

try {
    [Windows.UI.Notifications.ToastNotificationManager, Windows.UI.Notifications, ContentType = WindowsRuntime] | Out-Null
    [Windows.Data.Xml.Dom.XmlDocument, Windows.Data.Xml.Dom.XmlDocument, ContentType = WindowsRuntime] | Out-Null
} catch {
    throw 'This Windows host does not expose the toast notification runtime types required by this script.'
}

$appId = '{1AC14E77-02E7-4E5D-B744-2EB1AE5198B7}\WindowsPowerShell\v1.0\powershell.exe'
$samples = @(
    @{
        Title = 'Prayer Reminder: Fajr'
        Message = "Fajr prayer in 30 minutes`nTime: 04:42"
    },
    @{
        Title = 'Prayer Reminder: Dhuhr'
        Message = "Dhuhr prayer in 15 minutes`nTime: 12:09"
    },
    @{
        Title = 'Prayer Time: Maghrib'
        Message = "It's time for Maghrib prayer`nTime: 18:16"
    }
)

$failures = @()

Write-Section 'Sending sample prayer-style notifications...'

for ($index = 0; $index -lt $samples.Count; $index++) {
    $sample = $samples[$index]
    Write-Host ("[{0}/{1}] {2}" -f ($index + 1), $samples.Count, $sample.Title)

    try {
        Send-Toast -AppId $appId -Title $sample.Title -Message $sample.Message
        Write-Host '  Toast dispatched.'
    } catch {
        $failures += $_.Exception.Message
        Write-Warning ("  Toast dispatch failed: {0}" -f $_.Exception.Message)
    }

    if ($index -lt ($samples.Count - 1)) {
        Start-Sleep -Seconds 2
    }
}

Write-Section 'Troubleshooting notes:'
Write-Host '- Confirm notifications are enabled for PowerShell in Windows Settings.'
Write-Host '- Check Focus Assist / Do Not Disturb if nothing appears.'
Write-Host '- Run the script from an interactive desktop session, not over a non-interactive remoting session.'
Write-Host '- This only verifies toast delivery from Windows PowerShell; it does not exercise muslimtify''s backend logic.'

if ($failures.Count -gt 0) {
    Write-Section 'One or more toast dispatches failed in this environment.'
    Write-Host 'That usually means the current Windows session does not permit desktop toast delivery from this host.'
}
