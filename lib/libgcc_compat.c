#include <stdint.h>

// Compiler intrinsic functions untuk operasi 64-bit division/modulo
// Diperlukan oleh uACPI library pada arsitektur 32-bit

// Helper: 64-bit unsigned division
uint64_t __udivdi3(uint64_t a, uint64_t b) {
    if (b == 0) {
        return 0; // Division by zero
    }
    
    // Simple implementation using shift and subtract algorithm
    uint64_t quotient = 0;
    uint64_t remainder = 0;
    
    for (int i = 63; i >= 0; i--) {
        remainder <<= 1;
        remainder |= (a >> i) & 1;
        
        if (remainder >= b) {
            remainder -= b;
            quotient |= (1ULL << i);
        }
    }
    
    return quotient;
}

// Helper: 64-bit unsigned modulo
uint64_t __umoddi3(uint64_t a, uint64_t b) {
    if (b == 0) {
        return 0; // Division by zero
    }
    
    uint64_t remainder = 0;
    
    for (int i = 63; i >= 0; i--) {
        remainder <<= 1;
        remainder |= (a >> i) & 1;
        
        if (remainder >= b) {
            remainder -= b;
        }
    }
    
    return remainder;
}
