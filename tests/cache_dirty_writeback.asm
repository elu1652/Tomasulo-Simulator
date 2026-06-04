# cache_dirty_writeback.asm
#
# Test: write-back dirty eviction in the optional L1D cache
#
# The simulator uses word-addressed memory. With a 2-set direct-mapped
# cache and 4-word blocks:
#
# address 0 -> block 0 -> set 0, tag 0
# address 8 -> block 2 -> set 0, tag 1
#
# The store to address 0 misses, allocates set 0/tag 0, and marks the line dirty.
# The later load from address 8 maps to the same set with a different tag, so it
# evicts the dirty line and increments the writeback counter.
#
# Important: stores still update architectural memory only at ROB commit. This
# test validates cache metadata/stat behavior, not byte-accurate cache storage.
#
# Expected cache behavior:
# SD 0 -> miss, dirty line allocated
# LD 8 -> miss, dirty line evicted, writeback counted
#
# Expected final state:
# Mem[0] = 99 after the store commits
# R2 = 20
#
# ARCH_L1D_ENABLED true
# ARCH_L1D_NUM_SETS 2
# ARCH_L1D_BLOCK_SIZE_WORDS 4
# ARCH_L1D_HIT_LATENCY 1
# ARCH_L1D_MISS_PENALTY 10
#
# EXPECT_MEM 0 99
# EXPECT_REG R2 20
# EXPECT_L1D_ACCESSES 2
# EXPECT_L1D_HITS 0
# EXPECT_L1D_MISSES 2
# EXPECT_L1D_WRITEBACKS 1
# EXPECT_MEMORY_STALL_CYCLES 20

.REG R10 0
.REG R11 32
.REG R1 99

.MEM 0 10
.MEM 32 20

SD R1, 0(R10)
LD R2, 0(R11)