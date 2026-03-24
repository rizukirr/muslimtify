Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Invoke-CMake {
  param(
    [Parameter(Mandatory = $true)]
    [string[]]$Arguments,
    [Parameter(Mandatory = $true)]
    [string]$FailureMessage
  )

  & cmake @Arguments
  if ($LASTEXITCODE -ne 0) {
    throw "$FailureMessage (exit code $LASTEXITCODE)."
  }
}

function Update-UserPath {
  param(
    [Parameter(Mandatory = $true)]
    [string]$PathToAdd
  )

  function Normalize-PathEntry {
    param(
      [Parameter(Mandatory = $true)]
      [string]$Path
    )

    $NormalizedPath = $Path.Trim().Trim('"')
    $PathRoot = [System.IO.Path]::GetPathRoot($NormalizedPath)
    while ($NormalizedPath.Length -gt $PathRoot.Length -and (
      $NormalizedPath.EndsWith([System.IO.Path]::DirectorySeparatorChar.ToString()) -or
      $NormalizedPath.EndsWith([System.IO.Path]::AltDirectorySeparatorChar.ToString())
    )) {
      $NormalizedPath = $NormalizedPath.Substring(0, $NormalizedPath.Length - 1)
    }

    return $NormalizedPath
  }

  $CurrentUserPath = [Environment]::GetEnvironmentVariable('Path', 'User')
  $NormalizedPathToAdd = Normalize-PathEntry -Path $PathToAdd

  if ([string]::IsNullOrWhiteSpace($CurrentUserPath)) {
    try {
      [Environment]::SetEnvironmentVariable('Path', $PathToAdd, 'User')
      return 'added'
    } catch {
      throw "Failed to update the user PATH: $($_.Exception.Message)"
    }
  }

  $PathEntries = $CurrentUserPath.Split(';') | ForEach-Object { $_.Trim() } | Where-Object { $_ }
  foreach ($PathEntry in $PathEntries) {
    if ((Normalize-PathEntry -Path $PathEntry).Equals($NormalizedPathToAdd, [System.StringComparison]::OrdinalIgnoreCase)) {
      return 'present'
    }
  }

  $UpdatedUserPath = $CurrentUserPath.TrimEnd(';')
  if ($UpdatedUserPath.Length -gt 0) {
    $UpdatedUserPath = "$UpdatedUserPath;$PathToAdd"
  } else {
    $UpdatedUserPath = $PathToAdd
  }

  try {
    [Environment]::SetEnvironmentVariable('Path', $UpdatedUserPath, 'User')
    return 'added'
  } catch {
    throw "Failed to update the user PATH: $($_.Exception.Message)"
  }
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
  throw 'CMake was not found in PATH.'
}

$RepoRoot = $PSScriptRoot
$BuildDir = Join-Path $RepoRoot 'build-release'
$InstallPrefix = Join-Path (Join-Path $env:LOCALAPPDATA 'Programs') 'Muslimtify'
$InstallBinDir = Join-Path $InstallPrefix 'bin'
$MuslimtifyExe = Join-Path $InstallBinDir 'muslimtify.exe'
$ServiceExe = Join-Path $InstallBinDir 'muslimtify-service.exe'
$ConfigDir = Join-Path $env:APPDATA 'muslimtify'
$CacheDir = Join-Path $env:LOCALAPPDATA 'muslimtify'

Write-Host 'Muslimtify installer'
Write-Host "Repository root: $RepoRoot"
Write-Host "Build directory: $BuildDir"
Write-Host "Install prefix: $InstallPrefix"
Write-Host "Config directory: $ConfigDir"
Write-Host "Cache directory: $CacheDir"
Write-Host ''
Write-Host 'Configuring Release build'

Invoke-CMake @(
  '-S', $RepoRoot,
  '-B', $BuildDir,
  '-DCMAKE_BUILD_TYPE=Release',
  "-DCMAKE_INSTALL_PREFIX=$InstallPrefix"
) 'CMake configure failed'

Write-Host 'Building Release'
Invoke-CMake @(
  '--build', $BuildDir,
  '--config', 'Release'
) 'CMake build failed'

Write-Host 'Installing'
Invoke-CMake @(
  '--install', $BuildDir,
  '--config', 'Release'
) 'CMake install failed'

if (-not (Test-Path $MuslimtifyExe)) {
  throw "Installed executable not found: $MuslimtifyExe"
}

if (-not (Test-Path $ServiceExe)) {
  throw "Installed helper not found: $ServiceExe"
}

Write-Host 'Registering scheduled task'
& $MuslimtifyExe daemon install
if ($LASTEXITCODE -ne 0) {
  throw "muslimtify.exe daemon install failed (exit code $LASTEXITCODE)."
}

$PathUpdateStatus = 'unknown'
try {
  $PathUpdateStatus = Update-UserPath -PathToAdd $InstallBinDir
} catch {
  $PathUpdateStatus = $_.Exception.Message
}

Write-Host ''
Write-Host 'Installation complete'
Write-Host "Installed binary: $MuslimtifyExe"
Write-Host "Installed helper: $ServiceExe"
Write-Host "Config directory: $ConfigDir"
Write-Host "Cache directory: $CacheDir"
if ($PathUpdateStatus -eq 'added') {
  Write-Host "User PATH updated: added $InstallBinDir"
} elseif ($PathUpdateStatus -eq 'present') {
  Write-Host "User PATH updated: $InstallBinDir was already present"
} else {
  Write-Host "User PATH update failed: $PathUpdateStatus"
}
Write-Host 'Restart PowerShell or open a new terminal before running muslimtify.'
Write-Host ''
Write-Host 'Next commands'
Write-Host "  $MuslimtifyExe help"
Write-Host "  $MuslimtifyExe location auto"
Write-Host "  $MuslimtifyExe daemon status"
