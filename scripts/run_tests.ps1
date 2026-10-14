# Native C++ Test Runner for PowerShell
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

if (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    if (Test-Path "C:\mingw64\bin\g++.exe") {
        $env:Path = "C:\mingw64\bin;" + $env:Path
    } else {
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
    }
}

function Invoke-Step([scriptblock]$cmd, [string]$label) {
    & $cmd
    if ($LASTEXITCODE -ne 0) {
        Write-Host "`n[FAIL] $label failed with exit code $LASTEXITCODE" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "    MTG COMMANDER LIFE TRACKER - NATIVE C++ TESTS" -ForegroundColor Cyan
Write-Host "========================================================" -ForegroundColor Cyan

# 1. GameState
Write-Host "`n[1/5] Building & Running test_gamestate..." -ForegroundColor Yellow
Invoke-Step { g++ -std=c++17 -Wall -Wextra -O2 tests/test_gamestate.cpp GameState.cpp EventLog.cpp -o tests/test_gamestate.exe } "Compile test_gamestate"
Invoke-Step { .\tests\test_gamestate.exe } "Execute test_gamestate"

# 2. EventLog
Write-Host "`n[2/5] Building & Running test_eventlog..." -ForegroundColor Yellow
Invoke-Step { g++ -std=c++17 -Wall -Wextra -O2 tests/test_eventlog.cpp EventLog.cpp -o tests/test_eventlog.exe } "Compile test_eventlog"
Invoke-Step { .\tests\test_eventlog.exe } "Execute test_eventlog"

# 3. InputDecoder
Write-Host "`n[3/5] Building & Running test_inputdecoder..." -ForegroundColor Yellow
Invoke-Step { g++ -std=c++17 -Wall -Wextra -O2 tests/test_inputdecoder.cpp InputDecoder.cpp -o tests/test_inputdecoder.exe } "Compile test_inputdecoder"
Invoke-Step { .\tests\test_inputdecoder.exe } "Execute test_inputdecoder"

# 4. Scenarios (HID-Agnostic End-to-End Simulation)
Write-Host "`n[4/5] Building & Running test_scenarios (Full-Game HID Simulation)..." -ForegroundColor Yellow
Invoke-Step { g++ -std=c++17 -Wall -Wextra -O2 tests/test_scenarios.cpp GameState.cpp EventLog.cpp AppController.cpp -o tests/test_scenarios.exe } "Compile test_scenarios"
Invoke-Step { .\tests\test_scenarios.exe } "Execute test_scenarios"

# 5. DisplayManager (Native GFX Mock Compilation & Verification)
if (Test-Path "tests/test_display.cpp") {
    Write-Host "`n[5/5] Building & Running test_display (Native DisplayManager Validation)..." -ForegroundColor Yellow
    Invoke-Step { g++ -std=c++17 -Wall -Wextra -O2 -Itests tests/test_display.cpp DisplayManager.cpp GameState.cpp EventLog.cpp -o tests/test_display.exe } "Compile test_display"
    Invoke-Step { .\tests\test_display.exe } "Execute test_display"
}

# 6. Build Desktop CLI Simulator
Write-Host "`n[*] Building Desktop CLI Simulator (tools/cli_simulator.exe)..." -ForegroundColor Cyan
Invoke-Step { g++ -std=c++17 -Wall -Wextra -O2 tools/cli_simulator.cpp GameState.cpp EventLog.cpp AppController.cpp -o tools/cli_simulator.exe } "Compile tools/cli_simulator.exe"
Write-Host "  [OK] tools/cli_simulator.exe built! Run '.\tools\cli_simulator.exe' to test in terminal." -ForegroundColor Green

Write-Host "`n========================================================" -ForegroundColor Green
Write-Host "  ALL NATIVE C++ UNIT & SCENARIO TESTS PASSED (100% SUCCESS)!" -ForegroundColor Green
Write-Host "========================================================" -ForegroundColor Green
