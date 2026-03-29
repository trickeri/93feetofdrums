# VOID Drum Engine - Build Script
# Run from Developer PowerShell or regular PowerShell with VS2022 installed

param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

$VS_CMAKE = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

if (-not (Test-Path $VS_CMAKE)) {
    Write-Error "CMake not found at expected VS2022 path. Install Visual Studio 2022 with C++ workload."
    exit 1
}

Write-Host "=== VOID DRUM ENGINE BUILD ===" -ForegroundColor Cyan
Write-Host "Config: $Config" -ForegroundColor Yellow

# Create build directory
$buildDir = Join-Path $PSScriptRoot "build"
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# Configure
Write-Host "`n[1/3] Configuring CMake..." -ForegroundColor Green
& $VS_CMAKE -S $PSScriptRoot -B $buildDir -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) { Write-Error "CMake configure failed"; exit 1 }

# Build
Write-Host "`n[2/3] Building..." -ForegroundColor Green
& $VS_CMAKE --build $buildDir --config $Config --parallel
if ($LASTEXITCODE -ne 0) { Write-Error "Build failed"; exit 1 }

# Locate the built VST3
$vst3File = Get-ChildItem -Path $buildDir -Recurse -Filter "VOID Drum Engine.vst3" -Directory | Select-Object -First 1
if ($vst3File) {
    Write-Host "`n[3/3] Build successful!" -ForegroundColor Green
    Write-Host "VST3 bundle: $($vst3File.FullName)" -ForegroundColor Cyan
} else {
    # Try finding the .vst3 as a file instead
    $vst3File = Get-ChildItem -Path $buildDir -Recurse -Filter "*.vst3" | Select-Object -First 1
    if ($vst3File) {
        Write-Host "`n[3/3] Build successful!" -ForegroundColor Green
        Write-Host "VST3 output: $($vst3File.FullName)" -ForegroundColor Cyan
    } else {
        Write-Host "`n[3/3] Build completed. Check build/ directory for output." -ForegroundColor Yellow
    }
}

Write-Host "`nTo install, run: .\install.ps1" -ForegroundColor Yellow
