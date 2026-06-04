from __future__ import annotations

import json
import os
import subprocess
import tempfile
import threading
from pathlib import Path

from flask import Flask, jsonify, request, send_from_directory


REPO_ROOT = Path(__file__).resolve().parents[1]
VISUALIZER_DIR = REPO_ROOT / "visualizer"
BUILD_DIR = REPO_ROOT / "build"
SIMULATOR_PATH = BUILD_DIR / "simulator"
TRACE_PATH = REPO_ROOT / "trace.json"

MAX_ASSEMBLY_BYTES = 256 * 1024
RUN_TIMEOUT_SECONDS = 10
PREDICTORS = [
    "always-not-taken",
    "always-taken",
    "one-bit",
    "two-bit",
    "gshare",
]

ARCHITECTURE_CONFIG_RANGES = {
    "robCapacity": (1, 128),
    "intRsCapacity": (1, 64),
    "mulRsCapacity": (1, 64),
    "fpAddRsCapacity": (1, 64),
    "fpMulRsCapacity": (1, 64),
    "loadBufferCapacity": (1, 64),
    "storeBufferCapacity": (1, 64),
    "intFuCount": (1, 16),
    "mulFuCount": (1, 16),
    "memFuCount": (1, 16),
    "fpAddPipelineCount": (1, 16),
    "fpAddPipelineDepth": (1, 32),
    "fpMulPipelineCount": (1, 16),
    "fpMulPipelineDepth": (1, 32),
    "intLatency": (1, 64),
    "mulLatency": (1, 64),
    "loadLatency": (1, 64),
    "storeLatency": (1, 64),
    "fpAddLatency": (1, 64),
    "fpMulLatency": (1, 64),
    "fpDivLatency": (1, 64),
    # L1D defaults mirror ArchitectureConfig. The cache remains opt-in; when
    # disabled, fixed load/store latencies still drive simulator timing.
    "l1dEnabled": (0, 1),
    "l1dNumSets": (1, 4096),
    "l1dBlockSizeWords": (1, 1024),
    "l1dHitLatency": (1, 10000),
    "l1dMissPenalty": (0, 10000),
}

app = Flask(__name__, static_folder=None)
run_lock = threading.Lock()


# Serve the static visualizer frontend from the repository.
@app.get("/")
def index():
    return send_from_directory(VISUALIZER_DIR, "index.html")


# Keep asset serving local to visualizer/.
@app.get("/<path:filename>")
def visualizer_file(filename):
    return send_from_directory(VISUALIZER_DIR, filename)


@app.post("/run")
def run_simulation():
    # Receive assembly code and optional branch predictor mode from the UI.
    assembly_code = get_assembly_code()
    predictor = get_predictor()

    validation_error = validate_assembly_code(assembly_code)
    if validation_error:
        return validation_error

    if predictor is False:
        return jsonify({
            "error": "Invalid predictor. Valid options: always-not-taken, always-taken, one-bit, two-bit, gshare.",
        }), 400

    config, config_error = get_architecture_config()
    if config_error:
        return config_error

    simulator_error = validate_simulator_exists()
    if simulator_error:
        return simulator_error

    # Write the submitted assembly to a temporary file for the C++ parser.
    with tempfile.NamedTemporaryFile(
        mode="w",
        suffix=".asm",
        encoding="utf-8",
        delete=False,
        dir="/tmp",
    ) as asm_file:
        asm_file.write(assembly_code)
        asm_path = Path(asm_file.name)

    try:
        with run_lock:
            # Run the local simulator without a shell, preserving the local-only backend model.
            result = run_simulator(asm_path, predictor, config)

            if result.returncode != 0:
                # Return stdout/stderr so the browser can show useful simulator failures.
                return jsonify({
                    "error": "Simulator failed.",
                    "stdout": result.stdout,
                    "stderr": result.stderr,
                }), 500

            # Read the generated trace and return it directly to the visualizer.
            trace = read_trace()

        return jsonify(trace)
    except subprocess.TimeoutExpired:
        return jsonify({"error": "Simulator timed out."}), 504
    except FileNotFoundError:
        return jsonify({"error": "Simulator finished, but trace.json was not generated."}), 500
    except json.JSONDecodeError:
        return jsonify({"error": "Generated trace.json is not valid JSON."}), 500
    finally:
        asm_path.unlink(missing_ok=True)


@app.post("/compare-predictors")
def compare_predictors():
    assembly_code = get_assembly_code()

    validation_error = validate_assembly_code(assembly_code)
    if validation_error:
        return validation_error

    config, config_error = get_architecture_config()
    if config_error:
        return config_error

    simulator_error = validate_simulator_exists()
    if simulator_error:
        return simulator_error

    results = []

    with run_lock:
        for predictor in PREDICTORS:
            results.append(run_predictor_comparison(
                assembly_code,
                predictor,
                config
            ))

    successful_results = [result for result in results if result["error"] is None]
    best_predictor = None

    if successful_results:
        best_predictor = max(
            successful_results,
            key=lambda result: result["accuracy"],
        )["predictor"]

    architecture_config_used = (
        successful_results[0].get("architectureConfig")
        if successful_results
        else config
    )

    return jsonify({
        "program": "custom",
        "bestPredictor": best_predictor,
        "architectureConfig": architecture_config_used,
        "results": results,
    })


def validate_assembly_code(assembly_code: str):
    if not assembly_code.strip():
        return jsonify({"error": "Assembly code is empty."}), 400

    if len(assembly_code.encode("utf-8")) > MAX_ASSEMBLY_BYTES:
        return jsonify({"error": "Assembly code is too large."}), 413

    return None


def validate_simulator_exists():
    if not SIMULATOR_PATH.exists():
        return jsonify({
            "error": "Simulator binary not found. Build it first with CMake.",
            "expectedPath": str(SIMULATOR_PATH),
        }), 500

    return None


def run_simulator(
    asm_path: Path,
    predictor: str | None,
    architecture_config: dict | None = None,
):
    command = [str(SIMULATOR_PATH), str(asm_path)]

    if predictor:
        command.extend(["--predictor", predictor])

    # Architecture overrides are per request:
    # frontend form -> Flask JSON -> --arch-config -> C++ ArchitectureConfig
    # -> trace.json architectureConfig -> frontend display.
    # Omitting the argument preserves simulator defaults for CLI/tests.
    if architecture_config:
        command.extend([
            "--arch-config",
            serialize_architecture_config(architecture_config),
        ])

    return subprocess.run(
        command,
        cwd=BUILD_DIR,
        capture_output=True,
        text=True,
        timeout=RUN_TIMEOUT_SECONDS,
        shell=False,
        check=False,
    )


def read_trace() -> dict:
    if not TRACE_PATH.exists():
        raise FileNotFoundError(TRACE_PATH)

    with TRACE_PATH.open("r", encoding="utf-8") as trace_file:
        return json.load(trace_file)


def run_predictor_comparison(
    assembly_code: str,
    predictor: str,
    architecture_config: dict | None = None,
) -> dict:
    result = {
        "predictor": predictor,
        "correct": 0,
        "total": 0,
        "accuracy": 0.0,
        "branchPredictions": [],
        "predictorState": None,
        "performanceStats": None,
        "architectureConfig": None,
        "error": None,
        "stdout": "",
        "stderr": "",
    }

    with tempfile.NamedTemporaryFile(
        mode="w",
        suffix=".asm",
        encoding="utf-8",
        delete=False,
        dir="/tmp",
    ) as asm_file:
        asm_file.write(assembly_code)
        asm_path = Path(asm_file.name)

    try:
        completed = run_simulator(asm_path, predictor, architecture_config)
    except subprocess.TimeoutExpired:
        result["error"] = "Simulator timed out."
        return result
    finally:
        asm_path.unlink(missing_ok=True)

    result["stdout"] = completed.stdout
    result["stderr"] = completed.stderr

    if completed.returncode != 0:
        result["error"] = "Simulator failed."
        return result

    try:
        trace = read_trace()
    except FileNotFoundError:
        result["error"] = "Simulator finished, but trace.json was not generated."
        return result
    except json.JSONDecodeError:
        result["error"] = "Generated trace.json is not valid JSON."
        return result

    final_cycle = get_final_cycle(trace)
    branch_predictions = get_final_branch_predictions(trace)
    accuracy = compute_accuracy(branch_predictions)

    result["correct"] = accuracy["correct"]
    result["total"] = accuracy["total"]
    result["accuracy"] = accuracy["accuracy"]
    result["branchPredictions"] = branch_predictions
    result["predictorState"] = final_cycle.get("predictorState")
    result["performanceStats"] = trace.get("performanceStats")
    result["architectureConfig"] = trace.get("architectureConfig")
    result.pop("stdout")
    result.pop("stderr")

    return result


def get_final_branch_predictions(trace: dict) -> list[dict]:
    final_cycle = get_final_cycle(trace)
    branch_predictions = final_cycle.get("branchPredictions", [])

    if isinstance(branch_predictions, list):
        return branch_predictions

    return []


def compute_accuracy(branch_predictions: list[dict]) -> dict:
    if not branch_predictions:
        return {
            "correct": 0,
            "total": 0,
            "accuracy": 0.0,
        }

    has_resolved_field = any("branchResolved" in prediction for prediction in branch_predictions)
    counted_predictions = [
        prediction for prediction in branch_predictions
        if not has_resolved_field or prediction.get("branchResolved") is True
    ]
    total = len(counted_predictions)
    correct = sum(
        1 for prediction in counted_predictions
        if prediction.get("predictionCorrect") is True
    )
    accuracy = (100.0 * correct / total) if total > 0 else 0.0

    return {
        "correct": correct,
        "total": total,
        "accuracy": round(accuracy, 2),
    }


def is_prediction_correct(prediction: dict) -> bool:
    if prediction.get("predictionCorrect") is True:
        return True

    result = str(prediction.get("result", "")).strip().lower()
    return result in {"correct", "hit"}


def get_final_cycle(trace: dict) -> dict:
    cycles = trace.get("cycles", [])

    if isinstance(cycles, list) and cycles:
        return cycles[-1]

    return {}


def get_assembly_code() -> str:
    if request.is_json:
        payload = request.get_json(silent=True) or {}
        return str(payload.get("code", payload.get("assembly", "")))

    if "assembly" in request.form:
        return request.form["assembly"]

    return request.get_data(as_text=True) or ""


def get_architecture_config():
    if not request.is_json:
        return None, None

    payload = request.get_json(silent=True) or {}
    raw_config = payload.get("architectureConfig")

    if raw_config is None:
        return None, None

    if not isinstance(raw_config, dict):
        return None, (jsonify({
            "error": "Invalid architecture config: architectureConfig must be an object",
        }), 400)

    # Flask validates the same flat integer object that the frontend sends.
    # The C++ layer validates again before applying values to ArchitectureConfig.
    # Prediction Analysis mode uses this same path for every predictor run.
    config = {}

    for key, value in raw_config.items():
        if key not in ARCHITECTURE_CONFIG_RANGES:
            return None, (jsonify({
                "error": f"Invalid architecture config: unknown key {key}",
            }), 400)

        if key == "l1dEnabled" and isinstance(value, bool):
            normalized_value = int(value)
        elif isinstance(value, bool) or not isinstance(value, int):
            return None, (jsonify({
                "error": f"Invalid architecture config: {key} must be an integer",
            }), 400)
        else:
            normalized_value = value

        min_value, max_value = ARCHITECTURE_CONFIG_RANGES[key]

        if normalized_value < min_value or normalized_value > max_value:
            return None, (jsonify({
                "error": (
                    f"Invalid architecture config: {key} must be between "
                    f"{min_value} and {max_value}"
                ),
            }), 400)

        config[key] = bool(normalized_value) if key == "l1dEnabled" else normalized_value

    return config, None


def serialize_architecture_config(config: dict) -> str:
    return ",".join(
        f"{key}={int(value) if isinstance(value, bool) else value}"
        for key, value in sorted(config.items())
    )


def get_predictor() -> str | bool | None:
    if not request.is_json:
        return None

    payload = request.get_json(silent=True) or {}
    predictor = str(payload.get("predictor", "")).strip()

    if not predictor:
        return None

    aliases = {
        "always-not-taken": "always-not-taken",
        "not-taken": "always-not-taken",
        "always-taken": "always-taken",
        "taken": "always-taken",
        "one-bit": "one-bit",
        "1bit": "one-bit",
        "1-bit": "one-bit",
        "two-bit": "two-bit",
        "2bit": "two-bit",
        "2-bit": "two-bit",
        "gshare": "gshare",
        "g-share": "gshare",
    }

    return aliases.get(predictor, False)


if __name__ == "__main__":
    host = os.environ.get("FLASK_HOST", "127.0.0.1")
    port = int(os.environ.get("FLASK_PORT", "5000"))
    app.run(host=host, port=port, debug=False)
