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
    if ([string]::IsNullOrWhiteSpace($NormalizedPath)) {
      return $null
    }

    $NormalizedPath = [Environment]::ExpandEnvironmentVariables($NormalizedPath)
    $NormalizedPath = $NormalizedPath.Trim().Trim('"').Replace('/', '\')

    if ([System.IO.Path]::IsPathRooted($NormalizedPath)) {
      try {
        $NormalizedPath = [System.IO.Path]::GetFullPath($NormalizedPath)
      } catch {
      }
    }

    $PathRoot = [System.IO.Path]::GetPathRoot($NormalizedPath)
    while ($PathRoot -and $NormalizedPath.Length -gt $PathRoot.Length -and (
      $NormalizedPath.EndsWith([System.IO.Path]::DirectorySeparatorChar.ToString()) -or
      $NormalizedPath.EndsWith([System.IO.Path]::AltDirectorySeparatorChar.ToString())
    )) {
      $NormalizedPath = $NormalizedPath.Substring(0, $NormalizedPath.Length - 1)
    }

    return $NormalizedPath
  }

  $NormalizedPathToAdd = Normalize-PathEntry -Path $PathToAdd
  if ($null -eq $NormalizedPathToAdd) {
    throw 'The PATH entry to add was empty after normalization.'
  }

  function Test-PathEntryPresent {
    param(
      [string]$CurrentUserPath
    )

    if ([string]::IsNullOrWhiteSpace($CurrentUserPath)) {
      return $false
    }

    $PathEntries = $CurrentUserPath.Split(';') | ForEach-Object { $_.Trim() } | Where-Object { $_ }
    foreach ($PathEntry in $PathEntries) {
      $NormalizedEntry = Normalize-PathEntry -Path $PathEntry
      if ($null -ne $NormalizedEntry -and $NormalizedEntry.Equals($NormalizedPathToAdd, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $true
      }
    }

    return $false
  }

  $CurrentUserPath = [Environment]::GetEnvironmentVariable('Path', 'User')
  if (Test-PathEntryPresent -CurrentUserPath $CurrentUserPath) {
    return 'present'
  }

  $LatestUserPath = [Environment]::GetEnvironmentVariable('Path', 'User')
  if (Test-PathEntryPresent -CurrentUserPath $LatestUserPath) {
    return 'present'
  }

  if ([string]::IsNullOrWhiteSpace($LatestUserPath)) {
    $UpdatedUserPath = $PathToAdd
  } else {
    $UpdatedUserPath = $LatestUserPath.TrimEnd(';', ' ')
    if ($UpdatedUserPath.Length -gt 0) {
      $UpdatedUserPath = "$UpdatedUserPath;$PathToAdd"
    } else {
      $UpdatedUserPath = $PathToAdd
    }
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

Write-Host 'Creating Start Menu shortcut for toast notifications'
$StartMenuDir = Join-Path ([Environment]::GetFolderPath('StartMenu')) 'Programs'
$ShortcutPath = Join-Path $StartMenuDir 'Muslimtify.lnk'

Add-Type -TypeDefinition @'
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

[ComImport]
[Guid("886D8EEB-8CF2-4446-8D02-CDBA1DBDCF99")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
interface IPropertyStore {
    int GetCount(out uint cProps);
    int GetAt(uint iProp, out PropertyKey pkey);
    int GetValue(ref PropertyKey key, out PropVariant pv);
    int SetValue(ref PropertyKey key, ref PropVariant pv);
    int Commit();
}

[StructLayout(LayoutKind.Sequential, Pack = 4)]
struct PropertyKey {
    public Guid fmtid;
    public uint pid;
    public PropertyKey(Guid fmtid, uint pid) { this.fmtid = fmtid; this.pid = pid; }
}

[StructLayout(LayoutKind.Explicit)]
struct PropVariant {
    [FieldOffset(0)] public ushort vt;
    [FieldOffset(8)] public IntPtr pszVal;

    public static PropVariant FromString(string val) {
        var pv = new PropVariant();
        pv.vt = 31; // VT_LPWSTR
        pv.pszVal = Marshal.StringToCoTaskMemUni(val);
        return pv;
    }
}

public static class ShortcutHelper {
    [DllImport("shell32.dll", PreserveSig = false)]
    private static extern void SHGetPropertyStoreFromParsingName(
        [MarshalAs(UnmanagedType.LPWStr)] string pszPath,
        IntPtr pbc, int flags,
        [MarshalAs(UnmanagedType.LPStruct)] Guid riid,
        [MarshalAs(UnmanagedType.Interface)] out IPropertyStore ppv);

    public static void SetAppUserModelId(string shortcutPath, string aumid) {
        // PKEY_AppUserModel_ID = {9F4C2855-9F79-4B39-A8D0-E1D42DE1D5F3}, 5
        var key = new PropertyKey(
            new Guid("9F4C2855-9F79-4B39-A8D0-E1D42DE1D5F3"), 5);
        var iid = new Guid("886D8EEB-8CF2-4446-8D02-CDBA1DBDCF99");
        IPropertyStore store;
        // GPS_READWRITE = 2
        SHGetPropertyStoreFromParsingName(shortcutPath, IntPtr.Zero, 2, iid, out store);
        var pv = PropVariant.FromString(aumid);
        store.SetValue(ref key, ref pv);
        store.Commit();
        Marshal.FinalReleaseComObject(store);
    }
}
'@

$WshShell = New-Object -ComObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut($ShortcutPath)
$Shortcut.TargetPath = $MuslimtifyExe
$Shortcut.Description = 'Prayer Time Notification Daemon'
$IcoPath = Join-Path $InstallPrefix 'share\icons\muslimtify.ico'
if (Test-Path $IcoPath) {
  $Shortcut.IconLocation = "$IcoPath,0"
}
$Shortcut.Save()
[System.Runtime.InteropServices.Marshal]::FinalReleaseComObject($Shortcut) | Out-Null
[System.Runtime.InteropServices.Marshal]::FinalReleaseComObject($WshShell) | Out-Null

[ShortcutHelper]::SetAppUserModelId($ShortcutPath, 'Muslimtify')
Write-Host "Start Menu shortcut created: $ShortcutPath"

$PathUpdateStatus = 'unknown'
$PathUpdateFailed = $false
try {
  $PathUpdateStatus = Update-UserPath -PathToAdd $InstallBinDir
} catch {
  $PathUpdateStatus = $_.Exception.Message
  $PathUpdateFailed = $true
}

Write-Host ''
if ($PathUpdateFailed) {
  Write-Host 'Installation completed with PATH update failure'
} else {
  Write-Host 'Installation complete'
}
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

if ($PathUpdateFailed) {
  exit 1
}
