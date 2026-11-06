# Lanterne — Build script (PowerShell)
# Run from project root: .\scripts\build.ps1 [command]
# Commands: build (default), clean, reconfigure, rebuild, open, help

param(
    [Parameter(Position = 0)]
    [ValidateSet("build", "clean", "reconfigure", "rebuild", "open", "help", "")]
    [string]$Command = "build",
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $ProjectRoot "build"
$Generator = "Visual Studio 17 2022"
$Arch = "x64"
$Target = "Lanterne_VST3"
$Vst3Path = Join-Path $BuildDir "Lanterne_artefacts\$Config\VST3\Lanterne.vst3"

function Write-Step { param($Msg) Write-Host "`n>> $Msg" -ForegroundColor Cyan }
function Write-Ok { param($Msg) Write-Host "OK: $Msg" -ForegroundColor Green }
function Write-Warn { param($Msg) Write-Host "WARN: $Msg" -ForegroundColor Yellow }

function Invoke-Clean {
    Write-Step "Cleaning build directory"
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
        Write-Ok "Removed $BuildDir"
    } else {
        Write-Ok "Already clean"
    }
}

function Invoke-Configure {
    Write-Step "Configuring CMake ($Generator -A $Arch)"
    Set-Location $ProjectRoot
    $juceArgs = @()
    if ($env:LANTERNE_JUCE_DIR) { $juceArgs += "-DLANTERNE_JUCE_DIR=$($env:LANTERNE_JUCE_DIR)" }
    & cmake -B build -S . -G $Generator -A $Arch @juceArgs
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
    Write-Ok "Configure done"
}

function Invoke-Build {
    Write-Step "Building $Target ($Config)"
    Set-Location $ProjectRoot
    & cmake --build build --config $Config --target $Target
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }
    Write-Ok "Build done"
    if (Test-Path $Vst3Path) {
        Write-Host "  VST3: $Vst3Path" -ForegroundColor Gray
    }
}

function Invoke-Rebuild {
    Invoke-Clean
    Invoke-Configure
    Invoke-Build
}

function Invoke-Open {
    $Sln = Join-Path $BuildDir "Lanterne.sln"
    if (-not (Test-Path $Sln)) {
        Write-Warn "No solution found. Run reconfigure or rebuild first."
        return
    }
    Start-Process $Sln
    Write-Ok "Opened $Sln"
}

function Show-Help {
    @"

Lanterne build script
======================
Usage:  .\scripts\build.ps1 [command] [-Config Release|Debug]

Commands:
  build        Build the VST3 (default). Uses existing build dir.
  clean        Delete the 'build' folder.
  reconfigure  Run CMake configure (creates/updates build files).
  rebuild      clean + reconfigure + build (full clean rebuild).
  open         Open Lanterne.sln in the default IDE.
  help         Show this message.

Examples:
  .\scripts\build.ps1              # build Release
  .\scripts\build.ps1 rebuild      # clean, configure, build
  .\scripts\build.ps1 build -Config Debug
  .\scripts\build.ps1 clean; .\scripts\build.ps1 reconfigure

JUCE: expected at .cache\JUCE. Set the environment variable LANTERNE_JUCE_DIR
to point at another checkout; the script forwards it to CMake.

WebView2 (Windows): If the plugin shows "getContext not available", install
the WebView2 NuGet package so CMake can find it, then rebuild:
  Register-PackageSource -Provider NuGet -Name nugetRepository -Location https://www.nuget.org/api/v2
  Install-Package Microsoft.Web.WebView2 -Scope CurrentUser -RequiredVersion 1.0.3485.44 -Source nugetRepository
Or set JUCE_WEBVIEW2_PACKAGE_LOCATION to the package directory and reconfigure.

"@
}

# Main
Set-Location $ProjectRoot
switch ($Command) {
    "build"       { Invoke-Build }
    "clean"       { Invoke-Clean }
    "reconfigure" { Invoke-Configure }
    "rebuild"     { Invoke-Rebuild }
    "open"        { Invoke-Open }
    "help"        { Show-Help }
    ""            { Invoke-Build }
}
Write-Host ""
