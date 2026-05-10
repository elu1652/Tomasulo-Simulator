# cdb_contention.asm
#
# Test: CDB contention with independent ADD instructions
#
# Multiple instructions may finish close together.
# Only one result should broadcast on the CDB per cycle.
#
# Setup:
# R3 = 5
# R5 = 2
#
# Expected final state:
# EXPECT_REG R1 7
# EXPECT_REG R2 7
# EXPECT_REG R4 7

ADDI R3, R0, 5
ADDI R5, R0, 2

ADD R1, R3, R5
ADD R2, R3, R5
ADD R4, R3, R5