# fp_independent_units.asm
#
# Test: separate FP_ADD and FP_MUL units
#
# This test checks that FADD/FSUB use the FP_ADD reservation station/FU,
# while FMUL/FDIV use the FP_MUL reservation station/FU.
#
# FADD and FMUL are independent and should be able to execute at the same time
# if both operands are ready and both FP units are free.
#
# Expected final state:
# R1 = 10
# R2 = 20
# R3 = 30
# R4 = 200
# R5 = 10
# R6 = 2
# R7 = 230
#
# EXPECT_REG R1 10
# EXPECT_REG R2 20
# EXPECT_REG R3 30
# EXPECT_REG R4 200
# EXPECT_REG R5 10
# EXPECT_REG R6 2
# EXPECT_REG R7 230
# EXPECT_COMMIT_COUNT FADD R3, R1, R2 1
# EXPECT_COMMIT_COUNT FMUL R4, R1, R2 1
# EXPECT_COMMIT_COUNT FSUB R5, R2, R1 1
# EXPECT_COMMIT_COUNT FDIV R6, R2, R1 1
# EXPECT_COMMIT_COUNT ADD R7, R3, R4 1

.REG R1 10
.REG R2 20

FADD R3, R1, R2
FMUL R4, R1, R2

FSUB R5, R2, R1
FDIV R6, R2, R1

ADD R7, R3, R4
