#!/usr/bin/env python3
"""
MTG Commander Life Tracker - Native Test Runner
Automated suite execution with real-time console reporting, structured log file generation,
and an interactive pause for review.
"""

import argparse
import datetime
import os
import re
import shutil
import subprocess
import sys
import time

# ANSI color codes
CYAN = "\033[96m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"
GRAY = "\033[90m"
BOLD = "\033[1m"
RESET = "\033[0m"


def main():
    parser = argparse.ArgumentParser(description="MTG Commander Life Tracker - Test Runner")
    parser.add_argument("--no-pause", "-n", action="store_true", help="Do not pause after test execution")
    parser.add_argument("--log-dir", default="logs", help="Directory where logs will be stored")
    args = parser.parse_args()

    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    os.chdir(repo_root)

    # Ensure g++ is reachable
    user_path = os.environ.get("Path", "")
    if "C:\\mingw64\\bin" not in user_path and os.path.exists("C:\\mingw64\\bin\\g++.exe"):
        os.environ["Path"] = "C:\\mingw64\\bin;" + user_path

    # Create logs directory
    os.makedirs(args.log_dir, exist_ok=True)
    start_time = datetime.datetime.now()
    timestamp_str = start_time.strftime("%Y%m%d_%H%M%S")
    log_file_path = os.path.join(args.log_dir, f"test_run_{timestamp_str}.log")
    latest_log_path = os.path.join(args.log_dir, "test_run_latest.log")

    log_lines = []

    def log_print(msg="", color="", end="\n"):
        if color:
            print(f"{color}{msg}{RESET}", end=end)
        else:
            print(msg, end=end)
        log_lines.append(msg)

    # Get compiler version
    try:
        comp_proc = subprocess.run(["g++", "--version"], capture_output=True, text=True, check=True)
        gpp_version = comp_proc.stdout.strip().splitlines()[0]
    except Exception:
        gpp_version = "g++ (version unknown)"

    log_print("=" * 60, CYAN)
    log_print("    MTG COMMANDER LIFE TRACKER - NATIVE C++ TESTS", CYAN)
    log_print("=" * 60, CYAN)
    log_print(f"  Start Time : {start_time.strftime('%Y-%m-%d %H:%M:%S')}", GRAY)
    log_print(f"  Compiler   : {gpp_version}", GRAY)
    log_print(f"  Log File   : {log_file_path}", GRAY)
    log_print("-" * 60, GRAY)

    test_targets = [
        {
            "name": "test_gamestate",
            "sources": ["tests/test_gamestate.cpp", "GameState.cpp", "EventLog.cpp"],
            "binary": "tests/test_gamestate.exe",
            "extra_flags": []
        },
        {
            "name": "test_eventlog",
            "sources": ["tests/test_eventlog.cpp", "EventLog.cpp"],
            "binary": "tests/test_eventlog.exe",
            "extra_flags": []
        },
        {
            "name": "test_inputdecoder",
            "sources": ["tests/test_inputdecoder.cpp", "InputDecoder.cpp"],
            "binary": "tests/test_inputdecoder.exe",
            "extra_flags": []
        },
        {
            "name": "test_scenarios",
            "sources": ["tests/test_scenarios.cpp", "GameState.cpp", "EventLog.cpp", "AppController.cpp"],
            "binary": "tests/test_scenarios.exe",
            "extra_flags": []
        }
    ]

    if os.path.exists("tests/test_display.cpp"):
        test_targets.append({
            "name": "test_display",
            "sources": ["tests/test_display.cpp", "DisplayManager.cpp", "GameState.cpp", "EventLog.cpp"],
            "binary": "tests/test_display.exe",
            "extra_flags": ["-Itests"]
        })

    if os.path.exists("tests/test_storage.cpp"):
        test_targets.append({
            "name": "test_storage",
            "sources": ["tests/test_storage.cpp", "StorageManager.cpp", "GameState.cpp", "EventLog.cpp"],
            "binary": "tests/test_storage.exe",
            "extra_flags": ["-I.", "-Itests"]
        })

    all_passed = True
    total_suites = len(test_targets)
    suite_results = []
    total_assertions = 0

    for idx, target in enumerate(test_targets, start=1):
        s_name = target["name"]
        log_print(f"\n[{idx}/{total_suites}] Building & Running {s_name}...", YELLOW)

        t_c_start = time.perf_counter()
        compile_cmd = ["g++", "-std=c++17", "-Wall", "-Wextra", "-O2"] + target.get("extra_flags", []) + target["sources"] + ["-o", target["binary"]]
        c_res = subprocess.run(compile_cmd, capture_output=True, text=True)
        t_c_end = time.perf_counter()
        compile_dur = t_c_end - t_c_start

        if c_res.returncode != 0:
            log_print(f"[FAIL] Compilation error in {s_name}:\n{c_res.stderr}", RED)
            all_passed = False
            suite_results.append({
                "index": idx,
                "name": s_name,
                "assertions": 0,
                "duration": compile_dur,
                "status": "COMPILE_FAIL"
            })
            continue

        t_r_start = time.perf_counter()
        run_res = subprocess.run([os.path.abspath(target["binary"])], capture_output=True, text=True)
        t_r_end = time.perf_counter()
        run_dur = t_r_end - t_r_start
        total_dur = compile_dur + run_dur

        # Print output lines
        for line in run_res.stdout.splitlines():
            if "[PASS]" in line or ">>>" in line:
                log_print(f"  {line}", GREEN)
            elif "[FAIL]" in line or "Assertion failed" in line:
                log_print(f"  {line}", RED)
            else:
                log_print(f"  {line}")

        if run_res.stderr:
            for line in run_res.stderr.splitlines():
                log_print(f"  {line}", RED)

        assertions = 0
        match = re.search(r"ALL\s+(\d+)\s+ASSERTIONS PASSED", run_res.stdout)
        if match:
            assertions = int(match.group(1))

        if run_res.returncode != 0:
            log_print(f"[FAIL] Runtime error in {s_name} (code {run_res.returncode})", RED)
            all_passed = False
            suite_results.append({
                "index": idx,
                "name": s_name,
                "assertions": assertions,
                "duration": total_dur,
                "status": "FAILED"
            })
        else:
            total_assertions += assertions
            suite_results.append({
                "index": idx,
                "name": s_name,
                "assertions": assertions,
                "duration": total_dur,
                "status": "PASSED"
            })

    # Build desktop CLI simulator
    log_print("\n[*] Building Desktop CLI Simulator (tools/cli_simulator.exe)...", CYAN)
    t_cli_start = time.perf_counter()
    cli_cmd = ["g++", "-std=c++17", "-Wall", "-Wextra", "-O2", "tools/cli_simulator.cpp", "GameState.cpp", "EventLog.cpp", "AppController.cpp", "-o", "tools/cli_simulator.exe"]
    res_cli = subprocess.run(cli_cmd, capture_output=True, text=True)
    t_cli_end = time.perf_counter()
    cli_dur = t_cli_end - t_cli_start

    cli_status = "SUCCESS"
    if res_cli.returncode != 0:
        log_print(f"[FAIL] Compilation error in tools/cli_simulator.exe:\n{res_cli.stderr}", RED)
        cli_status = "FAILED"
        all_passed = False
    else:
        log_print("  [OK] tools/cli_simulator.exe built! Run '.\\tools\\cli_simulator.exe' to test in terminal.", GREEN)

    end_time = datetime.datetime.now()
    total_elapsed = (end_time - start_time).total_seconds()
    passed_count = sum(1 for r in suite_results if r["status"] == "PASSED")

    # Format Test Execution Report
    log_print("\n" + "=" * 72, CYAN)
    log_print("                         TEST EXECUTION REPORT", CYAN)
    log_print("=" * 72, CYAN)
    log_print(f"  Execution Date : {start_time.strftime('%Y-%m-%d %H:%M:%S')}")
    log_print(f"  Compiler       : {gpp_version}")
    log_print(f"  Host Platform  : Windows (x86_64 native host)")
    log_print(f"  Firmware Target: ESP32 MTG Life Tracker Architecture")
    log_print("-" * 72, GRAY)
    log_print(f"  {'#':<3}  {'Suite Name':<22}  {'Assertions':>11}  {'Duration':>10}  {'Status':<10}", GRAY)
    log_print("-" * 72, GRAY)

    for r in suite_results:
        color = GREEN if r["status"] == "PASSED" else RED
        assert_str = str(r["assertions"]) if r["assertions"] > 0 else "N/A"
        dur_str = f"{r['duration']:.2f}s"
        log_print(f"  {r['index']:<3}  {r['name']:<22}  {assert_str:>11}  {dur_str:>10}  {r['status']:<10}", color)

    cli_color = GREEN if cli_status == "SUCCESS" else RED
    log_print(f"  {'*':<3}  {'cli_simulator (build)':<22}  {'-':>11}  {cli_dur:.2f}s{'':>4}  {cli_status:<10}", cli_color)

    log_print("-" * 72, GRAY)
    pct = round((passed_count / max(1, total_suites)) * 100, 1)
    log_print(f"  Suites Passed    : {passed_count} / {total_suites} ({pct}%)")
    log_print(f"  Total Assertions : {total_assertions} passed")
    log_print(f"  Total Run Time   : {total_elapsed:.2f}s")
    if all_passed:
        log_print("  Overall Verdict  : [PASS] ALL UNIT & SCENARIO TESTS PASSED (100%)", GREEN)
    else:
        log_print("  Overall Verdict  : [FAIL] ONE OR MORE SUITES FAILED", RED)
    log_print(f"  Full Log Saved   : {latest_log_path}")
    log_print("=" * 72, CYAN)

    # Save log files
    with open(log_file_path, "w", encoding="utf-8") as f:
        f.write("\n".join(log_lines) + "\n")
    shutil.copyfile(log_file_path, latest_log_path)

    # Interactive pause
    if not args.no_pause and sys.stdin.isatty():
        try:
            input(f"\n{CYAN}Press [Enter] to continue...{RESET}")
        except EOFError:
            pass

    return 0 if all_passed else 1


if __name__ == "__main__":
    sys.exit(main())
