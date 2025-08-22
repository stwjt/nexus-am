# Two-Taken Feature Test Suite

This test suite validates the implementation of the two-taken (2-tkn) feature in the frontend branch predictor. The two-taken feature allows the uBTB (micro Branch Target Buffer) to predict two consecutive taken branches in a single cycle, potentially providing 50%+ performance improvement with only 10% hardware overhead.

## Test Overview

The test suite consists of 5 main test categories:

### 1. Basic Two-Taken Tests (`basic_two_taken.c`)

Tests fundamental two-taken functionality with valid branch combinations:

- **Direct jump -> Direct jump**: Chain of unconditional jumps
- **Direct jump -> Call**: Jump followed by function call  
- **Call -> Return**: Function call immediately followed by return
- **pt_2nd=false case**: First branch taken, second FB has no branch (fallthrough)

**Expected Behavior**: These patterns should be predicted as 2-taken and generate two fetch bundles (FBs) that are stored in separate FTQ entries.

### 2. Invalid Combinations Tests (`invalid_combinations.c`)

Tests branch combinations that should NOT use 2-taken prediction:

- **Multi-target indirect -> any**: Indirect branches with varying targets as first branch
- **ret -> ret**: Two consecutive returns (RAS dual read port limitation)
- **any -> conditional**: Conditional branch as second branch (no 2-taken TAGE support)
- **call -> call**: Two consecutive calls (RAS dual write port limitation)
- **any -> multi-target indirect**: Multi-target indirect as second branch (ittage timing issues)

**Expected Behavior**: These patterns should fall back to single prediction mode.

### 3. Edge Cases Tests (`edge_cases.c`)

Tests corner scenarios and edge conditions:

- **Alignment boundaries**: 32B alignment issues and boundary crossings
- **Bubble scenarios**: Pipeline bubbles between predictions
- **Ping-pong patterns**: Alternating branch behaviors that stress tcnt updates
- **uCnt replacement**: Entry replacement scenarios in uBTB
- **tfx (target fix)**: Static vs dynamic indirect jump targets
- **Cross-page boundaries**: Long jumps that cross page boundaries
- **Update timing**: Rapid consecutive predictions

**Expected Behavior**: The implementation should handle these edge cases gracefully without crashes or incorrect predictions.

### 4. Performance Tests (`performance_test.c`)

Benchmarks to measure two-taken effectiveness:

- **Jump chain benchmark**: Long chains of direct jumps (ideal for 2-taken)
- **Mixed patterns**: Various valid 2-taken combinations
- **High frequency**: Tight loops with frequent jumps
- **Single predictions baseline**: Patterns without 2-taken opportunities
- **Stress test**: 16 consecutive jumps

**Expected Behavior**: Benchmarks 1, 2, 3, and 5 should show improved performance with 2-taken enabled. Benchmark 4 should show similar performance (no 2-taken opportunity).

### 5. Interaction Tests (`interaction_test.c`)

Tests interaction with other frontend components:

- **RAS interaction**: Call/return stack behavior with 2-taken
- **Deep call stack**: Stress testing RAS with nested calls
- **Mixed prediction sources**: Interaction between uBTB and higher-level predictors
- **FTQ double entry**: Two FBs stored in separate FTQ entries
- **Override bubbles**: Scenarios causing prediction conflicts
- **S3 verification**: Update timing after S3 stage verification

**Expected Behavior**: 2-taken should work correctly with existing frontend components without conflicts.

## Implementation Details Tested

### uBTB Structure
The tests validate the uBTB entry format with:
- **slot1**: Primary branch slot (pos, attr, target, etc.)
- **slot2**: Secondary branch slot for 2-taken
- **valid_2nd**: Second slot validity bit
- **pt_2nd**: Predict taken bit for second slot
- **tfx**: Target fix bit for indirect branches
- **tcnt/ucnt**: Taken/usage counters

### Update Mechanisms
- **High-level predictor results**: uBTB updated with higher predictor results
- **S3 verification**: Updates occur after S3 stage verification
- **tmpPredDff**: Temporary storage for consecutive predictions
- **Bubble handling**: Bubbles not written to tmpPredDff

### Limitations Enforced
- No conditional branches as second slot (no 2-taken TAGE support)
- No ret->ret or call->call (RAS port limitations)
- No multi-target indirect in certain positions (timing constraints)

## Building and Running

✅ **All tests successfully compile with RISC-V 64-bit toolchain!**

```bash
# Build all tests at once
make ARCH=riscv64-xs all

# Build individual tests
make ARCH=riscv64-xs basic_two_taken      # Basic functionality tests
make ARCH=riscv64-xs invalid_combinations # Invalid combination tests  
make ARCH=riscv64-xs edge_cases          # Edge case tests
make ARCH=riscv64-xs performance_test    # Performance benchmarks
make ARCH=riscv64-xs interaction_test    # Component interaction tests

# Clean build artifacts
make clean
```

### Generated Binaries
After successful compilation, the following test binaries are generated in `build/*/build/`:
- `basic_two_taken-riscv64-xs.bin` (5.8KB) - Core 2-taken functionality
- `edge_cases-riscv64-xs.bin` (8.0KB) - Edge cases and boundary conditions  
- `invalid_combinations-riscv64-xs.bin` (5.9KB) - Invalid pattern validation
- `performance_test-riscv64-xs.bin` (6.5KB) - Performance benchmarks
- `interaction_test-riscv64-xs.bin` (6.0KB) - Component interaction tests

## Expected Results

### With 2-taken Enabled:
- Basic tests should complete successfully
- Invalid combinations should fall back to single prediction
- Performance tests should show cycle count improvements for applicable patterns
- Edge cases should be handled without errors
- Component interactions should work correctly

### Without 2-taken Enabled:
- All tests should still run (falling back to single prediction)
- Performance tests should show baseline performance
- No crashes or incorrect behavior

## Validation Metrics

1. **Functionality**: All tests complete without crashes
2. **Correctness**: Invalid patterns properly rejected
3. **Performance**: Measurable improvement in applicable benchmarks
4. **Compatibility**: No regression in existing functionality

## Implementation Notes

This test suite is designed to work with the uBTB specification as documented in the design documents. Key features tested include:

- Limited 2-taken implementation in uBTB
- Valid/invalid branch combination enforcement
- FTQ double entry generation
- Pipeline update timing
- Component interaction compatibility

The tests provide comprehensive coverage of the 2-taken feature to ensure robust implementation and identify any issues before production deployment.

## Technical Notes

### Build System Fixes Applied
The test suite was updated to fix several build issues:
- **Assembly syntax**: Removed intermediate register loading (`li t1, %1`) and used direct parameter references (`blt t0, %1, 1b`)
- **Makefile structure**: Switched from dynamic multi-target approach to individual subdirectory builds (similar to amtest)
- **Linker flag issue**: Fixed `-melf64lriscv` flag being passed to gcc instead of the linker by using proper build isolation
- All tests now compile cleanly with the riscv64-xs toolchain

### Hardware Requirements
These tests are designed for gem5 simulation with the xs (XiangShan) CPU model, specifically targeting the uBTB 2-taken feature implementation as specified in the design documents.

### Test Status
✅ **All 5 test binaries successfully compiled and ready for execution**