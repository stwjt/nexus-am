#include "../tests/common.h"
#include <klib.h>

// Test invalid two-taken combinations that should fall back to single prediction
// According to spec, these combinations are NOT supported:
// - Multi-target indirect -> any
// - ret -> ret (RAS dual read ports)
// - any -> multi-target indirect (ittage timing)
// - any -> conditional (no 2-taken TAGE support)
// - call -> call (RAS dual write ports)

// Simulated multi-target indirect jump using different targets
void __attribute__ ((noinline)) test_indirect_first_invalid(int cnt) {
    int result = 0;
    volatile void* targets[4];
    
    // Setup jump targets
    asm volatile ("la %0, 2f" : "=r"(targets[0]));
    asm volatile ("la %0, 3f" : "=r"(targets[1]));
    asm volatile ("la %0, 4f" : "=r"(targets[2]));
    asm volatile ("la %0, 5f" : "=r"(targets[3]));
    
    // Test multi-target indirect as first branch (should not use 2-taken)
    asm volatile (
        "li t0, 0\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Multi-target indirect jump (first branch)
        "andi t2, t0, 3\n\t"      // t2 = t0 % 4
        "slli t2, t2, 3\n\t"      // t2 *= 8 (pointer size)
        "add t2, t2, %1\n\t"      // t2 = &targets[t0%4]
        "ld t3, 0(t2)\n\t"        // t3 = targets[t0%4]
        "jalr x0, t3, 0\n\t"      // Indirect jump to varying targets
        
        "2:\n\t"
        "j 6f\n\t"                // Second direct jump
        "3:\n\t"
        "j 6f\n\t"
        "4:\n\t"
        "j 6f\n\t"
        "5:\n\t"
        "j 6f\n\t"
        
        "6:\n\t"
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %2, 1b\n\t"
        
        : "+r"(result)
        : "r"(targets), "r"(cnt)
        : "t0", "t2", "t3", "memory"
    );
}

void __attribute__ ((noinline)) test_ret_ret_invalid(int cnt) {
    int result = 0;
    
    // Test ret -> ret pattern (should not use 2-taken due to RAS dual read)
    asm volatile (
        "addi sp, sp, -32\n\t"
        "sd ra, 24(sp)\n\t"
        
        "li t0, 0\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Call first function
        "jal ra, 4f\n\t"
        "3:\n\t"
        
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        "j 7f\n\t"
        
        // First function that calls second function
        "4:\n\t"
        "sd ra, 16(sp)\n\t"
        "jal ra, 6f\n\t"
        "5:\n\t"
        "ld ra, 16(sp)\n\t"
        "ret\n\t"                 // First ret
        
        // Second function that immediately returns
        "6:\n\t"
        "ret\n\t"                 // Second ret (ret->ret invalid for 2-taken)
        
        "7:\n\t"
        "ld ra, 24(sp)\n\t"
        "addi sp, sp, 32\n\t"
        : "+r"(result)
        : "r"(cnt)
        : "t0", "ra", "memory"
    );
}

void __attribute__ ((noinline)) test_conditional_second_invalid(int cnt) {
    int result = 0;
    
    // Test any -> conditional pattern (should not use 2-taken, no TAGE support)
    asm volatile (
        "li t0, 0\n\t"
        "\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // First direct jump
        "j 2f\n\t"
        "nop\n\t"
        "2:\n\t"
        
        // Conditional branch as second (should not use 2-taken)
        "andi t2, t0, 1\n\t"      // t2 = t0 % 2
        "beqz t2, 3f\n\t"         // Conditional branch
        "addi %0, %0, 1\n\t"
        "3:\n\t"
        
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        
        : "+r"(result)
        : "r"(cnt)
        : "t0", "t2", "memory"
    );
}

void __attribute__ ((noinline)) test_call_call_invalid(int cnt) {
    int result = 0;
    
    // Test call -> call pattern (should not use 2-taken due to RAS dual write)
    asm volatile (
        "addi sp, sp, -32\n\t"
        "sd ra, 24(sp)\n\t"
        
        "li t0, 0\n\t"
        "\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // First call
        "jal ra, 4f\n\t"
        "3:\n\t"
        
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        "j 7f\n\t"
        
        // First function that immediately calls second function
        "4:\n\t"
        "sd ra, 16(sp)\n\t"
        "jal ra, 6f\n\t"          // Second call (call->call invalid for 2-taken)
        "5:\n\t"
        "ld ra, 16(sp)\n\t"
        "ret\n\t"
        
        // Second function
        "6:\n\t"
        "addi %0, %0, 2\n\t"
        "ret\n\t"
        
        "7:\n\t"
        "ld ra, 24(sp)\n\t"
        "addi sp, sp, 32\n\t"
        : "+r"(result)
        : "r"(cnt)
        : "t0", "ra", "memory"
    );
}

void __attribute__ ((noinline)) test_indirect_second_invalid(int cnt) {
    int result = 0;
    volatile void* targets[2];
    
    // Setup jump targets
    asm volatile ("la %0, 3f" : "=r"(targets[0]));
    asm volatile ("la %0, 4f" : "=r"(targets[1]));
    
    // Test any -> multi-target indirect (should not use 2-taken due to ittage timing)
    asm volatile (
        "li t0, 0\n\t"
        "\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // First direct jump
        "j 2f\n\t"
        "nop\n\t"
        "2:\n\t"
        
        // Multi-target indirect as second branch (invalid for 2-taken)
        "andi t2, t0, 1\n\t"      // t2 = t0 % 2
        "slli t2, t2, 3\n\t"      // t2 *= 8 (pointer size)
        "add t2, t2, %1\n\t"      // t2 = &targets[t0%2]
        "ld t3, 0(t2)\n\t"        // t3 = targets[t0%2]
        "jalr x0, t3, 0\n\t"      // Indirect jump
        
        "3:\n\t"
        "j 5f\n\t"
        "4:\n\t"
        "j 5f\n\t"
        
        "5:\n\t"
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        
        : "+r"(result)
        : "r"(targets), "r"(cnt)
        : "t0", "t2", "t3", "memory"
    );
}

int main() {
    printf("Testing invalid two-taken combinations...\n");
    
    // Run tests to verify these patterns don't use 2-taken
    test_indirect_first_invalid(1000);
    test_ret_ret_invalid(1000);
    test_conditional_second_invalid(1000);
    test_call_call_invalid(1000);
    test_indirect_second_invalid(1000);
    
    printf("Invalid combination tests completed\n");
    return 0;
} 