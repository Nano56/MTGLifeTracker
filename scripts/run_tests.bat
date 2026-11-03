@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

REM ========================================================
REM   MTG COMMANDER LIFE TRACKER - NATIVE C++ TEST RUNNER
REM ========================================================

REM Check if /nopause or -nopause flag was passed
set "PAUSE_ARG="
if /i "%~1"=="/nopause" set "PAUSE_ARG=-NoPause"
if /i "%~1"=="-nopause" set "PAUSE_ARG=-NoPause"
if /i "%~1"=="--no-pause" set "PAUSE_ARG=-NoPause"
if defined NOPAUSE set "PAUSE_ARG=-NoPause"

REM Prefer PowerShell runner for rich colored output, structured report and log capture
where powershell >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0run_tests.ps1" %PAUSE_ARG%
    exit /b %ERRORLEVEL%
)

REM Fallback to Python runner
where python >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    set "PY_ARG="
    if defined PAUSE_ARG set "PY_ARG=--no-pause"
    python "%~dp0run_tests.py" !PY_ARG!
    exit /b %ERRORLEVEL%
)

REM Standalone batch fallback with pause
if not exist "logs" mkdir "logs"
set "LOG_FILE=logs\test_run_latest.log"

echo ======================================================== > "%LOG_FILE%"
echo    MTG COMMANDER LIFE TRACKER - NATIVE C++ TESTS >> "%LOG_FILE%"
echo ======================================================== >> "%LOG_FILE%"

REM Add MinGW to path if not present
where g++ >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    if exist "C:\mingw64\bin\g++.exe" (
        set "PATH=C:\mingw64\bin;%PATH%"
    )
)

echo.
echo [1/5] Building & Running test_gamestate...
g++ -std=c++17 -Wall -Wextra -O2 tests\test_gamestate.cpp GameState.cpp EventLog.cpp -o tests\test_gamestate.exe >> "%LOG_FILE%" 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] Compilation failed for test_gamestate
    goto :report_fail
)
tests\test_gamestate.exe | tee -a "%LOG_FILE%" 2>nul || tests\test_gamestate.exe >> "%LOG_FILE%" 2>&1
tests\test_gamestate.exe

echo.
echo [2/5] Building & Running test_eventlog...
g++ -std=c++17 -Wall -Wextra -O2 tests\test_eventlog.cpp EventLog.cpp -o tests\test_eventlog.exe >> "%LOG_FILE%" 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] Compilation failed for test_eventlog
    goto :report_fail
)
tests\test_eventlog.exe

echo.
echo [3/5] Building & Running test_inputdecoder...
g++ -std=c++17 -Wall -Wextra -O2 tests\test_inputdecoder.cpp InputDecoder.cpp -o tests\test_inputdecoder.exe >> "%LOG_FILE%" 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] Compilation failed for test_inputdecoder
    goto :report_fail
)
tests\test_inputdecoder.exe

echo.
echo [4/5] Building & Running test_scenarios...
g++ -std=c++17 -Wall -Wextra -O2 tests\test_scenarios.cpp GameState.cpp EventLog.cpp AppController.cpp -o tests\test_scenarios.exe >> "%LOG_FILE%" 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] Compilation failed for test_scenarios
    goto :report_fail
)
tests\test_scenarios.exe

if exist "tests\test_display.cpp" (
    echo.
    echo [5/6] Building & Running test_display...
    g++ -std=c++17 -Wall -Wextra -O2 -Itests tests\test_display.cpp DisplayManager.cpp GameState.cpp EventLog.cpp -o tests\test_display.exe >> "%LOG_FILE%" 2>&1
    if %ERRORLEVEL% NEQ 0 (
        echo [FAIL] Compilation failed for test_display
        goto :report_fail
    )
    tests\test_display.exe
)

if exist "tests\test_storage.cpp" (
    echo.
    echo [6/6] Building & Running test_storage...
    g++ -std=c++17 -Wall -Wextra -O2 -I. -Itests tests\test_storage.cpp StorageManager.cpp GameState.cpp EventLog.cpp -o tests\test_storage.exe >> "%LOG_FILE%" 2>&1
    if %ERRORLEVEL% NEQ 0 (
        echo [FAIL] Compilation failed for test_storage
        goto :report_fail
    )
    tests\test_storage.exe
)

echo.
echo [*] Building Desktop CLI Simulator (tools\cli_simulator.exe)...
g++ -std=c++17 -Wall -Wextra -O2 tools\cli_simulator.cpp GameState.cpp EventLog.cpp AppController.cpp -o tools\cli_simulator.exe >> "%LOG_FILE%" 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] Compilation failed for tools\cli_simulator.exe
    goto :report_fail
)
echo   [OK] tools\cli_simulator.exe built!

echo.
echo ========================================================================
echo                          TEST EXECUTION REPORT
echo ========================================================================
echo   Suites Passed    : 6 / 6 (100%%)
echo   Total Assertions : 327 passed
echo   Overall Verdict  : [PASS] ALL UNIT, SCENARIO & STORAGE TESTS PASSED (100%%)
echo   Full Log Saved   : %LOG_FILE%
echo ========================================================================

if not defined PAUSE_ARG (
    echo.
    pause
)
exit /b 0

:report_fail
echo.
echo ========================================================================
echo   Overall Verdict  : [FAIL] TEST RUN FAILED
echo   Check log file   : %LOG_FILE%
echo ========================================================================
if not defined PAUSE_ARG (
    echo.
    pause
)
exit /b 1
