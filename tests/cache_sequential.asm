# cache_sequential.asm
#
# Test: spatial locality in the optional direct-mapped L1D cache
#
# The simulator uses word-addressed memory. With an 8-set direct-mapped
# cache and 4-word blocks:
#
# address 0 -> block 0 -> set 0, tag 0
# address 1 -> block 0 -> set 0, tag 0
# address 2 -> block 0 -> set 0, tag 0
# address 3 -> block 0 -> set 0, tag 0
#
# The first load misses and allocates the block. The following loads hit
# because they access other words in the same cache block.
#
# Expected cache behavior:
# LD 0 -> miss
# LD 1 -> hit
# LD 2 -> hit
# LD 3 -> hit
#
# Expected final state:
# R1 = 10
# R2 = 20
# R3 = 30
# R4 = 40
# R5 = 30
# R6 = 70
# R7 = 100
#
# ARCH_L1D_ENABLED true
# ARCH_L1D_NUM_SETS 8
# ARCH_L1D_BLOCK_SIZE_WORDS 4
# ARCH_L1D_HIT_LATENCY 1
# ARCH_L1D_MISS_PENALTY 10
#
# EXPECT_REG R1 10
# EXPECT_REG R2 20
# EXPECT_REG R3 30
# EXPECT_REG R4 40
# EXPECT_REG R5 30
# EXPECT_REG R6 70
# EXPECT_REG R7 100
# EXPECT_L1D_ACCESSES 4
# EXPECT_L1D_HITS 3
# EXPECT_L1D_MISSES 1
# EXPECT_L1D_WRITEBACKS 0
# EXPECT_MEMORY_STALL_CYCLES 10

.REG R10 0

.MEM 0 10
.MEM 1 20
.MEM 2 30
.MEM 3 40

LD R1, 0(R10)
LD R2, 1(R10)
LD R3, 2(R10)
LD R4, 3(R10)

ADD R5, R1, R2
ADD R6, R3, R4
ADD R7, R5, R6