#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "assembler.h"

void run_test(const char *name, const char *asm_code, const unsigned char *expected_bytes, size_t expected_len) {
    printf("Running test: %s... ", name);
    
    char output[1024];
    size_t output_len = 0;
    
    int result = assemble_x86(asm_code, strlen(asm_code), output, 1024, &output_len);
    
    if (result != 0) {
        printf("FAILED (Assembly error)\n");
        return;
    }
    
    if (output_len < 84) {
        printf("FAILED (Output too short)\n");
        return;
    }
    
    unsigned char *code = (unsigned char *)output + 84;
    size_t code_len = output_len - 84;
    
    if (code_len != expected_len) {
        printf("FAILED (Length mismatch: expected %zu, got %zu)\n", expected_len, code_len);
        printf("Code: ");
        for(size_t i=0; i<code_len; i++) printf("%02X ", code[i]);
        printf("\n");
        return;
    }
    
    if (memcmp(code, expected_bytes, expected_len) != 0) {
        printf("FAILED (Content mismatch)\n");
        printf("Expected: ");
        for(size_t i=0; i<expected_len; i++) printf("%02X ", expected_bytes[i]);
        printf("\nGot:      ");
        for(size_t i=0; i<code_len; i++) printf("%02X ", code[i]);
        printf("\n");
        return;
    }
    
    printf("PASSED\n");
}

int main() {
    // Test 1: Simple MOV
    const char *test1_asm = "mov %eax, %ebx\nmov $1, %eax\nret\n";
    const unsigned char test1_bin[] = { 0x89, 0xC3, 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 };
    run_test("Basic instructions", test1_asm, test1_bin, sizeof(test1_bin));

    // Test 1b: DEC alone
    const char *test1b_asm = "dec %eax\n";
    const unsigned char test1b_bin[] = { 0x48 };
    run_test("DEC check", test1b_asm, test1b_bin, sizeof(test1b_bin));

    // Test 3: Labels and Jumps (Forward) - EXPECTING LONG JUMP (5 bytes)
    // jmp label (E9 01 00 00 00)
    // nop (90)
    // label:
    // ret (C3)
    const char *test3_asm = "jmp label\nnop\nlabel:\nret\n";
    const unsigned char test3_bin[] = { 0xE9, 0x01, 0x00, 0x00, 0x00, 0x90, 0xC3 };
    run_test("Forward Jump", test3_asm, test3_bin, sizeof(test3_bin));

    // Test 4: Labels and Jumps (Backward) - EXPECTING LONG JUMP
    // label:
    // dec %eax  (48)
    // jnz label (0F 85 F9 FF FF FF) (-7)
    // ret (C3)
    const char *test4_asm = "label:\ndec %eax\njnz label\nret\n";
    const unsigned char test4_bin[] = { 0x48, 0x0F, 0x85, 0xF9, 0xFF, 0xFF, 0xFF, 0xC3 };
    run_test("Backward Jump", test4_asm, test4_bin, sizeof(test4_bin));

    return 0;
}
