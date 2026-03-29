# VOID Drum Engine - Install Script
# Copies built VST3 to system VST3 folder (requires admin for Program Files)

param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    [switch]$UserOnly
)

$ErrorActionPreference = "Stop"

$pluginName = "VOID Drum Engine"
$buildDir = Join-Path $PSScriptRoot "build"

# System VST3 folder (standard location all DAWs scan)
$systemVst3 = "C:\Program Files\Common Files\VST3"
# User VST3 folder (no admin required)
$userVst3 = Join-Path $env:LOCALAPPDATA "Programs\Common Files\VST3"

# Find the built VST3 bundle
$vst3Bundle = Get-ChildItem -Path $buildDir -Recurse -Filter "$pluginName.vst3" -Directory | Select-Object -First 1

if (-not $vst3Bundle) {
    Write-Error "VST3 not found in build/. Run .\build.ps1 first."
    exit 1
}

Write-Host "=== VOID DRUM ENGINE INSTALLER ===" -ForegroundColor Cyan
Write-Host "Source: $($vst3Bundle.FullName)" -ForegroundColor Gray

if ($UserOnly) {
    $destDir = $userVst3
    Write-Host "Installing to user directory (no admin required)..." -ForegroundColor Yellow
} else {
    $destDir = $systemVst3
    Write-Host "Installing to system directory (may require admin)..." -ForegroundColor Yellow
}

# Create destination if needed
if (-not (Test-Path $destDir)) {
    New-Item -ItemType Directory -Path $destDir -Force | Out-Null
}

$destPath = Join-Path $destDir "$pluginName.vst3"

# Remove old version if present
if (Test-Path $destPath) {
    Write-Host "Removing previous installation..." -ForegroundColor Yellow
    Remove-Item -Path $destPath -Recurse -Force
}

# Copy VST3 bundle
Write-Host "Copying VST3 bundle..." -ForegroundColor Green
Copy-Item -Path $vst3Bundle.FullName -Destination $destPath -Recurse -Force

# Create sample folders in plugin data directory
$pluginData = Join-Path $env:APPDATA "VOID Drum Engine"
$sampleDir = Join-Path $pluginData "VOID_Samples"
$presetDir = Join-Path $pluginData "VOID_Presets"
$factoryPresetDir = Join-Path $presetDir "Factory"
$userPresetDir = Join-Path $presetDir "User"

$folders = @(
    (Join-Path $sampleDir "kicks"),
    (Join-Path $sampleDir "snares"),
    (Join-Path $sampleDir "hats"),
    (Join-Path $sampleDir "percs"),
    (Join-Path $sampleDir "toms"),
    (Join-Path $sampleDir "fx"),
    (Join-Path $sampleDir "user"),
    $factoryPresetDir,
    $userPresetDir
)

foreach ($folder in $folders) {
    if (-not (Test-Path $folder)) {
        New-Item -ItemType Directory -Path $folder -Force | Out-Null
    }
}

Write-Host ""
Write-Host "=== Installation Complete ===" -ForegroundColor Green
Write-Host ""
Write-Host "VST3 installed to:  $destPath" -ForegroundColor Cyan
Write-Host "Sample folder:      $sampleDir" -ForegroundColor Cyan
Write-Host "Presets folder:     $presetDir" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Drop your .wav/.flac/.ogg/.aiff samples into the sample subfolders"
Write-Host "  2. Open your DAW and scan for new plugins"
Write-Host "  3. Load 'VOID Drum Engine' on a track"
Write-Host ""
