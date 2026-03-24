Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ProgramsDir = if ($env:LOCALAPPDATA) { Join-Path $env:LOCALAPPDATA 'Programs' } else { $null }
$InstallPrefix = if ($ProgramsDir) { Join-Path $ProgramsDir 'Muslimtify' } else { $null }
$InstallBinDir = if ($InstallPrefix) { Join-Path $InstallPrefix 'bin' } else { $null }
$MuslimtifyExe = if ($InstallBinDir) { Join-Path $InstallBinDir 'muslimtify.exe' } else { $null }
$ConfigDir = if ($env:APPDATA) { Join-Path $env:APPDATA 'muslimtify' } else { $null }
$CacheDir = if ($env:LOCALAPPDATA) { Join-Path $env:LOCALAPPDATA 'muslimtify' } else { $null }
$TaskName = 'muslimtify'

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
    [string]$Path
  )

  if (-not $Path) {
    Write-Host "Skipped ${Label}: unavailable"
    return
  }

  if (Test-Path -LiteralPath $Path) {
    try {
      Remove-Item -LiteralPath $Path -Recurse -Force -ErrorAction Stop
      Write-Host "Removed $Label"
    } catch {
      Write-Host "Failed to remove ${Label}: $($_.Exception.Message)"
    }
  } else {
    Write-Host "Skipped ${Label}: missing"
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
    } else {
      Write-Host "Scheduled task uninstall failed (exit code $LASTEXITCODE)"
    }
  } catch {
    Write-Host "Scheduled task uninstall failed: $($_.Exception.Message)"
  }
} else {
  Write-Host 'Skipped scheduled task uninstall: binary missing'
}

Remove-PathIfPresent 'install prefix' $InstallPrefix
Remove-PathIfPresent 'config directory' $ConfigDir
Remove-PathIfPresent 'cache directory' $CacheDir

Write-Host ''
Write-Host 'Uninstall complete'
