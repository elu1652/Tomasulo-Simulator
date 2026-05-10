# Tomasulo Simulator

A cycle-based out-of-order CPU simulator written in C++.

This project simulates core ideas from Tomasulo-style dynamic scheduling, including reservation stations, register renaming, functional unit contention, common data bus writeback, and reorder buffer commit.

The goal is to show how instructions move through an out-of-order execution engine cycle by cycle.

---

## Current Features

- Custom assembly-like input format
- In-order instruction issue
- Out-of-order execution
- Reservation station capacity limits
- Separate integer, multiply, load, and store reservation stations/buffers
- Functional unit contention
- Register renaming with producer tags
- `Vj`, `Vk`, `Qj`, `Qk` operand tracking
- Common Data Bus (CDB)
- Single CDB broadcast per cycle
- Reorder Buffer (ROB)
- Logical ROB capacity
- In-order commit
- Stores commit through the ROB
- Cycle-by-cycle debug output
- Instruction timing table
- BEQ/BNE branch support
- Forward and backward branch tests
- Automated test runner
- Test file generator GUI
- Static always-not-taken branch prediction
- Speculative fall-through issue
- Branch misprediction detection
- PC redirect on taken branch
- Flush younger wrong-path instructions from RS/ROB/CDB
- Register producer cleanup on flush
- Flushed instruction marking in status table

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

These tests cover arithmetic, RAW dependencies, repeated writes to the same register, self-dependencies, CDB contention, out-of-order writeback, ROB capacity stalls, load-use dependencies, and store commit behavior.

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
Assembly code
```

Testing is automated by running:
```bash
python3 tests/run_tests.py
```

Automated testing runs all test files located in `tests/` and verifies resulting values with expected values. Errors are shown in terminal like:
```text
[FAIL] add_immediate.asm
R1: expected 8, got 7
```
Terminal will display the number of passed and failed tests.

---

## Current Limitations

* The simulator supports a small custom ISA rather than full RISC-V.
* Only static always-not-taken branch prediction is implemented.
* No dynamic 1-bit or 2-bit branch predictor yet.
* ROB capacity is logical; physical ROB slots are not reused yet.
* The current ROB tag is the dynamic instruction ID.
* Load-store ordering is simplified.
* There is no full load-store queue yet.
* Automated tests currently mainly validate final register/memory state, so some speculative behavior still needs stronger test checks.

---

## Planned Features

* Stronger automated testing for speculative execution
* Commit-count or flush-count validation in the test runner
* Dynamic branch prediction, such as 1-bit or 2-bit predictors
* True circular ROB with reusable physical slots
* Load-store queue
* CPI and performance experiments

---

## Project Status

The simulator currently implements Tomasulo-style out-of-order execution with ROB-based commit and static always-not-taken branch speculation, including misprediction recovery by flushing younger wrong-path instructions.