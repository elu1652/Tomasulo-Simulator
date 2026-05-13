# lsq_memory_order.asm
#
# Test: LSQ address-aware load execution and store-to-load forwarding
#
# This test intentionally uses MUL to make store operands take longer,
# so LSQ stalls/forwarding are visible in the cycle trace.
#
# Part 1: same-address store/load forwarding
# R1 = 3 * 4 = 12
# SD R1, 8(R0)
# LD R2, 8(R0)
# R2 should forward 12 from the older store.
#
# Part 2: different-address bypass
# R3 = 2 * 5 = 10
# SD R3, 12(R0)
# LD R4, 16(R0)
# R4 should read old Mem[16] = 0, not wait for store commit once address is known.
#
# Part 3: computed-address match, not textual match
# R6 = 2 * 2 = 4
# R7 = 11
# SD R7, 4(R6) writes to address 8
# LD R8, 8(R0) should forward 11.
#
# EXPECT_REG R1 12
# EXPECT_REG R2 12
# EXPECT_REG R3 10
# EXPECT_REG R4 0
# EXPECT_REG R6 4
# EXPECT_REG R7 11
# EXPECT_REG R8 11
# EXPECT_MEM 8 11
# EXPECT_MEM 12 10

# Part 1: same-address store/load forwarding with delayed store value
ADDI R9, R0, 3
ADDI R10, R0, 4
MUL R1, R9, R10
SD R1, 8(R0)
LD R2, 8(R0)

# Part 2: different-address bypass with delayed store value
ADDI R11, R0, 2
ADDI R12, R0, 5
MUL R3, R11, R12
SD R3, 12(R0)
LD R4, 16(R0)

# Part 3: computed-address match with delayed base address
ADDI R13, R0, 2
MUL R6, R13, R13
ADDI R7, R0, 11
SD R7, 4(R6)
LD R8, 8(R0)