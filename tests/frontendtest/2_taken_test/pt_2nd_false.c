#include <klib.h>

// Test pt_2nd=false case
// This is the most basic pattern where the first branch is taken and the second FB is fallthrough (no branch)

void __attribute__ ((noinline)) test_pt_2nd_false_case(int cnt) {
    int result = 0;
    
    asm volatile (
        "li t0, 0\n\t"
        ".align 4\n\t"
        "1:\n\t"
        // First taken branch
        "j 2f\n\t"
        "nop\n\t"
        "2:\n\t"
        // Sequential instructions (no branch - pt_2nd should be false)
        "addi %0, %0, 1\n\t"
        "addi %0, %0, 2\n\t"
        "addi %0, %0, 3\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        : "+r"(result)
        : "r"(cnt)
        : "t0", "memory"
    );
}

int main() {
    test_pt_2nd_false_case(10000);
    return 0;
} 