# fibonacci_loop.asm
#
# Test: Fibonacci sequence using a loop, dynamic memory addresses, ROB reuse, RS reuse, CDB wakeups,
#       register renaming, load-store behavior, and branch prediction.
#
# This program stores the first 10 Fibonacci numbers into memory:
#
# Mem[0] = 0
# Mem[1] = 1
# Mem[2] = 1
# Mem[3] = 2
# Mem[4] = 3
# Mem[5] = 5
# Mem[6] = 8
# Mem[7] = 13
# Mem[8] = 21
# Mem[9] = 34
#
# Register usage:
# R1 = previous Fibonacci value
# R2 = current Fibonacci value
# R3 = loop counter
# R4 = next Fibonacci value
# R8 = memory pointer
# R9 = loaded Mem[8]
# R10 = loaded Mem[9]
# R11 = check value, R9 + R10 = 55
#
# Expected final state:
# R1 = 21
# R2 = 34
# R3 = 0
# R4 = 34
# R8 = 10
# R9 = 21
# R10 = 34
# R11 = 55
#
# EXPECT_REG R1 21
# EXPECT_REG R2 34
# EXPECT_REG R3 0
# EXPECT_REG R4 34
# EXPECT_REG R8 10
# EXPECT_REG R9 21
# EXPECT_REG R10 34
# EXPECT_REG R11 55
# EXPECT_MEM 0 0
# EXPECT_MEM 1 1
# EXPECT_MEM 2 1
# EXPECT_MEM 3 2
# EXPECT_MEM 4 3
# EXPECT_MEM 5 5
# EXPECT_MEM 6 8
# EXPECT_MEM 7 13
# EXPECT_MEM 8 21
# EXPECT_MEM 9 34
# EXPECT_COMMIT_COUNT ADD R4, R1, R2 8
# EXPECT_COMMIT_COUNT SD R4, 0(R8) 8
# EXPECT_COMMIT_COUNT BNE R3, R0, fib_loop 8

ADDI R1, R0, 0
ADDI R2, R0, 1
ADDI R8, R0, 0

SD R1, 0(R8)

ADDI R8, R8, 1
SD R2, 0(R8)

ADDI R8, R8, 1
ADDI R3, R0, 8

fib_loop:
ADD R4, R1, R2
SD R4, 0(R8)

ADD R1, R2, R0
ADD R2, R4, R0

ADDI R8, R8, 1
ADDI R3, R3, -1
BNE R3, R0, fib_loop

LD R9, 8(R0)
LD R10, 9(R0)
ADD R11, R9, R10
