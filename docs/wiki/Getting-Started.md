# Getting Started

## Install a continuous prerelease

Every successful push publishes a Windows x64 installer on the repository's Releases page. Choose the newest prerelease for the branch and commit you want to test, verify the SHA-256 value in its release notes, then run the NSIS installer.

The CI smoke gate silently installs the package into an isolated directory, checks the Qt platform plugin, launches the installed executable for ten seconds, and silently uninstalls it before publishing.

## Build and run on Windows

Visual Studio 2022 Build Tools with the C++ workload is the only large prerequisite that must exist first. The helper provisions CMake, Ninja, Python, NSIS, Qt 6.8.3, and repository dependencies as needed.

```powershell
# Configure, build, and launch
powershell -ExecutionPolicy Bypass -File .\run.ps1

# Build only
powershell -ExecutionPolicy Bypass -File .\run.ps1 -NoRun

# Produce the self-contained installer
powershell -ExecutionPolicy Bypass -File .\run.ps1 -Package
```

Installers and their SHA-256 files are written to `build\packages`.

## Linux and macOS

```sh
./run.sh
./run.sh --no-run
./run.sh --clean
```

For the complete dependency matrix and manual CMake commands, open [Building qBittorrent Material](../BUILDING.md).
