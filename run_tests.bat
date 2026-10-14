@echo off
setlocal
echo ========================================================
echo    MTG COMMANDER LIFE TRACKER - NATIVE C++ TESTS
echo ========================================================

REM Add MinGW to path if not present
where g++ >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    if exist "C:\mingw64\bin\g++.exe" (
        set "PATH=C:\mingw64\bin;%PATH%"
    )
)

REM 1. GameState Native Test
echo.
echo [1/4] Compiling and running test_gamestate...
g++ -std=c++17 -Wall -Wextra -O2 tests\test_gamestate.cpp GameState.cpp EventLog.cpp -o tests\test_gamestate.exe
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] Compilation failed for test_gamestate
    exit /b 1
)
tests\test_gamestate.exe
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] test_gamestate failed
    exit /b 1
)

REM 2. EventLog Native Test
echo.
echo [2/4] Compiling and running test_eventlog...
g++ -std=c++17 -Wall -Wextra -O2 tests\test_eventlog.cpp EventLog.cpp -o tests\test_eventlog.exe
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] Compilation failed for test_eventlog
    exit /b 1
)
tests\test_eventlog.exe
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] test_eventlog failed
    exit /b 1
)

REM 3. InputDecoder Native Test
echo.
echo [3/4] Compiling and running test_inputdecoder...
g++ -std=c++17 -Wall -Wextra -O2 tests\test_inputdecoder.cpp InputDecoder.cpp -o tests\test_inputdecoder.exe
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] Compilation failed for test_inputdecoder
    exit /b 1
)
tests\test_inputdecoder.exe
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] test_inputdecoder failed
    exit /b 1
)

REM 4. Scenarios Native Test
echo.
echo [4/4] Compiling and running test_scenarios...
g++ -std=c++17 -Wall -Wextra -O2 tests\test_scenarios.cpp GameState.cpp EventLog.cpp AppController.cpp -o tests\test_scenarios.exe
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] Compilation failed for test_scenarios
    exit /b 1
)
tests\test_scenarios.exe
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] test_scenarios failed
    exit /b 1
)

REM 5. Build Desktop CLI Simulator
echo.
echo [*] Building Desktop CLI Simulator (cli_simulator.exe)...
g++ -std=c++17 -Wall -Wextra -O2 cli_simulator.cpp GameState.cpp EventLog.cpp AppController.cpp -o cli_simulator.exe
if %ERRORLEVEL% EQU 0 (
    echo   [OK] cli_simulator.exe built! Run 'cli_simulator.exe' to interactively test in terminal.
)

echo.
echo ========================================================
echo   ALL NATIVE C++ UNIT & SCENARIO TESTS PASSED (100%% SUCCESS)!
echo ========================================================
