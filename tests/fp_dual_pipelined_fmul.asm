# fp_dual_pipelined_fmul.asm
#
# Test: two FP_MUL pipelines can start two ready FMULs in the same cycle
#
# R5 is produced by a long-latency FMUL.
# Younger FMULs issue while waiting for R5.
# When R5 broadcasts, multiple FMULs wake up together.
#
# Expected final state:
# R1 = 2
# R2 = 3
# R3 = 4
# R4 = 5
# R5 = 6
# R10 = 12
# R11 = 18
# R12 = 24
# R13 = 30
#
# EXPECT_REG R1 2
# EXPECT_REG R2 3
# EXPECT_REG R3 4
# EXPECT_REG R4 5
# EXPECT_REG R5 6
# EXPECT_REG R10 12
# EXPECT_REG R11 18
# EXPECT_REG R12 24
# EXPECT_REG R13 30
# EXPECT_COMMIT_COUNT FMUL R5, R1, R2 1
# EXPECT_COMMIT_COUNT FMUL R10, R5, R1 1
# EXPECT_COMMIT_COUNT FMUL R11, R5, R2 1
# EXPECT_COMMIT_COUNT FMUL R12, R5, R3 1
# EXPECT_COMMIT_COUNT FMUL R13, R5, R4 1

ADDI R1, R0, 2
ADDI R2, R0, 3
ADDI R3, R0, 4
ADDI R4, R0, 5

FMUL R5, R1, R2

FMUL R10, R5, R1
FMUL R11, R5, R2
FMUL R12, R5, R3
FMUL R13, R5, R4