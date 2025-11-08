#include <stdint.h>
#include <stdbool.h>
#pragma once
#include "util.h"
#include "terminal.h"
#include "multiboot_header.h"

struct block{
    size_t size;
    bool free;
    struct block *next;
};

typedef struct block block_t;

void print_end(multiboot_info_t* mb_info);

void heap_init();
void *malloc(size_t size);
void free(void *ptr);
