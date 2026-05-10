# branch_not_taken_commit_count.asm
#
# Test: not-taken branch keeps fall-through instructions
#
# R1 != R2, so BEQ should not be taken.
# Since the predictor is always-not-taken, this should be predicted correctly.
# The fall-through ADDI instructions should commit normally.
# The target instruction should also eventually execute after fall-through reaches it.
#
# Expected final state:
# EXPECT_REG R1 5
# EXPECT_REG R2 6
# EXPECT_REG R3 111
# EXPECT_REG R4 222
# EXPECT_REG R5 10
# EXPECT_COMMIT_COUNT ADDI R3, R0, 111 1
# EXPECT_COMMIT_COUNT ADDI R4, R0, 222 1
# EXPECT_COMMIT_COUNT ADDI R5, R0, 10 1
# EXPECT_COMMIT_COUNT BEQ R1, R2, target 1

ADDI R1, R0, 5
ADDI R2, R0, 6
BEQ R1, R2, target
ADDI R3, R0, 111
ADDI R4, R0, 222
target:
ADDI R5, R0, 10
