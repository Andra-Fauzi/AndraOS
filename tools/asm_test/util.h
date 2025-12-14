#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

// Mock kernel functions with libc
#define kprint(msg, info) printf("%s", msg)
#define print_char(c, info) putchar(c)

// Definitions from util.h
typedef struct {
    char *buf;
    size_t len;
    size_t cap;
    size_t pos;
} MemStream;

// Use libc implementations for these
// memcpy, memset, etc are already included via string.h

static inline int tolower_custom(int c) { return tolower(c); }
static inline int toupper_custom(int c) { return toupper(c); }
static inline int ispunct_custom(int c) { return ispunct(c); }
static inline int isxdigit_custom(int c) { return isxdigit(c); }

// Redefine to avoid conflict if needed, or just use standard ones check
// In the assembler.c I will use these names, so headers must match or I use macros
