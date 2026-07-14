param(
    [string]$Executable = (Join-Path $PSScriptRoot '..\build\qbittorrent.exe'),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\docs\images\app'),
    [string]$CaptureRoot = (Join-Path $env:TEMP ('qbittorrent-material-doc-capture-' + [guid]::NewGuid().ToString('N')))
)

$ErrorActionPreference = 'Stop'

$Executable = [IO.Path]::GetFullPath($Executable)
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$CaptureRoot = [IO.Path]::GetFullPath($CaptureRoot)

if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) {
    throw "Build the application before capturing screenshots: $Executable"
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $CaptureRoot | Out-Null

$previousWorkspaceRoot = $env:QBT_WORKSPACE_ROOT

$captures = @(
    @{ File = '01-main-window.png'; Theme = 'light'; Style = '';  Page = 0; Width = 960; Height = 900; Dialog = '' },
    @{ File = '02-toolbar-and-filter.png'; Theme = 'dark';  Style = '';  Page = 0; Width = 960; Height = 900; Dialog = '' },
    @{ File = '03-filter-sidebar.png'; Theme = 'light'; Style = '';  Page = 0; Width = 960; Height = 640; Dialog = '' },
    @{ File = '04-transfer-list.png'; Theme = 'light'; Style = '';  Page = 0; Width = 960; Height = 768; Dialog = '' },
    @{ File = '05-properties-tabs.png'; Theme = 'dark';  Style = '';  Page = 0; Width = 960; Height = 768; Dialog = '' },
    @{ File = '06-statusbar.png'; Theme = 'light'; Style = '';  Page = 3; Width = 960; Height = 900; Dialog = '' },
    @{ File = '07-navigation-and-toolbar.png'; Theme = 'dark';  Style = '';  Page = 1; Width = 960; Height = 900; Dialog = '' },
    @{ File = '08-main-workspace.png'; Theme = 'light'; Style = '';  Page = 2; Width = 960; Height = 900; Dialog = '' },
    @{ File = '09-custom-workspace-tabs.png'; Theme = 'light'; Style = '';  Page = 4; Width = 960; Height = 900; Dialog = '' },
    @{ File = '10-tab-context-menu.png'; Theme = 'light'; Style = '';  Page = 0; Width = 960; Height = 900; Dialog = 'options' },
    @{ File = '11-tab-typography-color.png'; Theme = 'dark';  Style = '';  Page = 0; Width = 960; Height = 900; Dialog = 'options' },
    @{ File = '12-workspace-portability.png'; Theme = 'light'; Style = '';  Page = 0; Width = 960; Height = 768; Dialog = 'add-link' },
    @{ File = '13-restored-workspace.png'; Theme = 'dark';  Style = '';  Page = 0; Width = 960; Height = 768; Dialog = 'about-license' },
    @{ File = '14-split-dock-compact.png'; Theme = 'light'; Style = 'B'; Page = 0; Width = 960; Height = 640; Dialog = '' },
    @{ File = '15-split-dock-dark-compact.png'; Theme = 'dark';  Style = 'B'; Page = 0; Width = 960; Height = 640; Dialog = '' },
    @{ File = '16-card-flow-compact.png'; Theme = 'light'; Style = 'C'; Page = 0; Width = 960; Height = 640; Dialog = '' },
    @{ File = '17-card-flow-dark-compact.png'; Theme = 'dark';  Style = 'C'; Page = 0; Width = 960; Height = 640; Dialog = '' }
)

try {
    foreach ($capture in $captures) {
        $output = Join-Path $OutputDirectory $capture.File
        $captureName = [IO.Path]::GetFileNameWithoutExtension($capture.File)
        $profileRoot = Join-Path $CaptureRoot "$captureName\profile"
        $env:QBT_WORKSPACE_ROOT = Join-Path $CaptureRoot "$captureName\workspace"
        if (Test-Path -LiteralPath $output) {
            Remove-Item -LiteralPath $output -Force
        }

        $arguments = @(
            "--profile-root=$profileRoot"
            "--capture-ui=$output"
            "--capture-theme=$($capture.Theme)"
            "--capture-page=$($capture.Page)"
            "--capture-width=$($capture.Width)"
            "--capture-height=$($capture.Height)"
        )
        if ($capture.Dialog) {
            $arguments += "--capture-dialog=$($capture.Dialog)"
        }
        if ($capture.Style) {
            $arguments += "--capture-style=$($capture.Style)"
        }

        # PowerShell does not reliably wait when invoking a GUI-subsystem
        # executable with the call operator. Start-Process gives every capture
        # time to render, save, and shut down before the file is verified.
        $quotedArguments = $arguments | ForEach-Object { '"' + $_.Replace('"', '\"') + '"' }
        $process = Start-Process -FilePath $Executable `
            -ArgumentList $quotedArguments `
            -WorkingDirectory (Split-Path -Parent $Executable) `
            -Wait -PassThru
        if ($process.ExitCode -ne 0) {
            throw "Capture process failed for $($capture.File) with exit code $($process.ExitCode)"
        }
        if (-not (Test-Path -LiteralPath $output -PathType Leaf)) {
            throw "Capture did not create $output"
        }
        Write-Host "Captured $($capture.File)"
    }
}
finally {
    $env:QBT_WORKSPACE_ROOT = $previousWorkspaceRoot
}
