#!/usr/bin/env python3

import argparse
import json
import re
import subprocess
from pathlib import Path


PREDICTORS = [
    "always-not-taken",
    "always-taken",
    "one-bit",
    "two-bit",
    "gshare",
]


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def parse_accuracy(stdout: str):
    pattern = r"Branch prediction accuracy:\s+(\d+)/(\d+)\s+correct\s+=\s+([\d.]+)%"
    match = re.search(pattern, stdout)

    if not match:
        return {
            "correct": 0,
            "total": 0,
            "accuracy": 0.0,
        }

    return {
        "correct": int(match.group(1)),
        "total": int(match.group(2)),
        "accuracy": float(match.group(3)),
    }


def load_trace(trace_path: Path):
    if not trace_path.exists():
        return None

    with trace_path.open("r", encoding="utf-8") as f:
        return json.load(f)


def latest_branch_predictions(trace):
    if not trace:
        return []

    cycles = trace.get("cycles", [])
    if not cycles:
        return []

    # Usually the final cycle has all branch prediction entries accumulated.
    return cycles[-1].get("branchPredictions", [])


def latest_predictor_state(trace):
    if not trace:
        return None

    cycles = trace.get("cycles", [])
    if not cycles:
        return None

    return cycles[-1].get("predictorState")


def run_predictor(simulator: Path, asm_file: Path, predictor: str):
    root = repo_root()
    trace_path = root / "trace.json"

    if trace_path.exists():
        trace_path.unlink()

    command = [
        str(simulator),
        str(asm_file),
        "--predictor",
        predictor,
    ]

    result = subprocess.run(
        command,
        cwd=root,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    accuracy = parse_accuracy(result.stdout)
    trace = load_trace(trace_path)

    return {
        "predictor": predictor,
        "returnCode": result.returncode,
        "correct": accuracy["correct"],
        "total": accuracy["total"],
        "accuracy": accuracy["accuracy"],
        "branchPredictions": latest_branch_predictions(trace),
        "predictorState": latest_predictor_state(trace),
        "stdoutTail": result.stdout.splitlines()[-40:],
        "stderr": result.stderr,
    }


def main():
    parser = argparse.ArgumentParser(
        description="Compare branch prediction accuracy across all predictor modes."
    )

    parser.add_argument(
        "asm_file",
        help="Assembly file to run against all predictors.",
    )

    parser.add_argument(
        "--simulator",
        default="build/simulator",
        help="Path to simulator executable. Default: build/simulator",
    )

    parser.add_argument(
        "--out",
        default="predictor_comparison.json",
        help="Output JSON file. Default: predictor_comparison.json",
    )

    args = parser.parse_args()

    root = repo_root()
    asm_file = Path(args.asm_file)
    simulator = Path(args.simulator)

    if not asm_file.is_absolute():
        asm_file = root / asm_file

    if not simulator.is_absolute():
        simulator = root / simulator

    if not asm_file.exists():
        raise FileNotFoundError(f"Assembly file not found: {asm_file}")

    if not simulator.exists():
        raise FileNotFoundError(f"Simulator not found: {simulator}")

    results = []

    for predictor in PREDICTORS:
        print(f"Running {predictor}...")
        results.append(run_predictor(simulator, asm_file, predictor))

    best = max(results, key=lambda item: item["accuracy"])

    output = {
        "program": str(asm_file.relative_to(root)),
        "bestPredictor": best["predictor"],
        "results": results,
    }

    out_path = Path(args.out)
    if not out_path.is_absolute():
        out_path = root / out_path

    with out_path.open("w", encoding="utf-8") as f:
        json.dump(output, f, indent=2)

    print()
    print("Branch Predictor Accuracy Comparison")
    print("-----------------------------------")

    for result in results:
        print(
            f"{result['predictor']:18} "
            f"{result['correct']:>3}/{result['total']:<3} "
            f"{result['accuracy']:>6.2f}%"
        )

    print()
    print(f"Best predictor: {best['predictor']}")
    print(f"Wrote {out_path}")


if __name__ == "__main__":
    main()