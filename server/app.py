from __future__ import annotations

import json
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

app = Flask(__name__, static_folder=None)
run_lock = threading.Lock()


@app.get("/")
def index():
    return send_from_directory(VISUALIZER_DIR, "index.html")


@app.get("/<path:filename>")
def visualizer_file(filename):
    return send_from_directory(VISUALIZER_DIR, filename)


@app.post("/run")
def run_simulation():
    assembly_code = get_assembly_code()
    predictor = get_predictor()

    if not assembly_code.strip():
        return jsonify({"error": "Assembly code is empty."}), 400

    if predictor is False:
        return jsonify({
            "error": "Invalid predictor. Valid options: always-not-taken, always-taken, one-bit, two-bit.",
        }), 400

    if len(assembly_code.encode("utf-8")) > MAX_ASSEMBLY_BYTES:
        return jsonify({"error": "Assembly code is too large."}), 413

    if not SIMULATOR_PATH.exists():
        return jsonify({
            "error": "Simulator binary not found. Build it first with CMake.",
            "expectedPath": str(SIMULATOR_PATH),
        }), 500

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
            command = [str(SIMULATOR_PATH), str(asm_path)]
            if predictor:
                command.extend(["--predictor", predictor])

            result = subprocess.run(
                command,
                cwd=BUILD_DIR,
                capture_output=True,
                text=True,
                timeout=RUN_TIMEOUT_SECONDS,
                shell=False,
                check=False,
            )

            if result.returncode != 0:
                return jsonify({
                    "error": "Simulator failed.",
                    "stdout": result.stdout,
                    "stderr": result.stderr,
                }), 500

            if not TRACE_PATH.exists():
                return jsonify({
                    "error": "Simulator finished, but trace.json was not generated.",
                    "stdout": result.stdout,
                    "stderr": result.stderr,
                }), 500

            with TRACE_PATH.open("r", encoding="utf-8") as trace_file:
                trace = json.load(trace_file)

        return jsonify(trace)
    except subprocess.TimeoutExpired:
        return jsonify({"error": "Simulator timed out."}), 504
    except json.JSONDecodeError:
        return jsonify({"error": "Generated trace.json is not valid JSON."}), 500
    finally:
        asm_path.unlink(missing_ok=True)


def get_assembly_code() -> str:
    if request.is_json:
        payload = request.get_json(silent=True) or {}
        return str(payload.get("code", payload.get("assembly", "")))

    if "assembly" in request.form:
        return request.form["assembly"]

    return request.get_data(as_text=True) or ""


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
    }

    return aliases.get(predictor, False)


if __name__ == "__main__":
    app.run(host="127.0.0.1", port=5000, debug=False)
