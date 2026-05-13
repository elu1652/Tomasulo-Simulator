# memory_order_store_then_load.asm
#
# Test: younger load after older store to same address
#
# Correct architectural result:
# R2 should get 7 from the older store.
#
# Depending on current memory model, this may fail because
# the load can execute before the older store commits.
#
# EXPECT_REG R1 7
# EXPECT_REG R2 7
# EXPECT_MEM 8 7

ADDI R1, R0, 7
SD R1, 8(R0)
LD R2, 8(R0)