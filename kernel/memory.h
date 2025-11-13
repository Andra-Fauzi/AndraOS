#include <stdint.h>
#include <stdbool.h>
#pragma once
#include "util.h"
#include "terminal.h"
#include "multiboot_header.h"

#define HEAP_SIZE 0x1000000
#define MIN_BLOCK_SIZE 16
#define ALIGNMENT 4
#define PAGE_SIZE 4096
#define KERNEL_BASE 0xC0000000

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
uint32_t virt_to_phys(void *v);
void *phys_to_virt(uint32_t p);
