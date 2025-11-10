#include "paging.h"
#include <stdint.h>
#include "util.h"
#include "terminal.h"

#define PAGE_SIZE 4096

extern multiboot_info_t *multiboot_info;
extern uint8_t _end; // akhir kernel

// Page directory & first page table statis, aligned 4KB
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

void reserve_physical_frame(uint32_t physical_address) {
}

void invalidate_tlb(uint32_t virtual_address) {
    asm volatile("invlpg (%0)" :: "r" (virtual_address) : "memory");
}

uint32_t get_physical_address(uint32_t virtual_address) {
    uint32_t dir_index   = (virtual_address >> 22) & 0x3FF;
    uint32_t table_index = (virtual_address >> 12) & 0x3FF;

    uint32_t pde = page_directory[dir_index];       // entry di page directory
    uint32_t *page_table = (uint32_t*)(pde & 0xFFFFF000); // base address PT

    uint32_t pte = page_table[table_index];        // ambil PTE
    uint32_t physical_address = pte & 0xFFFFF000; // frame address

    return physical_address;
}


// uint32_t *get_page_table(uint32_t virtual_address) {
//     uint32_t *page_table;
//     uint32_t dir_index = (virtual_address >> 22) & 0x3FF;
//     uint32_t table_index = (virtual_address >> 12) & 0x3FF;

//     page_table = page_directory[dir_index];
//     return page_table;
// }

void map_page_custom_page_dir(uint32_t *page_dir, uint32_t virtual_address, uint32_t physical_address, uint32_t flags) {
    uint32_t table_index = (virtual_address >> 12) & 0x3FF;
    uint32_t dir_index = (virtual_address >> 22) & 0x3FF;
    uint32_t pd_entry = page_dir[dir_index];
    uint32_t *page_table;

    // Jika belum ada page table untuk directory ini, alokasikan satu
    if ((pd_entry & 0x1) == 0) {
        // gunakan alloc_page dari memory.c untuk mendapatkan page 4KB-aligned
        uint32_t new_page_table = alloc_page();
        if (new_page_table == 0) {
            // out of memory saat membuat page table -> hentikan (atau tangani sesuai kebutuhan)
            for(;;) asm("hlt");
        }

        // Set entry directory -> page table (Present + RW)
        page_dir[dir_index] = new_page_table | flags;
        page_table = (uint32_t*)new_page_table;
    } else {
        // Ambil alamat page table dari directory entry
        page_table = (uint32_t*)(pd_entry & ~0xFFF);
    }
    
    // Set entry page table -> frame fisik (Present + RW)
    page_table[table_index] = (physical_address & ~0xFFF) | flags;
    
    // flush TLB untuk alamat virtual itu supaya CPU memakai mapping baru
    asm volatile("invlpg (%0)" :: "r" (virtual_address) : "memory");
}

void map_page(uint32_t virtual_address, uint32_t physical_address, uint32_t flags) {
    uint32_t table_index = (virtual_address >> 12) & 0x3FF;
    uint32_t dir_index = (virtual_address >> 22) & 0x3FF;
    uint32_t pd_entry = page_directory[dir_index];
    uint32_t *page_table;

    // Jika belum ada page table untuk directory ini, alokasikan satu
    if ((pd_entry & 0x1) == 0) {
        // gunakan alloc_page dari memory.c untuk mendapatkan page 4KB-aligned
        uint32_t new_page_table = alloc_page();
        if (new_page_table == 0) {
            // out of memory saat membuat page table -> hentikan (atau tangani sesuai kebutuhan)
            for(;;) asm("hlt");
        }

        // Set entry directory -> page table (Present + RW)
        page_directory[dir_index] = new_page_table | 3;
        page_table = (uint32_t*)new_page_table;
    } else {
        // Ambil alamat page table dari directory entry
        page_table = (uint32_t*)(pd_entry & ~0xFFF);
    }
    
    // Set entry page table -> frame fisik (Present + RW)
    page_table[table_index] = (physical_address & ~0xFFF) | 3;
    
    // flush TLB untuk alamat virtual itu supaya CPU memakai mapping baru
    asm volatile("invlpg (%0)" :: "r" (virtual_address) : "memory");
}

void init_paging() {
    // Bersihkan semua entry
    memset(page_directory, 0, sizeof(page_directory));
    memset(first_page_table, 0, sizeof(first_page_table));

    // Identity mapping untuk 0x00000000 � 4MB
    
    for (uint32_t i = 0; i < 1024; i++) {
        first_page_table[i] = (i * PAGE_SIZE) | 3; // Present + RW
    }

    // Pasang first page table ke page directory
    page_directory[0] = ((uint32_t)first_page_table) | 3; // Present + RW

    // Load CR3
    asm volatile("mov %0, %%cr3" :: "r"(page_directory));

    // Aktifkan paging dengan set PG bit di CR0
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // set PG bit
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}


void disable_paging() {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}
