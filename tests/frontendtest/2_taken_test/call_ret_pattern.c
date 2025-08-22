#include <klib.h>

// Test call -> ret pattern
// This is the most basic 2-taken pattern where a call is immediately followed by a return

void __attribute__ ((noinline)) test_call_ret_pattern(int cnt) {
    int result = 0;
    
    asm volatile (
        "addi sp, sp, -16\n\t"
        "sd ra, 8(sp)\n\t"
        "li t0, 0\n\t"
        ".align 4\n\t"
        "1:\n\t"
        // Call instruction
        "jal ra, 4f\n\t"
        "3:\n\t"
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        "j 5f\n\t"
        // Function that immediately returns (call->ret pattern)
        "4:\n\t"
        "addi %0, %0, 2\n\t"
        "ret\n\t"
        "5:\n\t"
        "ld ra, 8(sp)\n\t"
        "addi sp, sp, 16\n\t"
        : "+r"(result)
        : "r"(cnt)
        : "t0", "ra", "memory"
    );
}

int main() {
    test_call_ret_pattern(10000);
    return 0;
} 