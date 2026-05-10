# branch_flush_commit_count.asm
#
# Test: taken forward branch flushes wrong-path fall-through instructions
#
# R1 == R2, so BEQ should be taken.
# The fall-through ADDI instructions should be issued speculatively
# but flushed before commit.
#
# Expected final state:
# EXPECT_REG R1 5
# EXPECT_REG R2 5
# EXPECT_REG R3 0
# EXPECT_REG R4 0
# EXPECT_REG R5 10
# EXPECT_COMMIT_COUNT ADDI R3, R0, 111 0
# EXPECT_COMMIT_COUNT ADDI R4, R0, 222 0
# EXPECT_COMMIT_COUNT ADDI R5, R0, 10 1

ADDI R1, R0, 5
ADDI R2, R0, 5
BEQ R1, R2, target
ADDI R3, R0, 111
ADDI R4, R0, 222
target:
ADDI R5, R0, 10
