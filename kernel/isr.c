#include "isr.h"
#include "syscall.h"

void isr_install() {
	idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
	idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
	idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
	idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
	idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
	idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
	idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
	idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
	idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
	idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
	idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
	idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
	idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
	idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
	idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
	idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
	idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
	idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
	idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
	idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
	idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
	idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
	idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
	idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
	idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
	idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
	idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
	idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
	idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
	idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
	idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
	idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
	idt_set_gate(128, (uint32_t)isr128, 0x08, 0x8E);
}

extern multiboot_info_t *multiboot_info;

bool error_isr = false;

void isr_handler(struct regs *r) {
	if (r->int_no == 128) {
		syscall_handler(r);
		return;
	}
	
	if (r->int_no == 14) {
		uint32_t cr2;
		asm volatile("mov %%cr2, %0" : "=r"(cr2));
		printf("\nPAGE FAULT (Int 14) at 0x%x\n", cr2);
		printf("EIP: 0x%x  Error Code: 0x%x\n", r->eip, r->err_code);
		printf("EFLAGS: 0x%x  CS: 0x%x\n", r->eflags, r->cs);
		
		uint32_t present = !(r->err_code & 0x1);
		uint32_t rw = r->err_code & 0x2;
		uint32_t us = r->err_code & 0x4;
		uint32_t reserved = r->err_code & 0x8;
		uint32_t id = r->err_code & 0x10;
		
		printf("Reason: %s %s %s %s %s\n",
			present ? "Page Not Present" : "Page Protection Violation",
			rw ? "Write" : "Read",
			us ? "User-mode" : "Kernel-mode",
			reserved ? "Reserved Bit violation" : "",
			id ? "Instruction Fetch" : ""
		);
		
		error_isr = true;
		for(;;) asm("hlt");
	}

	// kprint("interrupt terjadi");
	if(error_isr == false) {
		printf("Interrupt terjadi no: %d\n", r->int_no);
		printf("EIP: 0x%x\n", r->eip);
		error_isr = true;
	}
}
