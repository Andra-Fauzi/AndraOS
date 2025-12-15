#include "util.h"
#include "assembler.h"
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

typedef struct {
    char **data;
    int len;
    int cap;
} StringArray;

// Character classification
int tolower(int c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

int toupper(int c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

int ispunct(int c) {
    return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') ||
           (c >= '[' && c <= '`') || (c >= '{' && c <= '~');
}

int isxdigit(int c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// String functions
char *strchr(const char *str, int c) {
    if (!str) return NULL;
    for (; *str; str++) {
        if (*str == c) return (char *)str;
    }
    return NULL;
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
    if (!s1 || !s2) return 1;
    for (size_t i = 0; i < n && s1[i] && s2[i]; i++) {
        int c1 = tolower(s1[i]);
        int c2 = tolower(s2[i]);
        if (c1 != c2) return c1 - c2;
    }
    return 0;
}

// Number conversion
unsigned long strtoul(const char *str, char **endptr, int base) {
    if (!str) return 0;
    
    unsigned long result = 0;
    int sign = 1;
    
    // Skip whitespace
    while (*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')) str++;
    
    // Handle sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    // Auto-detect base
    if (base == 0 || base == 16) {
        if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            base = 16;
            str += 2;
        } else if (base == 0) {
            base = 10;
        }
    }
    
    // Convert
    while (*str) {
        int digit = -1;
        if (*str >= '0' && *str <= '9') {
            digit = *str - '0';
        } else if (*str >= 'a' && *str <= 'f') {
            digit = *str - 'a' + 10;
        } else if (*str >= 'A' && *str <= 'F') {
            digit = *str - 'A' + 10;
        }
        
        if (digit < 0 || digit >= base) break;
        
        result = result * base + digit;
        str++;
    }
    
    if (endptr) *endptr = (char *)str;
    return result * sign;
}

long double strtold(const char *str, char **endptr) {
    if (!str) return 0.0L;
    
    // Simple implementation: just parse integer part
    long double result = 0.0L;
    int sign = 1;
    
    // Skip whitespace
    while (*str && (*str == ' ' || *str == '\t')) str++;
    
    // Handle sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    // Parse integer part
    while (*str >= '0' && *str <= '9') {
        result = result * 10.0L + (*str - '0');
        str++;
    }
    
    // Parse decimal part if present
    if (*str == '.') {
        str++;
        long double frac = 0.1L;
        while (*str >= '0' && *str <= '9') {
            result += frac * (*str - '0');
            frac *= 0.1L;
            str++;
        }
    }
    
    if (endptr) *endptr = (char *)str;
    return result * sign;
}

// GCC builtins for 64-bit arithmetic on i386
unsigned long long __udivdi3(unsigned long long a, unsigned long long b) {
    return a / b;
}

long long __divdi3(long long a, long long b) {
    return a / b;
}

unsigned long long __umoddi3(unsigned long long a, unsigned long long b) {
    return a % b;
}

long long __moddi3(long long a, long long b) {
    return a % b;
}

// Chibicc variables and functions
StringArray include_paths = {NULL, 0, 0};
char *base_file = "kernel.c";

int file_exists(char *path) {
    // Stub: always return 1 for now
    return 1;
}

// Global exit status for chibicc errors
static int chibicc_exit_status = 0;

void exit(int status) {
    // In kernel context, don't halt - just save exit status
    // The compiler wrapper will check this and return error
    chibicc_exit_status = status;
    // Use longjmp-like mechanism or just return
    // For now, we'll use a goto pattern by throwing error via error_tok
    // But simpler: just set flag and return
}

// Get the exit status that was set
int get_chibicc_exit_status(void) {
    return chibicc_exit_status;
}

// Reset exit status
void reset_chibicc_exit_status(void) {
    chibicc_exit_status = 0;
}

// Global variables for chibicc
bool opt_fpic = false;
bool opt_fcommon = false;

// Main compilation interface - calls chibicc API from main.c
int subc_compile(char *src, int src_len, char *dest, size_t dest_max, size_t *dest_len) {
    if (!src || src_len <= 0) {
        if (dest_len) *dest_len = 0;
        return 1;  // Error
    }
    
    // Forward declare external functions from main.c
    extern void chibicc_set_source(uint8_t *buf, size_t len);
    extern int chibicc_run_from_memory(char *out, size_t out_size, size_t *out_len);
    
    // Allocate temporary buffer for assembly output
    static char asm_buffer[16384];  // 16KB for assembly
    
    // Reset exit status before compilation
    reset_chibicc_exit_status();
    
    // Set source buffer for chibicc
    chibicc_set_source((uint8_t *)src, src_len);
    
    // Run compilation from memory - produces assembly code
    size_t asm_len = 0;
    int result = chibicc_run_from_memory(asm_buffer, 16384, &asm_len);

    extern multiboot_info_t *multiboot_info;
    extern uint16_t active_cluster;
    kprint("hasil buffer : \n", multiboot_info);
    kprint(asm_buffer, multiboot_info);
    print_char('\n', multiboot_info);
    kprint("panjang buffer : \n", multiboot_info);
    char len_str[255];
    to_string(asm_len, len_str);
    kprint(len_str, multiboot_info);
    print_char('\n', multiboot_info);

    // char name[11];
    // to_fat_name_fixed("sigma.asm\0", name);
    // writefile(0, name, asm_buffer, asm_len);
    
    // Check if chibicc called exit() due to error
    int exit_status = get_chibicc_exit_status();
    if (exit_status != 0) {
        if (dest_len) *dest_len = 0;
        // free(asm_buffer);
        return 1;  // Chibicc compilation error
    }
    
    if (result != 0 || asm_len == 0) {
        if (dest_len) *dest_len = 0;
        // free(asm_buffer);  // Skip free if malloc not fully implemented
        return 1;  // Compilation error
    }
    
    // Now assemble the x86 code to ELF binary
    result = assemble_x86(asm_buffer, asm_len, dest, dest_max, dest_len);
    
    // free(asm_buffer);  // Skip free if malloc not fully implemented
    
    return result;  // Return assembly result
}
