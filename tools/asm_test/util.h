#ifndef UTIL_H
#define UTIL_H

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Disable conflicting declarations from lib/util.h by defining them as macros to standard functions
// or we just don't include lib/util.h by not having it in include path.
// But assembler.c has #include "util.h".
// So this file IS "util.h".

// We already included string.h etc.
// We need to match signatures expected by assembler.c IF it uses them.
// assembler.c uses standard functions. It doesn't use the custom implementations in util.h except maybe to_string.

void to_string(int value, char *buffer);

// Mock multiboot_info_t? No, assembler.c declares: extern multiboot_info_t *multiboot_info;
// But it doesn't include the definition. It likely expects it from somewhere.
// It includes assembler.h, util.h.
// It also has: extern multiboot_info_t *multiboot_info;
// But multiboot_info_t is likely defined in kernel (multiboot.h or similar).
// assembler.c doesn't include multiboot header?
// Line 1: #include "assembler.h"
// Line 2: #include "util.h"
// Line 6: extern multiboot_info_t *multiboot_info;

// If multiboot_info_t is not a known type, compiler errors.
// We need to define it here or in repro_issue.c (but that's after util.h inclusion).
// Better define it in util.h or mock header.

typedef void multiboot_info_t;

#endif
