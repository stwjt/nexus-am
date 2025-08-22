#include <am.h>
#include <klib.h>

int main() {
    volatile int counter = 0;
    
    // Simple finite loop to test execution time
    for (int i = 0; i < 1000; i++) {
        counter += i;
    }
    
    return (counter > 500000) ? 1 : 0;
} 