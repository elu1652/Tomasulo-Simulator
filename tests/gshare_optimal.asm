# gshare_optimal.asm
#
# Test: branch pattern designed to favor GShare
#
# This program creates repeated correlated branch behavior.
#
# There are two main branch sites per loop iteration:
#
#   Branch A: BNE R1, R0, branch_a_taken
#   Branch B: BNE R1, R0, branch_b_taken
#
# R1 toggles every loop iteration:
#
#   R1 pattern: 1, 0, 1, 0, 1, 0, ...
#
# Therefore each static branch alternates:
#
#   Branch A: T N T N T N ...
#   Branch B: T N T N T N ...
#
# This is hard for local 1-bit and 2-bit predictors because each branch's
# local behavior alternates.
#
# GShare should do better because the recent global branch history helps
# separate the taken and not-taken contexts.
#
# The loop runs 32 iterations to give GShare enough time to train.
#
# Expected final state:
# R1 = 1
# R2 = 0
# R5 = 1
# R6 = 32
# R9 = 16
# R10 = 16
# R11 = 16
# R12 = 16
#
# EXPECT_REG R1 1
# EXPECT_REG R2 0
# EXPECT_REG R5 1
# EXPECT_REG R6 32
# EXPECT_REG R9 16
# EXPECT_REG R10 16
# EXPECT_REG R11 16
# EXPECT_REG R12 16
# EXPECT_COMMIT_COUNT BNE R1, R0, branch_a_taken 32
# EXPECT_COMMIT_COUNT BNE R1, R0, branch_b_taken 32
# EXPECT_COMMIT_COUNT BNE R2, R0, loop 32
# EXPECT_COMMIT_COUNT ADDI R9, R9, 1 16
# EXPECT_COMMIT_COUNT ADDI R10, R10, 1 16
# EXPECT_COMMIT_COUNT ADDI R11, R11, 1 16
# EXPECT_COMMIT_COUNT ADDI R12, R12, 1 16

ADDI R1, R0, 1
ADDI R2, R0, 32
ADDI R5, R0, 1
ADDI R6, R0, 32

ADDI R9, R0, 0
ADDI R10, R0, 0
ADDI R11, R0, 0
ADDI R12, R0, 0

loop:
BNE R1, R0, branch_a_taken

branch_a_not_taken:
ADDI R10, R10, 1
BEQ R0, R0, after_branch_a

branch_a_taken:
ADDI R9, R9, 1

after_branch_a:
BNE R1, R0, branch_b_taken

branch_b_not_taken:
ADDI R12, R12, 1
BEQ R0, R0, after_branch_b

branch_b_taken:
ADDI R11, R11, 1

after_branch_b:
SUB R1, R5, R1
SUB R2, R2, R5
BNE R2, R0, loop