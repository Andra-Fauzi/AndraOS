#pragma once
#include <stdint.h>
#include <stddef.h>

// From original assembler.h
int assemble_x86(const char *asm_text, size_t asm_len, char *output, size_t output_max, size_t *output_len);
