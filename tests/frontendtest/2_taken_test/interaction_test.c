#include "../tests/common.h"
#include <klib.h>

// Test two-taken interaction with other frontend components
// Including RAS interaction, TAGE conflicts, FTQ behavior, etc.

void __attribute__ ((noinline)) test_ras_interaction(int cnt) {
    int result = 0;
    
    // Test RAS interaction with 2-taken predictions
    // Valid: call -> ret, but not call -> call or ret -> ret
    asm volatile (
        "addi sp, sp, -32\n\t"
        "sd ra, 24(sp)\n\t"
        
        "li t0, 0\n\t"
        "\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Test call -> ret pattern (should work with 2-taken)
        "jal ra, 4f\n\t"         // Call function
        "3:\n\t"
        
        // Test jump -> call pattern
        "j 6f\n\t"
        "nop\n\t"
        "6:\n\t"
        "jal ra, 8f\n\t"         // Should be 2-taken with jump
        "7:\n\t"
        
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        "j 9f\n\t"
        
        // Function that immediately returns (call->ret 2-taken)
        "4:\n\t"
        "addi %0, %0, 2\n\t"
        "ret\n\t"
        
        // Another function
        "8:\n\t"
        "addi %0, %0, 3\n\t"
        "ret\n\t"
        
        "9:\n\t"
        "ld ra, 24(sp)\n\t"
        "addi sp, sp, 32\n\t"
        : "+r"(result)
        : "r"(cnt)
        : "t0", "ra", "memory"
    );
}

void __attribute__ ((noinline)) test_deep_call_stack(int cnt) {
    int result = 0;
    
    // Test deep call stack to stress RAS with 2-taken
    asm volatile (
        "addi sp, sp, -64\n\t"
        "sd ra, 56(sp)\n\t"
        
        "li t0, 0\n\t"
        "\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Deep call chain with 2-taken opportunities
        "jal ra, 4f\n\t"         // Level 1
        "3:\n\t"
        
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        "j 11f\n\t"
        
        // Level 1 function
        "4:\n\t"
        "sd ra, 48(sp)\n\t"
        "jal ra, 6f\n\t"         // Level 2
        "5:\n\t"
        "ld ra, 48(sp)\n\t"
        "ret\n\t"
        
        // Level 2 function
        "6:\n\t"
        "sd ra, 40(sp)\n\t"
        "jal ra, 8f\n\t"         // Level 3
        "7:\n\t"
        "ld ra, 40(sp)\n\t"
        "ret\n\t"
        
        // Level 3 function (call->ret 2-taken opportunity)
        "8:\n\t"
        "sd ra, 32(sp)\n\t"
        "jal ra, 10f\n\t"        // Level 4
        "9:\n\t"
        "ld ra, 32(sp)\n\t"
        "ret\n\t"
        
        // Level 4 function (immediate return)
        "10:\n\t"
        "addi %0, %0, 4\n\t"
        "ret\n\t"                // Should be 2-taken with call
        
        "11:\n\t"
        "ld ra, 56(sp)\n\t"
        "addi sp, sp, 64\n\t"
        : "+r"(result)
        : "r"(cnt)
        : "t0", "ra", "memory"
    );
}

void __attribute__ ((noinline)) test_mixed_prediction_sources(int cnt) {
    int result = 0;
    
    // Test interaction between uBTB 2-taken and higher-level predictors
    asm volatile (
        "li t0, 0\n\t"
        "\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Mix of patterns that involve different predictors
        "andi t2, t0, 3\n\t"     // t2 = t0 % 4
        
        // Pattern A: Direct jumps (uBTB 2-taken)
        "bnez t2, 4f\n\t"
        "j 2f\n\t"
        "nop\n\t"
        "2:\n\t"
        "j 3f\n\t"
        "nop\n\t"
        "3:\n\t"
        "j 8f\n\t"
        
        // Pattern B: Conditional branches (no 2-taken for cond as 2nd)
        "4:\n\t"
        "li t3, 1\n\t"
        "beq t2, t3, 5f\n\t"
        "j 6f\n\t"
        "nop\n\t"
        "5:\n\t"
        "andi t4, t0, 1\n\t"     // Conditional as second (invalid for 2-taken)
        "beqz t4, 6f\n\t"
        "nop\n\t"
        "6:\n\t"
        "j 8f\n\t"
        
        "8:\n\t"
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        
        : "+r"(result)
        : "r"(cnt)
        : "t0", "t2", "t3", "t4", "memory"
    );
}

void __attribute__ ((noinline)) test_ftq_double_entry(int cnt) {
    int result = 0;
    
    // Test FTQ behavior with 2-taken generating two FB entries
    // According to spec: "2 taken生成的1个FB，分别存在两个FTQ entry中"
    asm volatile (
        "li t0, 0\n\t"
        "\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Create pattern that should generate 2 FTQ entries
        "j 2f\n\t"               // First FB
        "nop\n\t"
        "2:\n\t"
        "j 3f\n\t"               // Second FB (2-taken)
        "nop\n\t"
        "3:\n\t"
        
        // Add more work to stress FTQ
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

void __attribute__ ((noinline)) test_override_bubble_scenarios(int cnt) {
    int result = 0;
    
    // Test scenarios that might cause override bubbles
    // When uBTB prediction differs from high-level predictors
    asm volatile (
        "li t0, 0\n\t"
        "\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Create patterns that might cause prediction mismatches
        "andi t2, t0, 7\n\t"     // t2 = t0 % 8
        
        // Alternating behavior to stress predictors
        "li t3, 4\n\t"
        "blt t2, t3, 2f\n\t"
        
        // Branch taken path
        "j 3f\n\t"
        "nop\n\t"
        "3:\n\t"
        "j 5f\n\t"               // 2-taken opportunity
        "nop\n\t"
        "j 6f\n\t"
        
        // Branch not taken path
        "2:\n\t"
        "addi %0, %0, 10\n\t"
        "j 5f\n\t"
        
        "5:\n\t"
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        
        "6:\n\t"
        : "+r"(result)
        : "r"(cnt)
        : "t0", "t2", "t3", "memory"
    );
}

void __attribute__ ((noinline)) test_s3_verification(int cnt) {
    int result = 0;
    
    // Test S3 stage verification for 2-taken updates
    // According to spec: updates happen after S3 verification
    asm volatile (
        "li t0, 0\n\t"
        "\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Pattern that should be verified in S3
        "j 2f\n\t"
        "nop\n\t"
        "2:\n\t"
        "j 3f\n\t"
        "nop\n\t"
        "3:\n\t"
        
        // Add computation to allow S3 verification
        "add t2, t0, t0\n\t"
        "sub t3, t2, t0\n\t"
        "or t4, t2, t3\n\t"
        
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        
        : "+r"(result)
        : "r"(cnt)
        : "t0", "t2", "t3", "t4", "memory"
    );
}

int main() {
    printf("Testing two-taken interactions with frontend components...\n");
    
    // Test RAS interactions
    test_ras_interaction(1000);
    test_deep_call_stack(500);
    
    // Test predictor interactions
    test_mixed_prediction_sources(1000);
    
    // Test FTQ behavior
    test_ftq_double_entry(1000);
    
    // Test edge scenarios
    test_override_bubble_scenarios(1000);
    test_s3_verification(1000);
    
    printf("Interaction tests completed\n");
    return 0;
} 