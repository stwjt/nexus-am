#include <klib.h>

#define ADDR_BASE 0x80400000

#define BYTE (1)
#define KB (1024 * BYTE)
#define MB (1024 * KB)

#define CACHE_LINE_SIZE (64 * BYTE)
// #define DCACHE_SIZE (64 * KB)
#define L2_CACHE_SIZE (1 * MB)
#define ITERATION (8)
#define ACCESS_NUM (ITERATION * L2_CACHE_SIZE / CACHE_LINE_SIZE)


int main() {

  uint64_t addr = ADDR_BASE;
#define STEP 8
  assert(ACCESS_NUM % STEP == 0);
#pragma GCC unroll 8

  for (uint64_t i = 0; i < ACCESS_NUM; i++) {
    uint64_t *restrict ptr = (uint64_t *)addr;
    *ptr = i;
    addr += CACHE_LINE_SIZE;
  }

  return 0;
}