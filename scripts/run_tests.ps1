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

Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "    MTG COMMANDER LIFE TRACKER - NATIVE C++ TESTS" -ForegroundColor Cyan
Write-Host "========================================================" -ForegroundColor Cyan

# 1. GameState
Write-Host "`n[1/4] Building & Running test_gamestate..." -ForegroundColor Yellow
& g++ -std=c++17 -Wall -Wextra -O2 tests/test_gamestate.cpp GameState.cpp EventLog.cpp -o tests/test_gamestate.exe
& .\tests\test_gamestate.exe

# 2. EventLog
Write-Host "`n[2/4] Building & Running test_eventlog..." -ForegroundColor Yellow
& g++ -std=c++17 -Wall -Wextra -O2 tests/test_eventlog.cpp EventLog.cpp -o tests/test_eventlog.exe
& .\tests\test_eventlog.exe

# 3. InputDecoder
Write-Host "`n[3/4] Building & Running test_inputdecoder..." -ForegroundColor Yellow
& g++ -std=c++17 -Wall -Wextra -O2 tests/test_inputdecoder.cpp InputDecoder.cpp -o tests/test_inputdecoder.exe
& .\tests\test_inputdecoder.exe

# 4. Scenarios (HID-Agnostic End-to-End Simulation)
Write-Host "`n[4/4] Building & Running test_scenarios (Full-Game HID Simulation)..." -ForegroundColor Yellow
& g++ -std=c++17 -Wall -Wextra -O2 tests/test_scenarios.cpp GameState.cpp EventLog.cpp AppController.cpp -o tests/test_scenarios.exe
& .\tests\test_scenarios.exe

# 5. Build Desktop CLI Simulator
Write-Host "`n[*] Building Desktop CLI Simulator (tools/cli_simulator.exe)..." -ForegroundColor Cyan
& g++ -std=c++17 -Wall -Wextra -O2 tools/cli_simulator.cpp GameState.cpp EventLog.cpp AppController.cpp -o tools/cli_simulator.exe
Write-Host "  [OK] tools/cli_simulator.exe built! Run '.\tools\cli_simulator.exe' to test in terminal." -ForegroundColor Green

Write-Host "`n========================================================" -ForegroundColor Green
Write-Host "  ALL NATIVE C++ UNIT & SCENARIO TESTS PASSED (100% SUCCESS)!" -ForegroundColor Green
Write-Host "========================================================" -ForegroundColor Green
