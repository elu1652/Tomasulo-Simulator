# full_system_demo.asm
#
# Test: full processor demo
#
# This program is designed to exercise most implemented simulator features:
#
# - ADDI
# - ADD
# - SUB
# - MUL
# - LD
# - SD
# - BEQ
# - BNE
# - loops
# - branch prediction
# - wrong-path flush
# - ROB reuse
# - reservation station reuse
# - CDB wakeups
# - register renaming
# - store commit through ROB
# - LSQ same-address store-to-load behavior
# - LSQ computed-address matching
# - LSQ different-address load bypass
#
# Part 1:
# Stores the first 10 Fibonacci numbers:
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
# Then loads Mem[8] and Mem[9]:
# R11 = 21 + 34 = 55
#
# Part 2:
# Tests LSQ forwarding and computed address matching:
# R14 = 6 * 7 = 42
# Store R14 to Mem[12]
# Load Mem[12] into R15
#
# Then:
# R16 = 8
# Store 77 to 4(R16), which is also Mem[12]
# Load Mem[12] into R18
#
# Part 3:
# Tests different-address load bypass:
# R21 = 2 * 5 = 10
# Store R21 to Mem[14]
# Load Mem[15], which should remain 0
#
# Part 4:
# Tests branch flush:
# A taken BEQ skips wrong-path instructions that try to write R28 and Mem[15].
# R28 should remain 0.
# Mem[15] should remain 0.
#
# Final checksum:
# R30 = R11 + R15 + R18 + R22
# R30 = 55 + 42 + 77 + 0 = 174
#
# R31 = R29 + R5
# R31 = 123 + 1 = 124
#
# EXPECT_REG R1 21
# EXPECT_REG R2 34
# EXPECT_REG R3 0
# EXPECT_REG R4 34
# EXPECT_REG R5 1
# EXPECT_REG R8 10
# EXPECT_REG R9 21
# EXPECT_REG R10 34
# EXPECT_REG R11 55
# EXPECT_REG R12 6
# EXPECT_REG R13 7
# EXPECT_REG R14 42
# EXPECT_REG R15 42
# EXPECT_REG R16 8
# EXPECT_REG R17 77
# EXPECT_REG R18 77
# EXPECT_REG R19 2
# EXPECT_REG R20 5
# EXPECT_REG R21 10
# EXPECT_REG R22 0
# EXPECT_REG R28 0
# EXPECT_REG R29 123
# EXPECT_REG R30 174
# EXPECT_REG R31 124
#
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
# EXPECT_MEM 12 77
# EXPECT_MEM 14 10
# EXPECT_MEM 15 0
#
# EXPECT_COMMIT_COUNT ADD R4, R1, R2 8
# EXPECT_COMMIT_COUNT SD R4, 0(R8) 8
# EXPECT_COMMIT_COUNT BNE R3, R0, fib_loop 8
# EXPECT_COMMIT_COUNT ADDI R28, R0, 999 0
# EXPECT_COMMIT_COUNT SD R28, 15(R0) 0

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

ADDI R12, R0, 6
ADDI R13, R0, 7
MUL R14, R12, R13

SD R14, 12(R0)
LD R15, 12(R0)

ADDI R16, R0, 8
ADDI R17, R0, 77

SD R17, 4(R16)
LD R18, 12(R0)

ADDI R19, R0, 2
ADDI R20, R0, 5
MUL R21, R19, R20

SD R21, 14(R0)
LD R22, 15(R0)

ADDI R5, R0, 1

BEQ R5, R5, correct_path

ADDI R28, R0, 999
SD R28, 15(R0)

correct_path:
BNE R5, R5, after_not_taken

ADDI R29, R0, 123

after_not_taken:
ADD R30, R11, R15
ADD R30, R30, R18
ADD R30, R30, R22

ADD R31, R29, R5