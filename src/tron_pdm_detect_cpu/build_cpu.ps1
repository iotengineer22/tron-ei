# C:\Users\ioten\e2_studio\workspace\tron_pdm_detect_cpu\build_cpu.ps1
# C++ / C compiler build script for EK-RA8P1

Write-Host "Starting EK-RA8P1 CPU Project Build..." -ForegroundColor Cyan

# 1. Add Renesas e2 studio toolchain and GNU make paths to environment PATH
$toolchainPath = "C:\Renesas\RA\e2studio_v2026-04.2_fsp_v6.5.0\toolchains\gcc_arm\13.2.rel1\bin"
$makePath = "C:\Renesas\RA\e2studio_v2026-04.2_fsp_v6.5.0\eclipse\plugins\com.renesas.ide.exttools.gnumake.win32.x86_64_4.3.1.v20240909-0854\mk"

$env:PATH = "$toolchainPath;$makePath;" + $env:PATH

# 2. Check current directory
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$debugDir = Join-Path $scriptDir "Debug"

if (-not (Test-Path $debugDir)) {
    Write-Error "Debug directory not found! Make sure you run this script from the root of the project."
    exit 1
}

# 3. Run build inside Debug directory
Push-Location $debugDir

Write-Host "Running make..." -ForegroundColor Yellow
& make.exe -j12 all

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nSUCCESS: Build completed successfully!" -ForegroundColor Green
    Write-Host "Output binary: Debug\tron_pdm_detect_cpu.srec" -ForegroundColor Green
    Write-Host "Output ELF:    Debug\tron_pdm_detect_cpu.elf" -ForegroundColor Green
} else {
    Write-Host "`nERROR: Build failed with exit code $LASTEXITCODE" -ForegroundColor Red
}

Pop-Location
pause
