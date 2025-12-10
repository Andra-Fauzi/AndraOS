#include "gdt.h"
// #include <string.h>

// GDT entries: 0=null, 1=kernel code, 2=kernel data, 3=user code, 4=user data, 5=TSS
struct gdt_entry gdt[6];
struct gdt_ptr gp;

// TSS for task switching
struct tss_entry tss;

extern void gdt_flush(uint32_t);

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
	gdt[num].base_low = (base & 0xFFFF);
	gdt[num].base_middle = (base >> 16) & 0xFF;
	gdt[num].base_high = (base >> 24) & 0xFF;

	gdt[num].limit_low = (limit & 0xFFFF);
	gdt[num].granularity = ((limit >> 16) & 0x0F);
	gdt[num].granularity |= (gran & 0xF0);
	gdt[num].access = access;
}

void tss_install(uint32_t idx, uint32_t kss, uint32_t kesp) {
	uint32_t base = (uint32_t)&tss;
	uint32_t limit = sizeof(tss);

	// Clear TSS
	memset(&tss, 0, sizeof(tss));

	// Set kernel stack
	tss.ss0 = kss;
	tss.esp0 = kesp;

	// Set segments
	tss.cs = 0x0B;  // Kernel code segment with RPL=3
	tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x13;  // Kernel data segment with RPL=3

	// Add TSS descriptor to GDT
	gdt_set_gate(idx, base, limit, 0xE9, 0x00);  // 0xE9 = Present, Ring 3, TSS

	// Load TSS
	asm volatile("ltr %%ax" : : "a"(idx * 8));
}

void set_kernel_stack(uint32_t stack) {
	tss.esp0 = stack;
}

void gdt_install(void) {
	gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
	gp.base = (uint32_t)&gdt;

	gdt_set_gate(0, 0, 0, 0, 0);                    // Null descriptor
	gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);     // Kernel code
	gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);     // Kernel data
	gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);     // User code
	gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);     // User data
	
	gdt_flush((uint32_t)&gp);
	
	// Install TSS (entry 5)
	tss_install(5, 0x10, 0);  // Kernel data segment 0x10, stack will be set later
}
