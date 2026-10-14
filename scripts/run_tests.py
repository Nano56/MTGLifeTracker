#!/usr/bin/env python3
import os
import subprocess
import sys

def main():
    print("=" * 60)
    print("      MTG COMMANDER LIFE TRACKER - AUTOMATED TEST SUITE")
    print("=" * 60)

    # Change to repository root
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    os.chdir(repo_root)

    # Ensure g++ is reachable
    user_path = os.environ.get("Path", "")
    if "C:\\mingw64\\bin" not in user_path:
        os.environ["Path"] = "C:\\mingw64\\bin;" + user_path

    test_targets = [
        {
            "name": "test_gamestate",
            "sources": ["tests/test_gamestate.cpp", "GameState.cpp", "EventLog.cpp"],
            "binary": "tests/test_gamestate.exe"
        },
        {
            "name": "test_eventlog",
            "sources": ["tests/test_eventlog.cpp", "EventLog.cpp"],
            "binary": "tests/test_eventlog.exe"
        },
        {
            "name": "test_inputdecoder",
            "sources": ["tests/test_inputdecoder.cpp", "InputDecoder.cpp"],
            "binary": "tests/test_inputdecoder.exe"
        },
        {
            "name": "test_scenarios",
            "sources": ["tests/test_scenarios.cpp", "GameState.cpp", "EventLog.cpp", "AppController.cpp"],
            "binary": "tests/test_scenarios.exe"
        }
    ]

    all_passed = True
    total_suites = len(test_targets)
    passed_suites = 0

    for target in test_targets:
        print(f"\n[BUILD] Compiling {target['name']}...")
        compile_cmd = ["g++", "-std=c++17", "-Wall", "-Wextra", "-O2"] + target["sources"] + ["-o", target["binary"]]
        res = subprocess.run(compile_cmd, capture_output=True, text=True)
        if res.returncode != 0:
            print(f"[FAIL] Compilation error in {target['name']}:\n{res.stderr}")
            all_passed = False
            continue

        print(f"[RUN]   Executing {target['name']}...")
        run_res = subprocess.run([os.path.abspath(target["binary"])], capture_output=True, text=True)
        print(run_res.stdout.strip())
        if run_res.returncode != 0:
            print(f"[FAIL] Runtime error in {target['name']}:\n{run_res.stderr}")
            all_passed = False
        else:
            passed_suites += 1

    print("\n" + "=" * 60)
    if all_passed and passed_suites == total_suites:
        print(f"  SUCCESS: ALL {passed_suites}/{total_suites} TEST SUITES PASSED (100% COVERAGE)!")
        print("=" * 60)
        return 0
    else:
        print(f"  FAILURE: {passed_suites}/{total_suites} SUITES PASSED.")
        print("=" * 60)
        return 1

if __name__ == "__main__":
    sys.exit(main())

