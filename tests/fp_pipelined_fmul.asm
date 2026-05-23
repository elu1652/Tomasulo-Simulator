# fp_pipelined_fmul.asm
#
# Test: pipelined FP_MUL can accept one FMUL per cycle
#
# With a non-pipelined FP_MUL unit, these FMUL instructions would have to wait
# for the previous FMUL to finish.
#
# With a pipelined FP_MUL unit, multiple FMUL instructions can be in flight at
# once, each in a different pipeline stage.
#
# Expected final state:
# R1 = 2
# R2 = 3
# R3 = 4
# R4 = 5
# R10 = 6
# R11 = 12
# R12 = 20
# R13 = 10
#
# EXPECT_REG R1 2
# EXPECT_REG R2 3
# EXPECT_REG R3 4
# EXPECT_REG R4 5
# EXPECT_REG R10 6
# EXPECT_REG R11 12
# EXPECT_REG R12 20
# EXPECT_REG R13 10
# EXPECT_COMMIT_COUNT FMUL R10, R1, R2 1
# EXPECT_COMMIT_COUNT FMUL R11, R2, R3 1
# EXPECT_COMMIT_COUNT FMUL R12, R3, R4 1
# EXPECT_COMMIT_COUNT FMUL R13, R1, R4 1

.REG R1 2
.REG R2 3
.REG R3 4
.REG R4 5

FMUL R10, R1, R2
FMUL R11, R2, R3
FMUL R12, R3, R4
FMUL R13, R1, R4
