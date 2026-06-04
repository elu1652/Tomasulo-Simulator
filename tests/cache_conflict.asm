# cache_conflict.asm
#
# Test: direct-mapped L1D cache conflict misses
#
# The simulator uses word-addressed memory. With a 2-set direct-mapped
# cache and 4-word blocks:
#
# address 0 -> block 0 -> set 0, tag 0
# address 8 -> block 2 -> set 0, tag 1
# address 0 -> block 0 -> set 0, tag 0
#
# The second access evicts the first block because both blocks map to set 0.
# The third access to address 0 misses again.
#
# Expected cache behavior:
# LD 0 -> miss
# LD 8 -> miss
# LD 0 -> miss
#
# Expected final state:
# R1 = 10
# R2 = 20
# R3 = 10
# R4 = 30
# R5 = 40
#
# ARCH_L1D_ENABLED true
# ARCH_L1D_NUM_SETS 2
# ARCH_L1D_BLOCK_SIZE_WORDS 4
# ARCH_L1D_HIT_LATENCY 1
# ARCH_L1D_MISS_PENALTY 10
#
# EXPECT_REG R1 10
# EXPECT_REG R2 20
# EXPECT_REG R3 10
# EXPECT_REG R4 30
# EXPECT_REG R5 40
# EXPECT_L1D_ACCESSES 3
# EXPECT_L1D_HITS 0
# EXPECT_L1D_MISSES 3
# EXPECT_L1D_WRITEBACKS 0
# EXPECT_MEMORY_STALL_CYCLES 30

.REG R10 0
.REG R11 8

.MEM 0 10
.MEM 8 20

LD R1, 0(R10)
LD R2, 0(R11)
LD R3, 0(R10)

ADD R4, R1, R2
ADD R5, R4, R3
