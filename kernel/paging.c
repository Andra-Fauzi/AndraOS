#include "paging.h"

#define PAGE_SIZE 4096

extern uint8_t _end;
extern multiboot_info_t *multiboot_info;

uint32_t *page_table __attribute__((aligned(4096)));
uint32_t *page_directory __attribute__((aligned(4096)));

void map_page(uint32_t virt_address, uint32_t phys_address, uint32_t flags) {
}

void init_paging() {
    kprint("Using malloc for paging structures...\n", multiboot_info);

    // malloc di kernel pre-paging return PHYSICAL address
    uint32_t *page_table = (uint32_t *)malloc(4096);
    uint32_t *page_directory = (uint32_t *)malloc(4096);

    // Align to 4KB boundary
    page_table = (uint32_t *)((uint32_t)page_table & ~0xFFF);
    page_directory = (uint32_t *)((uint32_t)page_directory & ~0xFFF);

    memset(page_table, 0, 4096);
    memset(page_directory, 0, 4096);

    // **PENTING**: Identity map region dimana page structures berada
    uint32_t pd_phys = (uint32_t)page_directory;
    uint32_t pt_phys = (uint32_t)page_table;

    // Calculate indices untuk page tables
    uint32_t pd_index = pd_phys / (1024 * PAGE_SIZE);
    uint32_t pt_index = pt_phys / (1024 * PAGE_SIZE);

    // Identity map page directory dan page table themselves
    for(int i = 0; i < 1024; i++) {
        page_table[i] = (i * PAGE_SIZE) | 0x03; // Map semua 4MB pertama
    }

    // Page directory entry untuk first 4MB
    page_directory[0] = pt_phys | 0x03;

    // Juga map page directory dan page table sendiri jika di outside first 4MB
    if (pd_index > 0) {
        // Butuh additional page table untuk mapping ini
        // Complex - better stick dengan first 4MB
    }

    kprint("PD phys: ", multiboot_info);
    kprint_hex(pd_phys, multiboot_info);
    kprint("\nPT phys: ", multiboot_info);
    kprint_hex(pt_phys, multiboot_info);
    kprint("\n", multiboot_info);

    // Load CR3
    asm volatile("mov %0, %%cr3" :: "r"(pd_phys));

    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0));

    kprint("Paging enabled with malloc!\n", multiboot_info);
}
