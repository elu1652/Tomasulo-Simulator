#!/usr/bin/env python3

import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TEST_DIR = ROOT / "tests"
BUILD_DIR = ROOT / "build"
SIMULATOR = BUILD_DIR / "simulator"
TRACE_PATH = ROOT / "trace.json"


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
    expected_stats = {}
    expected_exec_end_after = []
    architecture_config_items = []

    # ARCH_L1D_* comments configure cache behavior only for the current test.
    # Tests without these directives keep the simulator default: L1D disabled.
    arch_l1d_keys = {
        "ARCH_L1D_ENABLED": "l1dEnabled",
        "ARCH_L1D_NUM_SETS": "l1dNumSets",
        "ARCH_L1D_BLOCK_SIZE_WORDS": "l1dBlockSizeWords",
        "ARCH_L1D_HIT_LATENCY": "l1dHitLatency",
        "ARCH_L1D_MISS_PENALTY": "l1dMissPenalty",
    }

    # EXPECT_L1D_* comments validate trace.json performanceStats, not stdout.
    l1d_stat_keys = {
        "EXPECT_L1D_ACCESSES": "l1dAccesses",
        "EXPECT_L1D_HITS": "l1dHits",
        "EXPECT_L1D_MISSES": "l1dMisses",
        "EXPECT_L1D_WRITEBACKS": "l1dWritebacks",
    }

    with open(test_file, "r") as f:
        for line in f:
            line = line.strip()

            arch_config_match = re.match(r"#\s*ARCH_CONFIG\s+(.+)\s*$", line)
            if arch_config_match:
                architecture_config_items.extend(
                    item.strip()
                    for item in arch_config_match.group(1).split(",")
                    if item.strip()
                )

            arch_l1d_match = re.match(r"#\s*(ARCH_L1D_[A-Z_]+)\s+([A-Za-z0-9_-]+)\s*$", line)
            if arch_l1d_match:
                directive = arch_l1d_match.group(1)
                value = arch_l1d_match.group(2)

                if directive in arch_l1d_keys:
                    architecture_config_items.append(
                        f"{arch_l1d_keys[directive]}={value}"
                    )

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

            stat_match = re.match(r"#\s*EXPECT_STAT\s+([A-Za-z0-9_]+)\s+(-?\d+(?:\.\d+)?)", line)
            if stat_match:
                key = stat_match.group(1)
                value_text = stat_match.group(2)
                expected_stats[key] = (
                    float(value_text)
                    if "." in value_text
                    else int(value_text)
                )

            l1d_stat_match = re.match(r"#\s*(EXPECT_L1D_[A-Z_]+)\s+(-?\d+)", line)
            if l1d_stat_match:
                directive = l1d_stat_match.group(1)

                if directive in l1d_stat_keys:
                    expected_stats[l1d_stat_keys[directive]] = int(
                        l1d_stat_match.group(2)
                    )

            memory_stall_match = re.match(r"#\s*EXPECT_MEMORY_STALL_CYCLES\s+(-?\d+)", line)
            if memory_stall_match:
                expected_stats["memoryStallCycles"] = int(
                    memory_stall_match.group(1)
                )

            exec_end_match = re.match(
                r"#\s*EXPECT_EXEC_END_AFTER\s+(.+?)\s+AFTER\s+(.+)\s*$",
                line
            )
            if exec_end_match:
                # Used by cache pending-fill tests to ensure a younger load
                # cannot complete before the miss that fills its block.
                expected_exec_end_after.append((
                    exec_end_match.group(1).strip(),
                    exec_end_match.group(2).strip(),
                ))

    architecture_config = (
        ",".join(architecture_config_items)
        if architecture_config_items
        else None
    )

    return (
        expected_regs,
        expected_mem,
        expected_commit_counts,
        expected_stats,
        architecture_config,
        expected_exec_end_after,
    )


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


def parse_performance_stats():
    with open(TRACE_PATH, "r") as f:
        trace = json.load(f)

    stats = trace.get("performanceStats", {})
    return stats if isinstance(stats, dict) else {}


def parse_instruction_status():
    with open(TRACE_PATH, "r") as f:
        trace = json.load(f)

    statuses = trace.get("instructionStatus", [])
    return statuses if isinstance(statuses, list) else []


def run_test(test_file):
    (
        expected_regs,
        expected_mem,
        expected_commit_counts,
        expected_stats,
        architecture_config,
        expected_exec_end_after,
    ) = parse_expectations(test_file)

    command = [str(SIMULATOR), str(test_file)]

    if architecture_config:
        command.extend(["--arch-config", architecture_config])

    result = subprocess.run(
        command,
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
    actual_stats = {}

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

    if expected_stats:
        try:
            actual_stats = parse_performance_stats()
        except (FileNotFoundError, json.JSONDecodeError) as error:
            failures.append(f"Could not read performance stats from trace.json: {error}")

    for key, expected_value in expected_stats.items():
        actual_value = actual_stats.get(key)

        if isinstance(expected_value, float):
            if not isinstance(actual_value, (int, float)) or abs(actual_value - expected_value) > 1e-6:
                failures.append(
                    f"Stat {key}: expected {expected_value}, got {actual_value}"
                )
        elif actual_value != expected_value:
            failures.append(
                f"Stat {key}: expected {expected_value}, got {actual_value}"
            )

    if expected_exec_end_after:
        try:
            instruction_status = parse_instruction_status()
        except (FileNotFoundError, json.JSONDecodeError) as error:
            failures.append(f"Could not read instruction status from trace.json: {error}")
            instruction_status = []

        status_by_text = {
            entry.get("rawText"): entry
            for entry in instruction_status
            if isinstance(entry, dict)
        }

        for later_text, earlier_text in expected_exec_end_after:
            later_status = status_by_text.get(later_text)
            earlier_status = status_by_text.get(earlier_text)

            if later_status is None:
                failures.append(f"Missing instruction status for '{later_text}'")
                continue

            if earlier_status is None:
                failures.append(f"Missing instruction status for '{earlier_text}'")
                continue

            later_end = later_status.get("execEndCycle")
            earlier_end = earlier_status.get("execEndCycle")

            if not isinstance(later_end, int) or not isinstance(earlier_end, int):
                failures.append(
                    f"Execution timing for '{later_text}' or '{earlier_text}' is not numeric"
                )
            elif later_end <= earlier_end:
                failures.append(
                    f"Execution end order: expected '{later_text}' ({later_end}) "
                    f"after '{earlier_text}' ({earlier_end})"
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
