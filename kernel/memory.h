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
    struct block *prev;
};

typedef struct block block_t;

void print_end(multiboot_info_t* mb_info);

void heap_init();
void *malloc(size_t size);
void free(void *ptr);

/* Allocate a single 4KB-aligned page for use as a page-table or other page-aligned structure.
 * Returns the physical/virtual address of the page (aligned to 0x1000), or 0 on OOM.
 * This is a simple bump allocator built on top of the kernel heap start.
 */
uint32_t alloc_page(void);
