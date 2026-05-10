# Tomasulo Simulator

A cycle-based out-of-order CPU simulator written in C++.

This project simulates core ideas from Tomasulo-style dynamic scheduling, including reservation stations, register renaming, functional unit contention, common data bus writeback, and reorder buffer commit.

The goal is to show how instructions move through an out-of-order execution engine cycle by cycle.

---

## Current Features

- Cycle-based Tomasulo-style out-of-order execution simulator
- In-order issue with out-of-order execution and in-order ROB commit
- Reservation stations for integer, multiply, load, and store operations
- Functional unit capacity limits and structural hazard handling
- Register renaming with producer tags and `Vj` / `Vk` / `Qj` / `Qk` operand tracking
- Common Data Bus with single-result broadcast per cycle
- Reorder Buffer with logical capacity, precise in-order commit, and store commit support
- Speculative branch execution with misprediction recovery
- Static, 1-bit, and 2-bit branch predictor modes indexed by static PC
- Flush support for younger wrong-path instructions in RS/ROB/CDB
- Cycle-by-cycle debug output, instruction timing table, and branch prediction summary
- Automated test runner and GUI test-file generator

---

## Supported Instructions

The simulator currently supports:

```asm
ADD R1, R2, R3
ADDI R1, R1, 5
SUB R1, R2, R3
MUL R1, R2, R3
LD  R1, offset(R2)
SD  R1, offset(R2)
BNE R1, R2, loop
BEQ R1, R2, loop
```

Example:

```asm
LD R1, 0(R0)
ADD R2, R1, R3
MUL R4, R2, R5
```

---

## Architecture Overview

The simulator models these main components:

```text
Assembly Program
      |
      v
Parser
      |
      v
Simulator
      |
      +--> Register File
      +--> Memory
      +--> Reservation Stations
      +--> Functional Units
      +--> Common Data Bus
      +--> Reorder Buffer
      +--> Debug/State Printer
      +--> Branch Predictor
      +--> Debug/State Printer
```

Instructions issue in program order, wait in reservation stations until operands and functional units are available, execute when ready, write results through the CDB or ROB, and finally commit in order through the ROB.

---

## Cycle Timing Model

Each simulator cycle currently follows this order:

```text
1. Issue one instruction if possible
2. Execute / decrement active instructions
3. Print debug state
4. Commit one ready ROB head entry
5. Broadcast one old CDB result
6. Complete newly finished instructions
7. Queue new CDB/store results
8. Advance to next cycle
```

This means a register-writing instruction that finishes execution in cycle `N` queues a CDB result at the end of cycle `N`.

It can broadcast in cycle `N + 1`.

It can commit no earlier than cycle `N + 2`, assuming it is at the head of the ROB.

---

## Build

Requirements:

* C++17
* CMake
* g++

On Ubuntu:

```bash
sudo apt update
sudo apt install build-essential cmake
```

Build the project:

```bash
mkdir -p build
cd build
cmake ..
make
```

---

## Run

From the `build/` directory:

```bash
./simulator
```

This runs the default input file configured in `main.cpp`.

To run a specific test file:

```bash
./simulator ../tests/basic_arithmetic.asm
```

## Example Program

```asm
ADD R1, R3, R5
SUB R2, R3, R5
MUL R4, R3, R5
```

This tests arithmetic functions and committing to architectural register.

Expected behavior:

```text
I0: ADD R1, R3, R5 -> R1 = 7
I1: SUB R2, R3, R5 -> R2 = 3
I2: MUL R4, R3, R5 -> R4 = 10
```

---

## Example Debug Output

The simulator prints detailed cycle-by-cycle state, including:

```text
FU State
RS State
Register Producers
Active Instructions
CDB Queue
ROB
ROB Commit
CDB Broadcast
```

Example ROB output:

```text
ROB: 3/4
  I1 | MUL R2, R3, R5 | ready: no
  I2 | ADD R6, R3, R5 | ready: yes | R6 = 7
  I3 | LD R1, 0(R0) | ready: yes | R1 = 99
```

Example issue stall:

```text
ROB: 4/4
Issue stalled: LD R5, 4(R1) | ROB full
```

---

## Test Programs

Example test files are stored in:

```text
tests/
```

These tests cover arithmetic, RAW dependencies, repeated writes to the same register, self-dependencies, CDB contention, out-of-order writeback, ROB capacity stalls, load-use dependencies, store commit behavior, branches, and speculative flush behavior.

Test files can be easily created using the GUI displayed when running:
```bash
python3 tests/create_test.py
```
The GUI asks for:
```text
File name
Test title
Description
Expected registers
Expected memory
Expected commit counts
Assembly code
```

Testing is automated by running:
```bash
python3 tests/run_tests.py
```

Automated testing runs all test files located in `tests/` and verifies expected final register values, memory values, and optional commit-count expectations.
```text
[FAIL] add_immediate.asm
R1: expected 8, got 7
[PASS] backword_branch.asm
```

Expectations are written as:
```asm
# EXPECT_REG R1 5
# EXPECT_MEM 0 99
# EXPECT_COMMIT_COUNT ADD R2, R1, R3 1
```

---

## Current Limitations

* The simulator supports a small custom ISA rather than full RISC-V.
* 1-bit and 2-bit dynamic branch predictors are implemented but predictor type is currently selected in code rather than through a command-line option.
* ROB capacity is logical; physical ROB slots are not reused yet.
* The current ROB tag is the dynamic instruction ID.
* Load-store ordering is simplified.
* There is no full load-store queue yet.
* Automated tests validate final register/memory state and selected commit-count behavior, but they do not yet validate branch prediction accuracy as a correctness requirement.

---

## Planned Features

* Command-line option for selecting predictor type
* More branch-speculation test cases
* True circular ROB with reusable physical slots
* Load-store queue
* CPI and performance experiments

---

## Project Status

The simulator currently implements Tomasulo-style out-of-order execution with ROB-based commit and branch speculation. It includes static, 1-bit, and 2-bit branch predictor modes, misprediction recovery by flushing younger wrong-path instructions, and branch prediction summary output with predictor state and accuracy reporting.