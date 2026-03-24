Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ProgramsDir = if ($env:LOCALAPPDATA) { Join-Path $env:LOCALAPPDATA 'Programs' } else { $null }
$InstallPrefix = if ($ProgramsDir) { Join-Path $ProgramsDir 'Muslimtify' } else { $null }
$InstallBinDir = if ($InstallPrefix) { Join-Path $InstallPrefix 'bin' } else { $null }
$MuslimtifyExe = if ($InstallBinDir) { Join-Path $InstallBinDir 'muslimtify.exe' } else { $null }
$ConfigDir = if ($env:APPDATA) { Join-Path $env:APPDATA 'muslimtify' } else { $null }
$CacheDir = if ($env:LOCALAPPDATA) { Join-Path $env:LOCALAPPDATA 'muslimtify' } else { $null }
$TaskName = 'muslimtify'
$Summary = New-Object System.Collections.Generic.List[string]

function Write-Target {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Label,
    [string]$Path
  )

  if ($Path) {
    Write-Host "${Label}: $Path"
  } else {
    Write-Host "${Label}: unavailable"
  }
}

function Remove-PathIfPresent {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Label,
    [string]$Path,
    [Parameter(Mandatory = $true)]
    [string]$RemovedMessage,
    [Parameter(Mandatory = $true)]
    [string]$SkippedMessage
  )

  if (-not $Path) {
    Write-Host "Skipped ${Label}: unavailable"
    $Summary.Add("${Label}: skipped ($SkippedMessage)")
    return
  }

  if (Test-Path -LiteralPath $Path) {
    try {
      Remove-Item -LiteralPath $Path -Recurse -Force -ErrorAction Stop
      Write-Host "Removed $Label"
      $Summary.Add("${Label}: removed ($RemovedMessage)")
    } catch {
      Write-Host "Failed to remove ${Label}: $($_.Exception.Message)"
      $Summary.Add("${Label}: skipped (failed to remove)")
    }
  } else {
    Write-Host "Skipped ${Label}: missing"
    $Summary.Add("${Label}: skipped ($SkippedMessage)")
  }
}

function Remove-ScheduledTaskDirect {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Name
  )

  Write-Host 'Removing scheduled task'
  try {
    & schtasks.exe /delete /tn $Name /f *> $null
    if ($LASTEXITCODE -eq 0) {
      Write-Host 'Scheduled task removed'
      $Summary.Add("scheduled task '$Name': removed")
    } else {
      Write-Host "Scheduled task removal failed (exit code $LASTEXITCODE)"
      $Summary.Add("scheduled task '$Name': skipped (removal failed)")
    }
  } catch {
    Write-Host "Scheduled task removal failed: $($_.Exception.Message)"
    $Summary.Add("scheduled task '$Name': skipped (removal failed)")
  }
}

Write-Host 'Muslimtify uninstaller'
Write-Host "Task: $TaskName"
Write-Target 'Install prefix' $InstallPrefix
Write-Target 'Binary' $MuslimtifyExe
Write-Target 'Config directory' $ConfigDir
Write-Target 'Cache directory' $CacheDir
Write-Host ''
Write-Host 'This will remove the scheduled task and delete the install, config, and cache directories.'
Write-Host "Type yes to continue: " -NoNewline

if ([Console]::IsInputRedirected) {
  $Confirmation = [Console]::In.ReadLine()
} else {
  $Confirmation = Read-Host
}

if ($Confirmation -ne 'yes') {
  Write-Host 'Aborted.'
  exit 0
}

Write-Host ''

if ($MuslimtifyExe -and (Test-Path -LiteralPath $MuslimtifyExe)) {
  Write-Host 'Uninstalling scheduled task'
  try {
    & $MuslimtifyExe daemon uninstall *> $null
    if ($LASTEXITCODE -eq 0) {
      Write-Host 'Scheduled task removed'
      $Summary.Add("scheduled task '$TaskName': removed")
    } else {
      Write-Host "Scheduled task uninstall failed (exit code $LASTEXITCODE)"
      $Summary.Add("scheduled task '$TaskName': skipped (uninstall failed)")
    }
  } catch {
    Write-Host "Scheduled task uninstall failed: $($_.Exception.Message)"
    $Summary.Add("scheduled task '$TaskName': skipped (uninstall failed)")
  }
} else {
  Write-Host 'Binary missing; removing scheduled task directly'
  Remove-ScheduledTaskDirect $TaskName
}

Remove-PathIfPresent 'install prefix' $InstallPrefix 'install prefix removed' 'install prefix missing'
Remove-PathIfPresent 'config directory' $ConfigDir 'config directory removed' 'config directory missing'
Remove-PathIfPresent 'cache directory' $CacheDir 'cache directory removed' 'cache directory missing'

Write-Host ''
Write-Host 'Summary:'
foreach ($Item in $Summary) {
  Write-Host "  $Item"
}
Write-Host 'Uninstall complete'
