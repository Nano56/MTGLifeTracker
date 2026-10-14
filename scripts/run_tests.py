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

    all_passed = True
    total_suites = len(test_targets)
    passed_suites = 0

    for target in test_targets:
        print(f"\n[BUILD] Compiling {target['name']}...")
        compile_cmd = ["g++", "-std=c++17", "-Wall", "-Wextra", "-O2"] + target.get("extra_flags", []) + target["sources"] + ["-o", target["binary"]]
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

    # Also build desktop CLI simulator
    print("\n[BUILD] Compiling tools/cli_simulator.exe...")
    cli_cmd = ["g++", "-std=c++17", "-Wall", "-Wextra", "-O2", "tools/cli_simulator.cpp", "GameState.cpp", "EventLog.cpp", "AppController.cpp", "-o", "tools/cli_simulator.exe"]
    res_cli = subprocess.run(cli_cmd, capture_output=True, text=True)
    if res_cli.returncode != 0:
        print(f"[FAIL] Compilation error in tools/cli_simulator.exe:\n{res_cli.stderr}")
        all_passed = False
    else:
        print("  [OK] tools/cli_simulator.exe built successfully.")

    print("\n" + "=" * 60)
    if all_passed and passed_suites == total_suites:
        print(f"  SUCCESS: ALL {passed_suites}/{total_suites} TEST SUITES PASSED!")
        print("=" * 60)
        return 0
    else:
        print(f"  FAILURE: {passed_suites}/{total_suites} SUITES PASSED.")
        print("=" * 60)
        return 1

if __name__ == "__main__":
    sys.exit(main())
