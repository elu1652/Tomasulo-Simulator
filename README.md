# Tomasulo Simulator

A cycle-accurate out-of-order CPU simulator written in C++.

The simulator models concepts from modern computer architecture including:
- Tomasulo scheduling
- Reservation stations
- Register renaming
- Reorder buffer (ROB)
- Speculative execution
- Branch prediction

## Requirements

- Ubuntu 22.04
- g++
- cmake

Install dependencies:

```bash
sudo apt update
sudo apt install build-essential cmake
```

## Run

```
mkdir -p build
cd build
cmake ..
make
```