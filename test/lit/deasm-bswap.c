// (This test actually uses 'Inputs/deasm-bswap.ll')
// hardeningDisable=all clang deasm-bswap.c -o Inputs/deasm-bswap.ll -O2 -emit-llvm -S

// Check we can create an allexe and run it:
// RUN: bc2allvm %p/Inputs/deasm-bswap.ll -f -o %t
// RUN: %t

// Check all asm has been removed:
// RUN: allopt -analyze -i %t llvm-dis|& FileCheck %s

// CHECK: llvm.bswap
// CHECK-NOT: call {{.*}} asm

#include <stdint.h>
#include <stdio.h>

uint32_t call_bswap(uint32_t arg) {
  uint32_t result;
  asm ("bswap %0" : "=r"(result) : "r"(arg));
  return result;
}
uint64_t call_bswap64(uint64_t arg) {
  uint64_t result;
  asm ("bswap %0" : "=r"(result) : "r"(arg));
  return result;
}
uint32_t call_bswapl(uint32_t arg) {
  uint32_t result;
  asm ("bswapl %0" : "=r"(result) : "r"(arg));
  return result;
}
uint64_t call_bswapq(uint64_t arg) {
  uint64_t result;
  asm ("bswapq %0" : "=r"(result) : "r"(arg));
  return result;
}

int main() {
#define TEST(expr, out) \
  do { \
    unsigned long long val = (unsigned long long)(expr); \
    if (val != out) { \
      printf(#expr " returned %lld, expected %lld\n", val, \
          (unsigned long long)out); \
      return 1; \
    } \
  } while (0)

  TEST(call_bswap(0x12345678), 0x78563412);
  TEST(call_bswapl(0x12345678), 0x78563412);
  TEST(call_bswapq(0xabcdef0012345678ull), 0x7856341200efcdabull);
  TEST(call_bswap64(0xabcdef0012345678ull), 0x7856341200efcdabull);
  return 0;
}
