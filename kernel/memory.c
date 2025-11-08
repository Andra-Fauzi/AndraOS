#include "memory.h"

extern uint8_t _end;

#define HEAP_SIZE 0x1000000

uint8_t *heap_start;
uint8_t *heap_end;
block_t *heap_head;
block_t *intial_block;

void heap_init() {
    heap_start = &_end;
    heap_end = heap_start + HEAP_SIZE;
    heap_head = NULL;
    intial_block = NULL;
    intial_block = (block_t *)heap_start;
    intial_block->size = HEAP_SIZE - sizeof(block_t);
    intial_block->free = true;
    intial_block->next = NULL;
    heap_head = intial_block;
}

void *malloc(size_t size) {
    if (size == 0) return NULL;

    // Align 4 byte
    size_t alignment = 4;
    size_t aligned_size = (size + (alignment - 1)) & ~(alignment - 1);

    block_t *current = heap_head;
    block_t *before = NULL;

    // First fit: cari blok bebas yang cukup besar
    while (current != NULL) {
        if (current->free && current->size >= aligned_size) {
            // Jika blok cukup besar, bisa split untuk mengurangi fragmentasi
            if (current->size >= aligned_size + sizeof(block_t) + alignment) {
                // Split block
                block_t *new_block = (block_t *)((uint8_t *)(current + 1) + aligned_size);
                new_block->size = current->size - aligned_size - sizeof(block_t);
                new_block->free = true;
                new_block->next = current->next;
                
                current->size = aligned_size;
                current->next = new_block;
            }
            
            // Tandai blok dipakai
            current->free = false;
            return (void*)(current + 1);
        }
        before = current;
        current = current->next;
    }

    // Jika tidak ada blok yang cocok, buat blok baru di akhir heap
    if (before == NULL) {
        // Heap kosong, gunakan initial block
        if (intial_block->free && intial_block->size >= aligned_size) {
            intial_block->free = false;
            return (void*)(intial_block + 1);
        }
        return NULL; // Tidak bisa alokasi
    }

    // Cari blok terakhir untuk extended heap
    block_t *last = heap_head;
    while (last->next != NULL) {
        last = last->next;
    }

    uint8_t *new_block_addr = (uint8_t *)last + sizeof(block_t) + last->size;
    
    // Cek apakah cukup space di heap
    if (new_block_addr + sizeof(block_t) + aligned_size > heap_end) {
        return NULL; // Out of memory
    }

    // Buat blok baru
    current = (block_t *)new_block_addr;
    current->size = aligned_size;
    current->free = false;
    current->next = NULL;
    last->next = current;

    return (void*)(current + 1);
}


void free(void *ptr) {
    if(ptr == NULL) return;
    block_t *block = (block_t *)ptr - 1;
    block->free = true;;

    if(block->next && block->next->free) {
	    block->size += sizeof(block_t) + block->next->size;
	    block->next = block->next->next;
    }

    block_t *current_block = heap_head;
    while(current_block && current_block->next != block) {
	    current_block = current_block->next;
    }
    if(current_block && current_block->free) {
	    current_block->size += sizeof(block_t) + block->size;
	    current_block->next = block->next;
    }
}

void print_end(multiboot_info_t* mb_info) {
    char buffer[256];
    kprint_hex((uintptr_t)&_end, mb_info);
    kprint("endnya : ", mb_info);
    kprint(buffer, mb_info);

}
