Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ProgramsDir = if ($env:LOCALAPPDATA) { Join-Path $env:LOCALAPPDATA 'Programs' } else { $null }
$InstallPrefix = if ($ProgramsDir) { Join-Path $ProgramsDir 'Muslimtify' } else { $null }
$InstallBinDir = if ($InstallPrefix) { Join-Path $InstallPrefix 'bin' } else { $null }
$MuslimtifyExe = if ($InstallBinDir) { Join-Path $InstallBinDir 'muslimtify.exe' } else { $null }
$ConfigDir = if ($env:APPDATA) { Join-Path $env:APPDATA 'muslimtify' } else { $null }
$CacheDir = if ($env:LOCALAPPDATA) { Join-Path $env:LOCALAPPDATA 'muslimtify' } else { $null }
$TrustedAppData = [Environment]::GetFolderPath('ApplicationData')
$TrustedLocalAppData = [Environment]::GetFolderPath('LocalApplicationData')
$ExpectedInstallPrefix = if ($TrustedLocalAppData) { Join-Path (Join-Path $TrustedLocalAppData 'Programs') 'Muslimtify' } else { $null }
$ExpectedInstallBinDir = if ($ExpectedInstallPrefix) { Join-Path $ExpectedInstallPrefix 'bin' } else { '%LOCALAPPDATA%\Programs\Muslimtify\bin' }
$ExpectedConfigDir = if ($TrustedAppData) { Join-Path $TrustedAppData 'muslimtify' } else { $null }
$ExpectedCacheDir = if ($TrustedLocalAppData) { Join-Path $TrustedLocalAppData 'muslimtify' } else { $null }
$TaskName = 'muslimtify'
$Summary = New-Object System.Collections.Generic.List[string]
$HadErrors = $false

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

function Normalize-Path {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Path
  )

  return [System.IO.Path]::GetFullPath($Path).TrimEnd('\')
}

function Get-ComparablePath {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Path
  )

  $ExpandedPath = [Environment]::ExpandEnvironmentVariables($Path).Trim().Trim('"')
  if (-not $ExpandedPath) {
    return $null
  }

  try {
    return (Resolve-Path -LiteralPath $ExpandedPath -ErrorAction Stop).Path.TrimEnd('\')
  } catch {
    return [System.IO.Path]::GetFullPath($ExpandedPath).TrimEnd('\')
  }
}

function Test-ExpectedTarget {
  param(
    [Parameter(Mandatory = $true)]
    [string]$ActualPath,
    [Parameter(Mandatory = $true)]
    [string]$ExpectedPath
  )

  if (-not $ActualPath -or -not $ExpectedPath) {
    return $false
  }

  return (Normalize-Path $ActualPath) -ieq (Normalize-Path $ExpectedPath)
}

function Remove-MuslimtifyFromUserPath {
  param(
    [Parameter(Mandatory = $true)]
    [string]$TargetPath,
    [Parameter(Mandatory = $true)]
    [string]$ManualCleanupPath
  )

  if (-not $TargetPath) {
    Write-Host "PATH cleanup failed; remove this path manually from your user PATH: $ManualCleanupPath"
    $Summary.Add("user PATH: manual cleanup required ($ManualCleanupPath)")
    return
  }

  $UserEnvironmentKey = $null
  try {
    $UserEnvironmentKey = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey('Environment', $true)
    if (-not $UserEnvironmentKey) {
      throw 'User environment registry key is unavailable.'
    }

    $CurrentPath = $UserEnvironmentKey.GetValue('Path', $null, [Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames)
    if ($null -eq $CurrentPath -or $CurrentPath -eq '') {
      Write-Host 'PATH entry already absent'
      $Summary.Add("user PATH: already absent ($ManualCleanupPath)")
      return
    }

    $TargetComparablePath = Get-ComparablePath $TargetPath
    $Entries = @($CurrentPath.Split(';'))
    $KeptEntries = New-Object System.Collections.Generic.List[string]
    $RemovedCount = 0

    foreach ($Entry in $Entries) {
      if ($Entry -eq '') {
        $KeptEntries.Add($Entry)
        continue
      }

      $EntryComparablePath = Get-ComparablePath $Entry
      if ($EntryComparablePath -and ($EntryComparablePath -ieq $TargetComparablePath)) {
        $RemovedCount++
        continue
      }

      $KeptEntries.Add($Entry)
    }

    if ($RemovedCount -eq 0) {
      Write-Host 'PATH entry already absent'
      $Summary.Add("user PATH: already absent ($ManualCleanupPath)")
      return
    }

    $NewPath = [string]::Join(';', $KeptEntries)
    if ($NewPath) {
      $ValueKind = [Microsoft.Win32.RegistryValueKind]::String
      try {
        $ValueKind = $UserEnvironmentKey.GetValueKind('Path')
      } catch {
      }

      $UserEnvironmentKey.SetValue('Path', $NewPath, $ValueKind)
    } else {
      $UserEnvironmentKey.DeleteValue('Path', $false)
    }

    Write-Host 'PATH entry removed'
    if ($RemovedCount -eq 1) {
      $Summary.Add("user PATH: removed ($ManualCleanupPath)")
    } else {
      $Summary.Add("user PATH: removed $RemovedCount entries ($ManualCleanupPath)")
    }
  } catch {
    Write-Host "PATH cleanup failed; remove this path manually from your user PATH: $ManualCleanupPath"
    $Summary.Add("user PATH: manual cleanup required ($ManualCleanupPath)")
  } finally {
    if ($UserEnvironmentKey) {
      $UserEnvironmentKey.Close()
    }
  }
}

function Mark-Failure {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Message
  )

  $script:HadErrors = $true
  Write-Host $Message
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
      Mark-Failure "Failed to remove ${Label}: $($_.Exception.Message)"
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
    $TaskOutput = & schtasks.exe /delete /tn $Name /f 2>&1
    if ($LASTEXITCODE -eq 0) {
      Write-Host 'Scheduled task removed'
      $Summary.Add("scheduled task '$Name': removed")
      return $true
    } elseif ($LASTEXITCODE -eq 1) {
      $TaskOutputText = ($TaskOutput | Out-String).Trim()
      if ($TaskOutputText -match 'cannot find the file specified|cannot find the task specified|not found') {
        Write-Host 'Scheduled task already missing'
        $Summary.Add("scheduled task '$Name': skipped (already missing)")
        return $true
      }
    }

    Mark-Failure "Scheduled task removal failed (exit code $LASTEXITCODE)"
    $Summary.Add("scheduled task '$Name': skipped (removal failed)")
    return $false
  } catch {
    Mark-Failure "Scheduled task removal failed: $($_.Exception.Message)"
    $Summary.Add("scheduled task '$Name': skipped (removal failed)")
    return $false
  }
}

function Remove-ScheduledTask {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Name,
    [string]$MuslimtifyExe
  )

  if ($MuslimtifyExe -and (Test-Path -LiteralPath $MuslimtifyExe)) {
    Write-Host 'Uninstalling scheduled task'
    try {
      & $MuslimtifyExe daemon uninstall *> $null
      if ($LASTEXITCODE -eq 0) {
        Write-Host 'Scheduled task removed'
        $Summary.Add("scheduled task '$Name': removed")
        return
      }

      Mark-Failure "Scheduled task uninstall failed (exit code $LASTEXITCODE)"
      $Summary.Add("scheduled task '$Name': skipped (uninstall failed)")
    } catch {
      Mark-Failure "Scheduled task uninstall failed: $($_.Exception.Message)"
      $Summary.Add("scheduled task '$Name': skipped (uninstall failed)")
    }
  } else {
    Write-Host 'Binary missing; removing scheduled task directly'
  }

  Remove-ScheduledTaskDirect $Name
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

$TargetChecks = @(
  @{ Label = 'install prefix'; Actual = $InstallPrefix; Expected = $ExpectedInstallPrefix },
  @{ Label = 'config directory'; Actual = $ConfigDir; Expected = $ExpectedConfigDir },
  @{ Label = 'cache directory'; Actual = $CacheDir; Expected = $ExpectedCacheDir }
)

foreach ($Target in $TargetChecks) {
  if (-not (Test-ExpectedTarget $Target.Actual $Target.Expected)) {
    Mark-Failure "Unsafe $($Target.Label) target: $($Target.Actual)"
    $Summary.Add("$($Target.Label): skipped (unsafe target)")
  }
}

if ($HadErrors) {
  Write-Host ''
  Write-Host 'Aborting before deletion because one or more targets were not trusted.'
  Write-Host ''
  Write-Host 'Summary:'
  foreach ($Item in $Summary) {
    Write-Host "  $Item"
  }
  exit 1
}

Write-Target 'PATH target' $ExpectedInstallBinDir
Remove-ScheduledTask $TaskName $MuslimtifyExe
Remove-MuslimtifyFromUserPath $ExpectedInstallBinDir $ExpectedInstallBinDir
Remove-PathIfPresent 'install prefix' $InstallPrefix 'install prefix removed' 'install prefix missing'
Remove-PathIfPresent 'config directory' $ConfigDir 'config directory removed' 'config directory missing'
Remove-PathIfPresent 'cache directory' $CacheDir 'cache directory removed' 'cache directory missing'

Write-Host ''
Write-Host 'Summary:'
foreach ($Item in $Summary) {
  Write-Host "  $Item"
}
if ($HadErrors) {
  Write-Host 'Uninstall completed with errors'
  exit 1
}

Write-Host 'Uninstall complete'
