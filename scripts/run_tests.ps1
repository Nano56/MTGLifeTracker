# Native C++ Test Runner for PowerShell with Logging, Test Report & Pause
param(
    [switch]$NoPause = $false,
    [string]$LogDir = "logs"
)

$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

# Ensure MinGW g++ is in PATH
if (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    if (Test-Path "C:\mingw64\bin\g++.exe") {
        $env:Path = "C:\mingw64\bin;" + $env:Path
    } else {
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
    }
}

# Setup log directory and files
if (-not (Test-Path $LogDir)) {
    New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
}

$startTime = Get-Date
$timestampStr = $startTime.ToString("yyyyMMdd_HHmmss")
$logFile = Join-Path $LogDir "test_run_$timestampStr.log"
$latestLog = Join-Path $LogDir "test_run_latest.log"

# Clear or start new log file
Set-Content -Path $logFile -Value "" -Encoding utf8

function Log-Message([string]$msg, [System.ConsoleColor]$color = [System.ConsoleColor]::White, [switch]$NoConsole) {
    if (-not $NoConsole) {
        if ($color -eq [System.ConsoleColor]::White) {
            Write-Host $msg
        } else {
            Write-Host $msg -ForegroundColor $color
        }
    }
    Add-Content -Path $logFile -Value $msg -Encoding utf8
}

# Compiler version
$gppVersion = (g++ --version 2>&1 | Select-Object -First 1)

Log-Message "========================================================" -color Cyan
Log-Message "    MTG COMMANDER LIFE TRACKER - NATIVE C++ TESTS" -color Cyan
Log-Message "========================================================" -color Cyan
Log-Message "  Start Time : $($startTime.ToString('yyyy-MM-dd HH:mm:ss'))" -color Gray
Log-Message "  Compiler   : $gppVersion" -color Gray
Log-Message "  Log File   : $logFile" -color Gray
Log-Message "--------------------------------------------------------" -color Gray

$suiteResults = @()
$totalAssertions = 0
$overallSuccess = $true
$testIndex = 1

$testSuites = @(
    @{
        Name = "test_gamestate"
        Sources = @("tests/test_gamestate.cpp", "GameState.cpp", "EventLog.cpp")
        Binary = "tests/test_gamestate.exe"
        ExtraFlags = @()
    },
    @{
        Name = "test_eventlog"
        Sources = @("tests/test_eventlog.cpp", "EventLog.cpp")
        Binary = "tests/test_eventlog.exe"
        ExtraFlags = @()
    },
    @{
        Name = "test_inputdecoder"
        Sources = @("tests/test_inputdecoder.cpp", "InputDecoder.cpp")
        Binary = "tests/test_inputdecoder.exe"
        ExtraFlags = @()
    },
    @{
        Name = "test_scenarios"
        Sources = @("tests/test_scenarios.cpp", "GameState.cpp", "EventLog.cpp", "AppController.cpp")
        Binary = "tests/test_scenarios.exe"
        ExtraFlags = @()
    }
)

if (Test-Path "tests/test_display.cpp") {
    $testSuites += @{
        Name = "test_display"
        Sources = @("tests/test_display.cpp", "DisplayManager.cpp", "GameState.cpp", "EventLog.cpp")
        Binary = "tests/test_display.exe"
        ExtraFlags = @("-Itests")
    }
}

foreach ($suite in $testSuites) {
    $sName = $suite.Name
    Log-Message "`n[$testIndex/$($testSuites.Count)] Building & Running $sName..." -color Yellow

    $compileStart = [System.Diagnostics.Stopwatch]::StartNew()
    $compileCmd = @("g++", "-std=c++17", "-Wall", "-Wextra", "-O2") + $suite.ExtraFlags + $suite.Sources + @("-o", $suite.Binary)
    
    $compileOutput = & $compileCmd[0] $compileCmd[1..($compileCmd.Length-1)] 2>&1
    $compileExit = $LASTEXITCODE
    $compileStart.Stop()
    
    if ($compileExit -ne 0) {
        Log-Message "[FAIL] Compilation failed for $sName (exit code $compileExit):" -color Red
        Log-Message ($compileOutput -join "`n") -color Red
        $suiteResults += [PSCustomObject]@{
            Index = $testIndex
            Name = $sName
            Assertions = 0
            CompileTime = [math]::Round($compileStart.Elapsed.TotalSeconds, 2)
            RunTime = 0.0
            Status = "COMPILE_FAIL"
        }
        $overallSuccess = $false
        $testIndex++
        continue
    }

    $runStart = [System.Diagnostics.Stopwatch]::StartNew()
    $runOutput = & ".\$($suite.Binary)" 2>&1
    $runExit = $LASTEXITCODE
    $runStart.Stop()

    # Log run output
    foreach ($line in $runOutput) {
        $strLine = [string]$line
        if ($strLine -match "\[PASS\]" -or $strLine -match ">>>") {
            Log-Message "  $strLine" -color Green
        } elseif ($strLine -match "\[FAIL\]" -or $strLine -match "Assertion failed") {
            Log-Message "  $strLine" -color Red
        } else {
            Log-Message "  $strLine" -color White
        }
    }

    $assertions = 0
    $fullRunText = ($runOutput -join "`n")
    if ($fullRunText -match "ALL\s+(\d+)\s+ASSERTIONS PASSED") {
        $assertions = [int]$Matches[1]
    }

    if ($runExit -eq 0) {
        $totalAssertions += $assertions
        $suiteResults += [PSCustomObject]@{
            Index = $testIndex
            Name = $sName
            Assertions = $assertions
            CompileTime = [math]::Round($compileStart.Elapsed.TotalSeconds, 2)
            RunTime = [math]::Round($runStart.Elapsed.TotalSeconds, 2)
            Status = "PASSED"
        }
    } else {
        $overallSuccess = $false
        $suiteResults += [PSCustomObject]@{
            Index = $testIndex
            Name = $sName
            Assertions = $assertions
            CompileTime = [math]::Round($compileStart.Elapsed.TotalSeconds, 2)
            RunTime = [math]::Round($runStart.Elapsed.TotalSeconds, 2)
            Status = "FAILED"
        }
        Log-Message "[FAIL] $sName failed with exit code $runExit" -color Red
    }

    $testIndex++
}

# Build Desktop CLI Simulator
Log-Message "`n[*] Building Desktop CLI Simulator (tools/cli_simulator.exe)..." -color Cyan
$cliStart = [System.Diagnostics.Stopwatch]::StartNew()
$cliOutput = & g++ -std=c++17 -Wall -Wextra -O2 tools/cli_simulator.cpp GameState.cpp EventLog.cpp AppController.cpp -o tools/cli_simulator.exe 2>&1
$cliExit = $LASTEXITCODE
$cliStart.Stop()

$cliStatus = "SUCCESS"
if ($cliExit -ne 0) {
    $cliStatus = "FAILED"
    Log-Message "[FAIL] Compilation failed for tools/cli_simulator.exe:`n$($cliOutput -join "`n")" -color Red
    $overallSuccess = $false
} else {
    Log-Message "  [OK] tools/cli_simulator.exe built! Run '.\tools\cli_simulator.exe' to test in terminal." -color Green
}

# Generate and Display Test Report
$endTime = Get-Date
$totalDuration = [math]::Round(($endTime - $startTime).TotalSeconds, 2)
$passedCount = ($suiteResults | Where-Object { $_.Status -eq "PASSED" }).Count
$totalCount = $suiteResults.Count

Log-Message "`n========================================================================" -color Cyan
Log-Message "                         TEST EXECUTION REPORT" -color Cyan
Log-Message "========================================================================" -color Cyan
Log-Message ("  Execution Date : {0}" -f $startTime.ToString("yyyy-MM-dd HH:mm:ss")) -color White
Log-Message ("  Compiler       : {0}" -f $gppVersion) -color White
Log-Message ("  Host Platform  : Windows (x86_64 native host)") -color White
Log-Message ("  Firmware Target: ESP32 MTG Life Tracker Architecture") -color White
Log-Message "------------------------------------------------------------------------" -color Gray
Log-Message ("  {0,-3}  {1,-22}  {2,11}  {3,10}  {4,-10}" -f "#", "Suite Name", "Assertions", "Duration", "Status") -color Gray
Log-Message "------------------------------------------------------------------------" -color Gray

foreach ($r in $suiteResults) {
    $statusColor = if ($r.Status -eq "PASSED") { [System.ConsoleColor]::Green } else { [System.ConsoleColor]::Red }
    $durStr = "{0:F2}s" -f ($r.CompileTime + $r.RunTime)
    $assertStr = if ($r.Assertions -gt 0) { "$($r.Assertions)" } else { "N/A" }
    Log-Message ("  {0,-3}  {1,-22}  {2,11}  {3,10}  {4,-10}" -f $r.Index, $r.Name, $assertStr, $durStr, $r.Status) -color $statusColor
}

$cliDurStr = "{0:F2}s" -f $cliStart.Elapsed.TotalSeconds
$cliColor = if ($cliStatus -eq "SUCCESS") { [System.ConsoleColor]::Green } else { [System.ConsoleColor]::Red }
Log-Message ("  {0,-3}  {1,-22}  {2,11}  {3,10}  {4,-10}" -f "*", "cli_simulator (build)", "-", $cliDurStr, $cliStatus) -color $cliColor

Log-Message "------------------------------------------------------------------------" -color Gray
Log-Message ("  Suites Passed    : {0} / {1} ({2}%)" -f $passedCount, $totalCount, [math]::Round(($passedCount / [math]::Max(1, $totalCount)) * 100, 1)) -color White
Log-Message ("  Total Assertions : {0} passed" -f $totalAssertions) -color White
Log-Message ("  Total Run Time   : {0}s" -f $totalDuration) -color White
if ($overallSuccess) {
    Log-Message "  Overall Verdict  : [PASS] ALL UNIT & SCENARIO TESTS PASSED (100%)" -color Green
} else {
    Log-Message "  Overall Verdict  : [FAIL] ONE OR MORE SUITES FAILED" -color Red
}
Log-Message ("  Full Log Saved   : {0}" -f $latestLog) -color White
Log-Message "========================================================================" -color Cyan

# Copy complete log file to test_run_latest.log
Copy-Item -Path $logFile -Destination $latestLog -Force

# Pause for user review
if (-not $NoPause) {
    if ([Environment]::UserInteractive -and -not [Console]::IsInputRedirected) {
        Write-Host "`nPress [Enter] to continue..." -ForegroundColor Cyan -NoNewline
        [void][System.Console]::ReadLine()
    }
}

if (-not $overallSuccess) {
    exit 1
}
