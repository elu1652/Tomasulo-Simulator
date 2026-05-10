#!/usr/bin/env python3

import os
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TEST_DIR = ROOT / "tests"
BUILD_DIR = ROOT / "build"
SIMULATOR = BUILD_DIR / "simulator"


def build_project():
    BUILD_DIR.mkdir(exist_ok=True)

    subprocess.run(
        ["cmake", ".."],
        cwd=BUILD_DIR,
        check=True
    )

    subprocess.run(
        ["make"],
        cwd=BUILD_DIR,
        check=True
    )


def parse_expectations(test_file):
    expected_regs = {}
    expected_mem = {}
    expected_commit_counts = {}

    with open(test_file, "r") as f:
        for line in f:
            line = line.strip()

            # Register expectations
            reg_match = re.match(r"#\s*EXPECT_REG\s+R(\d+)\s+(-?\d+)", line)
            if reg_match:
                reg = int(reg_match.group(1))
                value = int(reg_match.group(2))
                expected_regs[reg] = value

            # Memory expectations
            mem_match = re.match(r"#\s*EXPECT_MEM\s+(\d+)\s+(-?\d+)", line)
            if mem_match:
                addr = int(mem_match.group(1))
                value = int(mem_match.group(2))
                expected_mem[addr] = value

            # Expected number of commits
            commit_match = re.match(r"#\s*EXPECT_COMMIT_COUNT\s+(.+)\s+(\d+)\s*$", line)
            if commit_match:
                instr = commit_match.group(1).strip()
                count = int(commit_match.group(2))
                expected_commit_counts[instr] = count

    return expected_regs, expected_mem, expected_commit_counts


def parse_final_registers(output):
    regs = {}
    in_reg_section = False

    for line in output.splitlines():
        if line.strip() == "Final Register State:":
            in_reg_section = True
            continue

        if line.strip() == "Memory State:":
            in_reg_section = False
            continue

        if in_reg_section:
            match = re.match(r"R(\d+):\s*(-?\d+)", line.strip())
            if match:
                regs[int(match.group(1))] = int(match.group(2))

    return regs


def parse_final_memory(output):
    mem = {}
    in_mem_section = False

    for line in output.splitlines():
        if line.strip() == "Memory State:":
            in_mem_section = True
            continue

        if line.strip() == "Instruction Status Table:":
            in_mem_section = False
            continue

        if in_mem_section:
            match = re.match(r"Mem\[(\d+)\]:\s*(-?\d+)", line.strip())
            if match:
                mem[int(match.group(1))] = int(match.group(2))

    return mem

def parse_commit_counts(output):
    commit_counts = {}

    for line in output.splitlines():
        match = re.match(r"ROB Commit:\s+I\d+\s+(.+)", line.strip())
        if match:
            instr = match.group(1).strip()
            commit_counts[instr] = commit_counts.get(instr, 0) + 1

    return commit_counts

def run_test(test_file):
    expected_regs, expected_mem, expected_commit_counts = parse_expectations(test_file)

    result = subprocess.run(
        [str(SIMULATOR), str(test_file)],
        cwd=BUILD_DIR,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )

    output = result.stdout + result.stderr

    if result.returncode != 0:
        return False, f"Simulator exited with code {result.returncode}\n{output}"

    actual_regs = parse_final_registers(output)
    actual_mem = parse_final_memory(output)
    actual_commit_counts = parse_commit_counts(output)

    failures = []

    for reg, expected_value in expected_regs.items():
        actual_value = actual_regs.get(reg)

        if actual_value != expected_value:
            failures.append(
                f"R{reg}: expected {expected_value}, got {actual_value}"
            )

    for addr, expected_value in expected_mem.items():
        actual_value = actual_mem.get(addr)

        if actual_value != expected_value:
            failures.append(
                f"Mem[{addr}]: expected {expected_value}, got {actual_value}"
            )

    for instr, expected_count in expected_commit_counts.items():
        actual_count = actual_commit_counts.get(instr, 0)

        if actual_count != expected_count:
            failures.append(
                f"Commit count for '{instr}': expected {expected_count}, got {actual_count}"
            )

    if failures:
        return False, "\n".join(failures)

    return True, "passed"


def main():
    build_project()

    test_files = sorted(TEST_DIR.glob("*.asm"))

    if not test_files:
        print("No .asm test files found.")
        return 1

    passed = 0
    failed = 0

    for test_file in test_files:
        ok, message = run_test(test_file)

        if ok:
            print(f"[PASS] {test_file.name}")
            passed += 1
        else:
            print(f"[FAIL] {test_file.name}")
            print(message)
            failed += 1

    print()
    print(f"Passed: {passed}")
    print(f"Failed: {failed}")

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())