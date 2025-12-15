#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include "util.h"

// Mock kernel types and variables
typedef void multiboot_info_t;
multiboot_info_t *multiboot_info = NULL;

void kprint(const char *str, void *info) {
    printf("%s", str);
}

void kprint_hex(uint32_t val, void *info) {
    printf("%X", val);
}

void to_string(int value, char *buffer) {
    sprintf(buffer, "%d", value);
}

// Forward declaration of assemble_x86 from assembler.c
int assemble_x86(const char *asm_text, size_t asm_len, char *output, size_t output_max, size_t *output_len);

int main() {
    // Simple test case: call a label defined later
    // In pass 1, label is recorded. In pass 2, call offset is calculated.
    const char *asm_code = 
        "call my_func\n"
        "nop\n"
        "my_func:\n"
        "ret\n";

    printf("Testing assembly:\n%s\n", asm_code);

    char output[1024];
    size_t output_len = 0;
    
    // We might need to mock util.c compilation or link against it.
    // Ideally we include assembler.c directly but it might have conflicts.
    // Let's try to link against assembler.o logic by compiling this file with assembler.c
    
    int ret = assemble_x86(asm_code, strlen(asm_code), output, sizeof(output), &output_len);
    
    if (ret != 0) {
        printf("Assembly failed with error %d\n", ret);
        return 1;
    }

    printf("Output size: %lu\n", output_len);
    
    // Offset for code in ELF usually starts at 84 (0x54)
    if (output_len > 84) {
        uint8_t *code = (uint8_t*)output + 84;
        size_t code_len = output_len - 84;
        
        printf("Code hex:\n");
        for (size_t i = 0; i < code_len; i++) {
            printf("%02X ", code[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        printf("\n");
    }

    // Test case 2: Backward call
    const char *asm_code_2 = 
        "func2:\n"
        "ret\n"
        "call func2\n";
        
    printf("\nTesting backward call:\n%s\n", asm_code_2);
    ret = assemble_x86(asm_code_2, strlen(asm_code_2), output, sizeof(output), &output_len);
    
    if (ret == 0 && output_len > 84) {
        uint8_t *code = (uint8_t*)output + 84;
        size_t code_len = output_len - 84;
        printf("Code hex:\n");
        for (size_t i = 0; i < code_len; i++) {
            printf("%02X ", code[i]);
        }
        printf("\n");
    }
    
    return 0;
}
