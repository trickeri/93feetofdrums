# VOID Drum Engine - Build + Package Script
# Builds the VST3, recompiles the Inno Setup installer, and optionally runs it.

param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    [switch]$Install
)

$ErrorActionPreference = "Stop"

# Check for running DAWs
$dawProcesses = Get-Process | Where-Object {
    $_.ProcessName -match 'Ableton|Live|FL Studio|FLEngine|Studio One|Bitwig|Reaper|Cubase|Nuendo|Pro Tools'
} | Select-Object -ExpandProperty ProcessName -Unique
if ($dawProcesses) {
    Write-Host ""
    Write-Host "WARNING: The following DAW(s) are running:" -ForegroundColor Red
    $dawProcesses | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
    Write-Host "Close your DAW before packaging." -ForegroundColor Red
    Write-Host ""
    exit 1
}

Write-Host "=== VOID DRUM ENGINE - BUILD + PACKAGE ===" -ForegroundColor Cyan

# Step 1: Build
Write-Host "`n[1/3] Building VST3..." -ForegroundColor Green
& "$PSScriptRoot\build.ps1" -Config $Config
if ($LASTEXITCODE -ne 0) { Write-Error "Build failed"; exit 1 }

# Step 2: Recompile Inno Setup installer
$iscc = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if (-not (Test-Path $iscc)) {
    Write-Warning "Inno Setup not found at $iscc - skipping installer packaging"
} else {
    Write-Host "`n[2/3] Packaging installer..." -ForegroundColor Green
    & $iscc "$PSScriptRoot\installer\installer.iss" /Q
    if ($LASTEXITCODE -ne 0) { Write-Error "Inno Setup compile failed"; exit 1 }
    Write-Host "Installer: $PSScriptRoot\dist\VOID_Drum_Engine_Setup_v1.0.0.exe" -ForegroundColor Cyan
}

# Step 3: Verify
$srcDll = Join-Path $PSScriptRoot "build\VOIDDrumEngine_artefacts\$Config\VST3\VOID Drum Engine.vst3\Contents\x86_64-win\VOID Drum Engine.vst3"
$srcHash = (Get-FileHash $srcDll -Algorithm MD5).Hash
Write-Host "`n[3/3] Build hash: $srcHash" -ForegroundColor Green

if ($Install) {
    Write-Host "`nInstalling..." -ForegroundColor Yellow
    & "$PSScriptRoot\dist\VOID_Drum_Engine_Setup_v1.0.0.exe" /SILENT
    Write-Host "Installer launched." -ForegroundColor Green
}

Write-Host "`n=== Done ===" -ForegroundColor Cyan
Write-Host "Run the installer: .\dist\VOID_Drum_Engine_Setup_v1.0.0.exe" -ForegroundColor Yellow
