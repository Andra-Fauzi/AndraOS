#include "elf.h"
#include "gdt.h"

extern uint16_t active_cluster;
extern multiboot_info_t *multiboot_info;
extern void switch_to_userland(uint32_t entry, uint32_t stack);

void *load_elf(uint16_t cluster, char name[12]) {
	int size = 0;
    uint8_t *file = readfile(cluster, name, &size);
    if (!file) return NULL;

    Elf32_Ehdr *eh = (Elf32_Ehdr*)file;

    // Cek magic
    if (eh->e_ident[0] != 0x7F || eh->e_ident[1] != 'E' ||
        eh->e_ident[2] != 'L' || eh->e_ident[3] != 'F')
        return NULL;

    Elf32_Phdr *ph = (Elf32_Phdr*)(file + eh->e_phoff);

	uint32_t alloc_test = alloc_page();

	("\n");
	kprint("test alloc : ");
	kprint_hex(alloc_test);
	kprint("\n");

    for(int i = 0; i < eh->e_phnum; i++) {
	    if(ph[i].p_type != 1) continue;
	    uint32_t vaddr = ph[i].p_vaddr;
        uint32_t memsz = ph[i].p_memsz;
        uint32_t filesz = ph[i].p_filesz;
        uint32_t offset = ph[i].p_offset;

        uint32_t page_start = vaddr & ~0xFFF;
        uint32_t page_end   = (vaddr + memsz + 0xFFF) & ~0xFFF;
	for(uint32_t addr = page_start; addr < page_end; addr += 0x1000) {
		uint32_t phys_mem = alloc_page();
		map_page(addr, virt_to_phys((void *)phys_mem), PTE_PRESENT | PTE_USER | PTE_WRITE);
	}
	memcpy((void *)vaddr, (void *)(file + offset), filesz);
	if(memsz > filesz) {
		memset((void *)(vaddr + filesz), 0, memsz - filesz);
	}
    }
    
    // Debug info
    kprint("entry: ");
    kprint_hex((uintptr_t)eh->e_entry);
    print_char('\n');

    return (void*)eh->e_entry;
}


void c_elf(char *buffer, int length) {
	asm volatile("cli");
	char buffer_s[512];
	char command_args[3][128];
	substr(buffer, ' ', length, command_args);
	char name[12];
	memset(name, ' ', 12);
	// kurang 4 karena command elf dan spasi nya
	memcpy(name, command_args[1], length-4);
	name[11] = '\0';
	void (*entry)() = load_elf(active_cluster, name);
	/*
	if(entry == NULL) {
		kprint("gak ada\n", multiboot_info);
		return;
	}
	*/
	// entry();
	asm volatile("sti");
	if (entry == NULL) return;

	entry();

	// we work on that later
	
	// // Allocate User Stack (8KB)
	// // We map it at 0xB0000000 (arbitrary user space address)
	// #define USER_STACK_VIRT 0xB0000000
	
	// uint32_t stack_page1 = alloc_page();
	// uint32_t stack_page2 = alloc_page();
	
	// if (!stack_page1 || !stack_page2) {
	// 	kprint("Stack allocation failed\n", multiboot_info);
	// 	return;
	// }

	// kprint("Allocated stack pages (kernel virt): ", multiboot_info);
	// kprint_hex(stack_page1, multiboot_info);
	// kprint(" ", multiboot_info);
	// kprint_hex(stack_page2, multiboot_info);
	// kprint("\n", multiboot_info);
	
	// kprint("Mapped user stack at: ", multiboot_info);
	// kprint_hex(USER_STACK_VIRT, multiboot_info);
	// kprint("\n", multiboot_info);

	// // Map the stack pages to user space
	// // Note: alloc_page returns kernel virtual address, so we convert to physical for mapping
	// map_page(USER_STACK_VIRT, virt_to_phys((void*)stack_page1), PTE_PRESENT | PTE_USER | PTE_WRITE);
	// map_page(USER_STACK_VIRT + 4096, virt_to_phys((void*)stack_page2), PTE_PRESENT | PTE_USER | PTE_WRITE);

	// // Setup Kernel Stack for Interrupts (TSS)
	// uint32_t kernel_stack = (uint32_t)alloc_page() + 4096;
	// if (kernel_stack == 4096) {
	// 	kprint("Kernel stack allocation failed\n", multiboot_info);
	// 	return;
	// }
	// set_kernel_stack(kernel_stack);
	// kprint("TSS Kernel Stack set to: ", multiboot_info);
	// kprint_hex(kernel_stack, multiboot_info);
	// kprint("\n", multiboot_info);

	// // Switch to userland with the new stack
	// // Stack grows down, so we start at the top of the second page
	// switch_to_userland((uint32_t)entry, USER_STACK_VIRT + 8192 - 16);
	// asm volatile("sti");
}
