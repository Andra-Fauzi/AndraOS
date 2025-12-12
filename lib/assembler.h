#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    char *code;
    size_t size;
    size_t capacity;
} CodeBuffer;

typedef struct {
    char *name;
    uint32_t offset;
} Label;

typedef struct {
    Label *labels;
    int label_count;
    int label_capacity;
} LabelTable;

// Assemble x86 assembly code to machine code with ELF headers
// Returns 0 on success, 1 on error
int assemble_x86(const char *asm_text, size_t asm_len, 
                 char *output, size_t output_max, size_t *output_len);

#endif // ASSEMBLER_H
