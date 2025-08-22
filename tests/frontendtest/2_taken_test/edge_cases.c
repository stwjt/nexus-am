#include "../tests/common.h"
#include <klib.h>

// Test edge cases and corner scenarios for two-taken prediction
// Including alignment issues, bubble scenarios, ping-pong effects, etc.

void __attribute__ ((noinline)) test_alignment_edge_cases(int cnt) {
    int result = 0;
    
    // Test 32B alignment boundaries as mentioned in spec
    // uBTB half align - pos is in 32B aligned FB
    asm volatile (
        "li t0, 0\n\t"
        ".align 5\n\t"           // 32-byte alignment
        "1:\n\t"
        
        // Branches near 32B boundary
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "j 2f\n\t"              // First branch near boundary
        "nop\n\t"
        "2:\n\t"
        "j 3f\n\t"              // Second branch crosses boundary
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

void __attribute__ ((noinline)) test_bubble_scenarios(int cnt) {
    int result = 0;
    
    // Test scenarios with bubbles between predictions
    // According to spec, bubbles should not be written to tmpPredDff
    asm volatile (
        "li t0, 0\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Create pattern that might cause bubbles
        "j 2f\n\t"
        "nop\n\t"
        "2:\n\t"
        
        // Add some complex operations that might cause pipeline bubbles
        "mul t2, t0, t0\n\t"     // Potential bubble source
        "div t3, t2, %1\n\t"     // Another potential bubble source
        
        "j 3f\n\t"               // Second branch after potential bubble
        "nop\n\t"
        "3:\n\t"
        
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        
        : "+r"(result)
        : "r"(cnt)
        : "t0", "t2", "t3", "memory"
    );
}

void __attribute__ ((noinline)) test_ping_pong_pattern(int cnt) {
    int result = 0;
    
    // Test ping-pong scenarios that might cause prediction conflicts
    // Alternating branch behaviors to stress tcnt updates
    asm volatile (
        "li t0, 0\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Alternating pattern: sometimes take first branch, sometimes don't
        "andi t2, t0, 7\n\t"     // t2 = t0 % 8
        "li t3, 3\n\t"
        "bgt t2, t3, 2f\n\t"     // Take if t2 > 3
        "j 4f\n\t"               // Don't take: skip to fallthrough
        
        "2:\n\t"
        "j 3f\n\t"               // Taken path: direct jump
        "nop\n\t"
        "3:\n\t"
        "j 5f\n\t"               // Second jump if first was taken
        
        "4:\n\t"                 // Fallthrough path
        "addi %0, %0, 10\n\t"    // Different behavior
        "j 5f\n\t"
        
        "5:\n\t"
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        
        : "+r"(result)
        : "r"(cnt)
        : "t0", "t2", "t3", "memory"
    );
}

void __attribute__ ((noinline)) test_ucnt_replacement_edge(int cnt) {
    int result = 0;
    
    // Test ucnt replacement scenarios
    // Create many different patterns to trigger replacement
    asm volatile (
        "li t0, 0\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Generate different jump patterns to fill uBTB
        "andi t2, t0, 15\n\t"    // t2 = t0 % 16
        
        // Pattern 0-3: direct jumps
        "li t3, 4\n\t"
        "blt t2, t3, 2f\n\t"
        "li t3, 8\n\t"
        "blt t2, t3, 3f\n\t"
        "li t3, 12\n\t"
        "blt t2, t3, 4f\n\t"
        "j 5f\n\t"
        
        "2:\n\t"
        "j 6f\n\t"               // Pattern A
        "3:\n\t"
        "j 6f\n\t"               // Pattern B
        "4:\n\t"
        "j 6f\n\t"               // Pattern C
        "5:\n\t"
        "j 6f\n\t"               // Pattern D
        
        "6:\n\t"
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        
        : "+r"(result)
        : "r"(cnt)
        : "t0", "t2", "t3", "memory"
    );
}

void __attribute__ ((noinline)) test_tfx_static_target(int cnt) {
    int result = 0;
    volatile void* target;
    
    // Setup static target
    asm volatile ("la %0, 3f" : "=r"(target));
    
    // Test tfx (target fix) scenarios with static vs dynamic targets
    asm volatile (
        "li t0, 0\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Static target indirect jump (tfx should be 1)
        "ld t2, %1\n\t"          // Always same target
        "jalr x0, t2, 0\n\t"     // Indirect with static target
        
        "3:\n\t"
        "j 4f\n\t"               // Second branch after static indirect
        "nop\n\t"
        "4:\n\t"
        
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %2, 1b\n\t"
        
        : "+r"(result)
        : "m"(target), "r"(cnt)
        : "t0", "t2", "memory"
    );
}

void __attribute__ ((noinline)) test_cross_page_boundary(int cnt) {
    int result = 0;
    
    // Test branches that might cross page boundaries
    // This is important for address prediction accuracy
    asm volatile (
        "li t0, 0\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Long jump that might cross page boundary
        "j 2f\n\t"
        ".skip 2048\n\t"         // Force distance
        "2:\n\t"
        
        "j 3f\n\t"               // Second jump after long jump
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

void __attribute__ ((noinline)) test_update_timing_edge(int cnt) {
    int result = 0;
    
    // Test update timing edge cases
    // Rapid consecutive predictions to stress update logic
    asm volatile (
        "li t0, 0\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Rapid sequence of jumps
        "j 2f\n\t"
        "2:\n\t"
        "j 3f\n\t"
        "3:\n\t"
        "j 4f\n\t"
        "4:\n\t"
        "j 5f\n\t"
        "5:\n\t"
        
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        
        : "+r"(result)
        : "r"(cnt)
        : "t0", "memory"
    );
}

int main() {
    printf("Testing two-taken edge cases...\n");
    
    // Run edge case tests
    test_alignment_edge_cases(500);
    test_bubble_scenarios(500);
    test_ping_pong_pattern(500);
    test_ucnt_replacement_edge(500);
    test_tfx_static_target(500);
    test_cross_page_boundary(100);  // Fewer iterations due to distance
    test_update_timing_edge(500);
    
    printf("Edge case tests completed\n");
    return 0;
} 