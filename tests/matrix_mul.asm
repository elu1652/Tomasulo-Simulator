# tests/final_dynamic_matvec_2iter.asm
#
# Based on final exam dynamic scheduling table.
#
# Functional units expected for close matching:
#   Integer ALU: 1 cycle, 1 unit
#   FP Adder: 4 cycles, 1 pipelined unit
#   FP Multiplier: 7 cycles, 1 pipelined unit
#   Load/Store: 2 cycles, 1 unit
#
# Simulator mapping:
#   a0 -> R10
#   a1 -> R11
#   t3 -> R13
#   f0 -> R1
#   f1 -> R2
#   f2 -> R3
#   f3 -> R4
#
# Use always-taken predictor and R13 = 3 if you want the first two
# loop branches to behave like perfect prediction.
#
# Expected after two completed iterations if memory has:
#   MEM[0]  = 10
#   MEM[4]  = 20
#   MEM[8]  = 30
#   MEM[12] = 40
#
# Then:
#   Iteration 1: R3 = 10 * 20 = 200, R4 = 200
#   Iteration 2: R3 = 30 * 40 = 1200, R4 = 1400
#
# EXPECT_REG R10 8
# EXPECT_REG R11 12
# EXPECT_REG R13 1
# EXPECT_REG R3 1200
# EXPECT_REG R4 1400
# EXPECT_COMMIT_COUNT LD R1, 0(R10) 2
# EXPECT_COMMIT_COUNT LD R2, 0(R11) 2
# EXPECT_COMMIT_COUNT FMUL R3, R1, R2 2
# EXPECT_COMMIT_COUNT FADD R4, R4, R3 2
# EXPECT_COMMIT_COUNT ADDI R10, R10, 4 2
# EXPECT_COMMIT_COUNT ADDI R11, R11, 4 2
# EXPECT_COMMIT_COUNT ADDI R13, R13, -1 2
# EXPECT_COMMIT_COUNT BNE R13, R0, loop 2

ADDI R10, R0, 0      # a0 = base of A
ADDI R11, R0, 4      # a1 = base of B
ADDI R13, R0, 3      # t3 = 3 so first two loop branches are taken
ADDI R4, R0, 0       # f3 accumulator = 0

loop:
LD R1, 0(R10)        # flw f0, 0(a0)
LD R2, 0(R11)        # flw f1, 0(a1)
FMUL R3, R1, R2      # fmul.s f2, f0, f1
FADD R4, R4, R3      # fadd.s f3, f3, f2

ADDI R10, R10, 4     # addi a0, a0, 4
ADDI R11, R11, 4     # addi a1, a1, 4
ADDI R13, R13, -1    # addi t3, t3, -1
BNE R13, R0, loop    # bne t3, zero, loop