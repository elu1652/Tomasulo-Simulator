# cache_load_stall.asm
#
# Test: loads to the same cache block while the first miss is still pending
#
# The simulator uses word-addressed memory. With an 8-set direct-mapped
# cache and 4-word blocks:
#
# address 0 -> block 0 -> set 0, tag 0
# address 1 -> block 0 -> set 0, tag 0
#
# The first load to address 0 misses and starts filling the cache block.
# The second load to address 1 targets the same cache block.
#
# In the intended blocking-cache teaching model, the second load should not
# treat the line as a normal hit before the first miss fill completes.
# It should wait until the block is available, then complete using the filled line.
#
# This test is intended to catch the bug where a miss immediately marks the
# line valid/tagged, causing a younger load to the same block to hit too early.
#
# Expected final state:
# R1 = 10
# R2 = 20
# R3 = 30
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
#
# Expected cache behavior after fixing pending-fill handling:
# EXPECT_L1D_ACCESSES 2
# EXPECT_L1D_HITS 1
# EXPECT_L1D_MISSES 1
# EXPECT_L1D_WRITEBACKS 0
# EXPECT_MEMORY_STALL_CYCLES 10
# EXPECT_EXEC_END_AFTER LD R2, 1(R10) AFTER LD R1, 0(R10)

.REG R10 0

.MEM 0 10
.MEM 1 20

LD R1, 0(R10)
LD R2, 1(R10)

ADD R3, R1, R2
