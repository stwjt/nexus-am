#include "../tests/common.h"
#include <klib.h>

// Performance test to measure the effectiveness of two-taken prediction
// This test should show improved performance when 2-taken is enabled

// Benchmark 1: Chain of direct jumps (ideal for 2-taken)
void __attribute__ ((noinline)) benchmark_jump_chain(int cnt) {
    volatile int result = 0;
    
    // Long chain of consecutive direct jumps
    asm volatile (
        "li t0, 0\n\t"
        "\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Chain of 8 consecutive direct jumps
        "j 2f\n\t"
        "nop\n\t"
        "2:\n\t"
        "j 3f\n\t"
        "nop\n\t"
        "3:\n\t"
        "j 4f\n\t"
        "nop\n\t"
        "4:\n\t"
        "j 5f\n\t"
        "nop\n\t"
        "5:\n\t"
        "j 6f\n\t"
        "nop\n\t"
        "6:\n\t"
        "j 7f\n\t"
        "nop\n\t"
        "7:\n\t"
        "j 8f\n\t"
        "nop\n\t"
        "8:\n\t"
        "j 9f\n\t"
        "nop\n\t"
        "9:\n\t"
        
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        
        : "+r"(result)
        : "r"(cnt)
        : "t0", "memory"
    );
}

// Benchmark 2: Mixed valid patterns
void __attribute__ ((noinline)) benchmark_mixed_patterns(int cnt) {
    volatile int result = 0;
    
    asm volatile (
        "addi sp, sp, -16\n\t"
        "sd ra, 8(sp)\n\t"
        
        "li t0, 0\n\t"
        "\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Pattern 1: jump -> call
        "j 2f\n\t"
        "nop\n\t"
        "2:\n\t"
        "jal ra, 10f\n\t"
        "3:\n\t"
        
        // Pattern 2: call -> ret
        "jal ra, 11f\n\t"
        "4:\n\t"
        
        // Pattern 3: jump -> jump
        "j 5f\n\t"
        "nop\n\t"
        "5:\n\t"
        "j 6f\n\t"
        "nop\n\t"
        "6:\n\t"
        
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        "j 12f\n\t"
        
        // Function 1
        "10:\n\t"
        "addi %0, %0, 2\n\t"
        "ret\n\t"
        
        // Function 2 (immediate return for call->ret pattern)
        "11:\n\t"
        "ret\n\t"
        
        "12:\n\t"
        "ld ra, 8(sp)\n\t"
        "addi sp, sp, 16\n\t"
        : "+r"(result)
        : "r"(cnt)
        : "t0", "ra", "memory"
    );
}

// Benchmark 3: High frequency pattern
void __attribute__ ((noinline)) benchmark_high_frequency(int cnt) {
    volatile int result = 0;
    
    // High frequency pattern to stress the predictor
    asm volatile (
        "li t0, 0\n\t"
        "\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Tight loop with frequent jumps
        "j 2f\n\t"
        "2:\n\t"
        "j 3f\n\t"
        "3:\n\t"
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        
        : "+r"(result)
        : "r"(cnt)
        : "t0", "memory"
    );
}

// Benchmark 4: Compare with single predictions
void __attribute__ ((noinline)) benchmark_single_predictions(int cnt) {
    volatile int result = 0;
    
    // Single predictions (no 2-taken opportunity)
    asm volatile (
        "li t0, 0\n\t"
        "\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Single jump with computation in between
        "j 2f\n\t"
        "nop\n\t"
        "2:\n\t"
        
        // Add computation to separate potential 2-taken
        "mul t2, t0, t0\n\t"
        "add t3, t2, t0\n\t"
        "sub t4, t3, t2\n\t"
        
        "j 3f\n\t"               // This should not be 2-taken with first
        "nop\n\t"
        "3:\n\t"
        
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        
        : "+r"(result)
        : "r"(cnt)
        : "t0", "t2", "t3", "t4", "memory"
    );
}

// Benchmark 5: Stress test with many consecutive jumps
void __attribute__ ((noinline)) benchmark_stress_test(int cnt) {
    volatile int result = 0;
    
    asm volatile (
        "li t0, 0\n\t"
        "\n\t"
        ".align 4\n\t"
        "1:\n\t"
        
        // Stress test: 16 consecutive jumps
        "j L2\n\t"
        "L2: j L3\n\t"
        "L3: j L4\n\t"
        "L4: j L5\n\t"
        "L5: j L6\n\t"
        "L6: j L7\n\t"
        "L7: j L8\n\t"
        "L8: j L9\n\t"
        "L9: j L10\n\t"
        "L10: j L11\n\t"
        "L11: j L12\n\t"
        "L12: j L13\n\t"
        "L13: j L14\n\t"
        "L14: j L15\n\t"
        "L15: j L16\n\t"
        "L16: j L17\n\t"
        "L17:\n\t"
        
        "addi %0, %0, 1\n\t"
        "addi t0, t0, 1\n\t"
        "blt t0, %1, 1b\n\t"
        
        : "+r"(result)
        : "r"(cnt)
        : "t0", "memory"
    );
}

// Simple timing function using cycle counter
unsigned long get_cycles() {
    unsigned long cycles;
    asm volatile ("rdcycle %0" : "=r"(cycles));
    return cycles;
}

int main() {
    unsigned long start_cycles, end_cycles;
    const int iterations = 10000;
    
    printf("Two-taken performance benchmarks\n");
    printf("Running %d iterations each...\n\n", iterations);
    
    // Benchmark 1: Jump chain
    printf("Benchmark 1: Direct jump chain (ideal for 2-taken)\n");
    start_cycles = get_cycles();
    benchmark_jump_chain(iterations);
    end_cycles = get_cycles();
    printf("Cycles: %lu\n\n", end_cycles - start_cycles);
    
    // Benchmark 2: Mixed patterns
    printf("Benchmark 2: Mixed valid patterns\n");
    start_cycles = get_cycles();
    benchmark_mixed_patterns(iterations);
    end_cycles = get_cycles();
    printf("Cycles: %lu\n\n", end_cycles - start_cycles);
    
    // Benchmark 3: High frequency
    printf("Benchmark 3: High frequency pattern\n");
    start_cycles = get_cycles();
    benchmark_high_frequency(iterations);
    end_cycles = get_cycles();
    printf("Cycles: %lu\n\n", end_cycles - start_cycles);
    
    // Benchmark 4: Single predictions
    printf("Benchmark 4: Single predictions (baseline)\n");
    start_cycles = get_cycles();
    benchmark_single_predictions(iterations);
    end_cycles = get_cycles();
    printf("Cycles: %lu\n\n", end_cycles - start_cycles);
    
    // Benchmark 5: Stress test
    printf("Benchmark 5: Stress test (16 consecutive jumps)\n");
    start_cycles = get_cycles();
    benchmark_stress_test(iterations);
    end_cycles = get_cycles();
    printf("Cycles: %lu\n\n", end_cycles - start_cycles);
    
    printf("Performance benchmarks completed\n");
    printf("Expected: Benchmarks 1, 2, 3, 5 should be faster with 2-taken enabled\n");
    printf("Expected: Benchmark 4 should show similar performance (no 2-taken opportunity)\n");
    
    return 0;
} 