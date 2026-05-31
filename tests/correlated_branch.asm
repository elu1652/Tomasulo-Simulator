# correlated_branch.asm
#
# Branch predictor with less taken bias.
#
# The goal is to reduce always-taken advantage by avoiding unconditional
# BEQ join branches.
#
# R1 toggles every loop iteration:
#   1, 0, 1, 0, ...
#
# Several branch sites depend on R1 or its inverse.
# Each branch is roughly 50% taken and 50% not taken.
#
# This should make always-taken much less dominant and gives GShare
# a better chance to use global history.
#
# EXPECT_REG R1 1
# EXPECT_REG R2 0
# EXPECT_REG R5 1
# EXPECT_REG R9 16
# EXPECT_REG R10 16
# EXPECT_REG R11 16
# EXPECT_REG R12 16
#
# EXPECT_COMMIT_COUNT BNE R1, R0, skip_a 32
# EXPECT_COMMIT_COUNT BEQ R1, R0, skip_b 32
# EXPECT_COMMIT_COUNT BNE R1, R0, skip_c 32
# EXPECT_COMMIT_COUNT BEQ R1, R0, skip_d 32
# EXPECT_COMMIT_COUNT BNE R2, R0, loop 32

.REG R1 1       # toggling condition
.REG R2 32      # loop count
.REG R5 1       # constant 1

.REG R9 0
.REG R10 0
.REG R11 0
.REG R12 0

loop:
# Branch A: taken when R1 = 1
# If branch is not taken, increment R9.
BNE R1, R0, skip_a
ADDI R9, R9, 1
skip_a:

# Branch B: taken when R1 = 0
# If branch is not taken, increment R10.
BEQ R1, R0, skip_b
ADDI R10, R10, 1
skip_b:

# Branch C: same direction as A
# If branch is not taken, increment R11.
BNE R1, R0, skip_c
ADDI R11, R11, 1
skip_c:

# Branch D: same direction as B
# If branch is not taken, increment R12.
BEQ R1, R0, skip_d
ADDI R12, R12, 1
skip_d:

# Toggle R1: 1 -> 0, 0 -> 1
SUB R1, R5, R1

# Loop counter
SUB R2, R2, R5
BNE R2, R0, loop