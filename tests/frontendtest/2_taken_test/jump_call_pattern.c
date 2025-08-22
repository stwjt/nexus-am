#include <klib.h>

// Test direct jump -> call pattern
// This is the most basic 2-taken pattern where a direct jump is followed by a call

void __attribute__ ((noinline)) test_jump_call_pattern(int cnt) {
    int result = 0;
    
    asm volatile (
        "li t0, 0\n\t"
        ".align 4\n\t"
        "1:\n\t"
        // First direct jump
        "j 2f\n\t"
        "nop\n\t"
        "2:\n\t"
        // Call instruction (should be predicted as 2-taken)
        "jal ra, 4f\n\t"
        "3:\n\t"
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        "j 5f\n\t"
        // Simple function
        "4:\n\t"
        "addi %0, %0, 2\n\t"
        "ret\n\t"
        "5:\n\t"
        : "+r"(result)
        : "r"(cnt)
        : "t0", "ra", "memory"
    );
}

int main() {
    test_jump_call_pattern(10000);
    return 0;
} 