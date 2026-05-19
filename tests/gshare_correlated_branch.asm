# gshare_correlated_branch.asm
#
# Test: repeated branches with global-history predictor
#
# This test is intended to be run with:
# ./build/simulator tests/gshare_correlated_branch.asm --predictor gshare
#
# The program runs a loop 4 times.
# Each loop iteration has two branches:
#
# 1. BEQ R2, R0, skip_add
#    This branch is usually not taken until R2 reaches 0.
#
# 2. BNE R1, R0, loop
#    This branch is taken until the loop counter reaches 0.
#
# This gives GShare repeated branch history to update its GHR and
# pattern-history table.
#
# Architectural correctness should not depend on the predictor.
# Even if GShare mispredicts, wrong-path instructions should be flushed.
#
# Expected final state:
# R1 = 0
# R2 = 0
# R3 = 30
# R4 = 10
# R5 = 1
# R6 = 99
#
# EXPECT_REG R1 0
# EXPECT_REG R2 0
# EXPECT_REG R3 30
# EXPECT_REG R4 10
# EXPECT_REG R5 1
# EXPECT_REG R6 99
# EXPECT_COMMIT_COUNT ADD R3, R3, R4 3
# EXPECT_COMMIT_COUNT BEQ R2, R0, skip_add 4
# EXPECT_COMMIT_COUNT BNE R1, R0, loop 4
# EXPECT_COMMIT_COUNT ADDI R6, R0, 99 1

ADDI R1, R0, 4
ADDI R2, R0, 3
ADDI R3, R0, 0
ADDI R4, R0, 10
ADDI R5, R0, 1

loop:
BEQ R2, R0, skip_add
ADD R3, R3, R4
SUB R2, R2, R5

skip_add:
SUB R1, R1, R5
BNE R1, R0, loop

ADDI R6, R0, 99