#include "memory.h"

extern uint8_t _end;

#define HEAP_SIZE 0x1000000
#define MIN_BLOCK_SIZE 16
#define ALIGNMENT 4
#define PAGE_SIZE 4096


uint8_t *heap_start;
uint8_t *heap_end;
block_t *heap_head;

void heap_init() {
    heap_start = &_end;
    heap_end = heap_start + HEAP_SIZE;

    // Align heap start
    heap_start = (uint8_t*)((uintptr_t)heap_start & ~(ALIGNMENT - 1));

    heap_head = (block_t *)heap_start;
    heap_head->size = HEAP_SIZE - sizeof(block_t);
    heap_head->free = true;
    heap_head->next = NULL;
    heap_head->prev = NULL;
}

void *malloc(size_t size) {
    if (size == 0) return NULL;

    // Align size
    size_t aligned_size = (size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
    if (aligned_size < MIN_BLOCK_SIZE) {
        aligned_size = MIN_BLOCK_SIZE;
    }

    block_t *current = heap_head;
    block_t *last = NULL;

    // First fit dengan splitting
    while (current != NULL) {
        if (current->free && current->size >= aligned_size) {
            // Split block jika cukup besar
            if (current->size >= aligned_size + sizeof(block_t) + MIN_BLOCK_SIZE) {
                block_t *new_block = (block_t *)((uint8_t *)(current + 1) + aligned_size);

                new_block->size = current->size - aligned_size - sizeof(block_t);
                new_block->free = true;
                new_block->next = current->next;
                new_block->prev = current;

                if (current->next) {
                    current->next->prev = new_block;
                }

                current->size = aligned_size;
                current->next = new_block;
            }

            current->free = false;
            return (void*)(current + 1);
        }
        last = current;
        current = current->next;
    }

    // Extend heap - buat block baru di akhir
    if (last == NULL) return NULL;

    uint8_t *new_block_addr = (uint8_t *)last + sizeof(block_t) + last->size;
    if (new_block_addr + sizeof(block_t) + aligned_size > heap_end) {
        return NULL; // Out of memory
    }

    block_t *new_block = (block_t *)new_block_addr;
    new_block->size = aligned_size;
    new_block->free = false;
    new_block->next = NULL;
    new_block->prev = last;

    last->next = new_block;
    return (void*)(new_block + 1);
}

void free(void *ptr) {
    if (ptr == NULL) return;

    block_t *block = (block_t *)ptr - 1;
    if (block->free) return; // Double free detection

    block->free = true;

    // Coalesce dengan next block
    if (block->next && block->next->free) {
        block->size += sizeof(block_t) + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    // Coalesce dengan previous block
    if (block->prev && block->prev->free) {
        block->prev->size += sizeof(block_t) + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

extern uint8_t _kernel_end;

uint32_t pd_index(uint32_t va) {
	return (va >> 22) & 0x3FF;
}

uint32_t pt_index(uint32_t va) {
	return (va >> 12) & 0x3FF;
}

/* Helper macros */
uint32_t virt_to_phys(void *v) {
    return (uint32_t)v - KERNEL_BASE;
}
void *phys_to_virt(uint32_t p) {
    return (void *)(p + KERNEL_BASE);
}

extern uint32_t boot_page_directory;
extern uint32_t boot_page_table1;

uint32_t alloc_page(void) {
    static uint32_t phys_page_ptr = 0;
    uint32_t phys_kernel_end = (uint32_t)&_kernel_end - KERNEL_BASE;
    uint32_t heap_phys_end = phys_kernel_end + HEAP_SIZE;

    if (phys_page_ptr == 0) {
        /* Initialize to heap_start aligned up to 4KB */
        phys_page_ptr = (phys_kernel_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    }

    if (phys_page_ptr + PAGE_SIZE > heap_phys_end) {
        return 0; /* Out of memory */
    }

    uint32_t phys_alloc = phys_page_ptr;
    phys_page_ptr += PAGE_SIZE;

    uint32_t va = phys_alloc + KERNEL_BASE;
    /* Map the page in the boot page directory */

    uint32_t pd_idx = pd_index(va);
    uint32_t pt_idx = pt_index(va);

    uint32_t *pd = (uint32_t *)&boot_page_directory;
    // uint32_t *pt;

    if(!(pd[pd_idx] & 0x1)) {
        if(phys_page_ptr + PAGE_SIZE > heap_phys_end) {
            phys_page_ptr -= PAGE_SIZE;
            return 0; /* Out of memory */
        }
        uint32_t pt_phys = phys_page_ptr;
        phys_page_ptr += PAGE_SIZE;

        memset(phys_to_virt(pt_phys), 0, PAGE_SIZE);
        pd[pd_idx] = (pt_phys & 0xFFFFF000) | 0x003;
    }

    uint32_t pt_phys_entry = pd[pd_idx] & 0xFFFFF000;
    uint32_t *pt = (uint32_t *)(pt_phys_entry + KERNEL_BASE);

    
    pt[pt_idx] = (phys_alloc & 0xFFFFF000) | 0x003;

    /* zero the page for safety */
    memset((void *)va, 0, PAGE_SIZE);
    return va;
}
