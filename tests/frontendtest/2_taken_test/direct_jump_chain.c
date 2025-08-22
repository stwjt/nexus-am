#include <klib.h>

// Test direct jump -> direct jump pattern
// This is the most basic 2-taken pattern where two consecutive
// direct jumps should be predicted together in a single cycle

void __attribute__ ((noinline)) test_direct_jump_chain(int cnt) {
    int result = 0;
    
    // Test direct jump -> direct jump pattern
    asm volatile (
        "li t0, 0\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // First direct jump
        "j 2f\n\t"
        "nop\n\t"
        "2:\n\t"
        
        // Second direct jump (should be predicted as 2-taken)
        "j 3f\n\t"
        "nop\n\t"
        "3:\n\t"
        
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        
        : "+r"(result)
        : "r"(cnt)
        : "t0", "memory"
    );
}

int main() {
    test_direct_jump_chain(10000);
    return 0;
} 