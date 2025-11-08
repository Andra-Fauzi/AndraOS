#include "paging.h"

#define PAGE_SIZE 4096

extern uint8_t _end;
extern multiboot_info_t *multiboot_info;

uint32_t *page_table __attribute__((aligned(4096)));
uint32_t *page_directory __attribute__((aligned(4096)));

void map_page(uint32_t virt_address, uint32_t phys_address, uint32_t flags) {
}

void init_paging() {
	page_table = (uint32_t *)malloc(4096);
	page_directory = (uint32_t *)malloc(4096);

	memset(page_table, 0, 4096);
	memset(page_directory, 0, 4096);

	for(int i = 0; i < 1024; i++) {
		page_table[i] = (i * 0x1000) | 1 | 2;
	}

	page_directory[0] = (((uint32_t)page_table)) | 1 | 2;

	kprint("about to enable paging", multiboot_info);

	asm volatile("mov %0, %%cr3" :: "r"(page_directory));
	uint32_t cr0;
	asm volatile("mov %%cr0, %0" : "=r"(cr0));
	cr0 |= 0x80000000;
	asm volatile("mov %0, %%cr0" :: "r"(cr0));
	kprint("paging enabled", multiboot_info);
}
