<#
  qBittorrent Material — one-click build & run (Windows)

  Fully automatic: installs every dependency into the repo (nothing global),
  configures, builds, and launches the app. Safe to re-run — each step is
  skipped if already done.

  Usage:
      powershell -ExecutionPolicy Bypass -File run.ps1        # build + run
      ...-File run.ps1 -NoRun                                  # build only
      ...-File run.ps1 -Clean                                  # wipe build/ first
      ...-File run.ps1 -Jobs 8                                 # parallel build jobs

  Requirements it will use if present, else install locally:
    - Visual Studio 2022 Build Tools (MSVC, C++). MUST be installed by you
      (Microsoft does not allow silent redistribution). If missing, the script
      prints the one-line winget command to get it.
    - CMake >= 3.21, Ninja  -> installed via winget if missing
    - Python 3              -> used to fetch Qt (aqtinstall)
    - Qt 6.8.3 (msvc2022_64) -> fetched into .\.qt via aqtinstall
    - vcpkg + libtorrent/zlib -> cloned into .\.vcpkg and built
#>
[CmdletBinding()]
param(
    [switch]$NoRun,
    [switch]$Clean,
    [int]$Jobs = 0
)

$ErrorActionPreference = 'Stop'
$Repo = $PSScriptRoot
$QtVersion = '6.8.3'
$QtArch = 'win64_msvc2022_64'
$QtDirName = 'msvc2022_64'
$Triplet = 'x64-windows'
$QtRoot = Join-Path $Repo '.qt'
$QtPrefix = Join-Path $QtRoot "$QtVersion\$QtDirName"
$VcpkgRoot = Join-Path $Repo '.vcpkg'
$BuildDir = Join-Path $Repo 'build'

function Info($m) { Write-Host "==> $m" -ForegroundColor Cyan }
function Warn($m) { Write-Host "!!  $m" -ForegroundColor Yellow }
function Die($m)  { Write-Host "ERROR: $m" -ForegroundColor Red; exit 1 }

function Have($cmd) { return [bool](Get-Command $cmd -ErrorAction SilentlyContinue) }

# --- 0. winget helper for cmake/ninja -----------------------------------------
function Ensure-Tool($cmd, $wingetId, $friendly) {
    if (Have $cmd) { return }
    if (Have 'winget') {
        Info "Installing $friendly via winget..."
        winget install --id $wingetId --accept-source-agreements --accept-package-agreements -e --silent | Out-Host
    }
    if (-not (Have $cmd)) {
        Die "$friendly ($cmd) not found and could not be auto-installed. Install it and re-run."
    }
}

# --- 1. Locate MSVC (vcvars64) -------------------------------------------------
function Find-VcVars {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $path = & $vswhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath 2>$null | Select-Object -First 1
        if ($path) {
            $vc = Join-Path $path 'VC\Auxiliary\Build\vcvars64.bat'
            if (Test-Path $vc) { return $vc }
        }
    }
    return $null
}

function Import-VcVars($vcvars) {
    Info "Importing MSVC environment ($vcvars)"
    cmd /c "`"$vcvars`" && set" | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') { Set-Item -Path "env:$($matches[1])" -Value $matches[2] }
    }
}

# --- 2. Qt via aqtinstall ------------------------------------------------------
function Ensure-Qt {
    if (Test-Path (Join-Path $QtPrefix 'bin\qmake.exe')) {
        Info "Qt $QtVersion already present at $QtPrefix"
        return
    }
    $py = if (Have 'python') { 'python' } elseif (Have 'py') { 'py' } else { $null }
    if (-not $py) { Ensure-Tool 'python' 'Python.Python.3.12' 'Python 3'; $py = 'python' }
    Info "Installing aqtinstall (Qt downloader)..."
    & $py -m pip install --quiet --upgrade aqtinstall
    Info "Downloading Qt $QtVersion $QtArch into $QtRoot (a few minutes)..."
    & $py -m aqt install-qt windows desktop $QtVersion $QtArch `
        -m qt5compat qtimageformats qtshadertools --outputdir $QtRoot
    if (-not (Test-Path (Join-Path $QtPrefix 'bin\qmake.exe'))) { Die "Qt install failed." }
}

# --- 3. vcpkg + libtorrent -----------------------------------------------------
function Ensure-Vcpkg {
    if (-not (Test-Path (Join-Path $VcpkgRoot 'vcpkg.exe'))) {
        if (-not (Test-Path $VcpkgRoot)) {
            Info "Cloning vcpkg into $VcpkgRoot ..."
            git clone --depth 1 https://github.com/microsoft/vcpkg.git $VcpkgRoot | Out-Host
        }
        Info "Bootstrapping vcpkg..."
        & (Join-Path $VcpkgRoot 'bootstrap-vcpkg.bat') -disableMetrics | Out-Host
    }
    $vcpkg = Join-Path $VcpkgRoot 'vcpkg.exe'
    $installed = Join-Path $VcpkgRoot "installed\$Triplet\lib\torrent-rasterbar.lib"
    if (Test-Path $installed) {
        Info "libtorrent already built in vcpkg."
    } else {
        Warn "Building libtorrent + Boost + OpenSSL + zlib from source."
        Warn "FIRST run only; this can take 30-90 minutes. Subsequent runs are instant."
        & $vcpkg install "libtorrent:$Triplet" "zlib:$Triplet" --clean-after-build | Out-Host
        if (-not (Test-Path $installed)) { Die "vcpkg failed to build libtorrent." }
    }
}

# --- MAIN ----------------------------------------------------------------------
Info "qBittorrent Material — automatic build & run"
Info "Repo: $Repo"

if ($Clean -and (Test-Path $BuildDir)) { Info "Cleaning build/"; Remove-Item -Recurse -Force $BuildDir }

$vcvars = Find-VcVars
if (-not $vcvars) {
    Die @"
Visual Studio 2022 Build Tools (MSVC C++) not found.
Install it (one-time) with:
    winget install --id Microsoft.VisualStudio.2022.BuildTools -e --override "--quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
then re-run this script.
"@
}
Import-VcVars $vcvars

Ensure-Tool 'cmake' 'Kitware.CMake' 'CMake'
Ensure-Tool 'ninja' 'Ninja-build.Ninja' 'Ninja'
Ensure-Qt
Ensure-Vcpkg

if ($Jobs -le 0) { $Jobs = [Environment]::ProcessorCount }

Info "Configuring (CMake + Ninja)..."
cmake -B $BuildDir -S $Repo -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot/scripts/buildsystems/vcpkg.cmake" `
    -DVCPKG_TARGET_TRIPLET=$Triplet `
    -DCMAKE_PREFIX_PATH="$QtPrefix"
if ($LASTEXITCODE -ne 0) { Die "CMake configure failed." }

Info "Building (-j $Jobs)..."
cmake --build $BuildDir --parallel $Jobs
if ($LASTEXITCODE -ne 0) { Die "Build failed." }

$exe = Get-ChildItem -Path $BuildDir -Recurse -Filter 'qbittorrent.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $exe) { Die "Build succeeded but qbittorrent.exe not found." }
Info "Build OK: $($exe.FullName)"

# Ensure Qt runtime DLLs are next to the exe for a portable run.
$windeployqt = Join-Path $QtPrefix 'bin\windeployqt.exe'
if (Test-Path $windeployqt) {
    Info "Deploying Qt runtime (windeployqt)..."
    & $windeployqt --qmldir "$Repo\src\quick\qml" --release "$($exe.FullName)" | Out-Null
}

if ($NoRun) {
    Info "Done (build only). Run it with: `"$($exe.FullName)`""
} else {
    Info "Launching qBittorrent Material..."
    & $exe.FullName
}
