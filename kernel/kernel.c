#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "terminal.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "sleep.h"
#include "ata.h"
#include "shell.h"
#include "multiboot_header.h"
#include "memory.h"
#include "paging.h"
#include "isolation.h"
#include "scheduler.h"
#include "fat.h"
#include "userland.h"
#include "apic.h"
#include "PCI.h"
#include "acpi.h"
#include <uacpi/uacpi.h>

#if defined(__linux__)
#error "You are not using cross compiler you will run to some trouble"
#endif

#if !defined(__i386)
#error "this kernel need compile with ix86-elf compiler"
#endif

// old deprecated VGA not using this anymore
//#define VGA_WIDTH 80
//#define VGA_HEIGHT 25
//#define VGA_MEMORY 0XB8000


multiboot_info_t *multiboot_info;

extern uint8_t _end;

uint32_t framebuffer_address;

void kernel_main(multiboot_info_t *mb_info) {
	// mb_info comes from bootloader at a low physical address (e.g., 0x10000)
	// We need to map the first 4MB back for multiboot access since boot.asm unmapped it
	
	// Re-establish identity mapping for first 4MB (1024 pages) to access multiboot info
	for (uint32_t i = 0; i < 1024; i++) {
		map_page(i * 0x1000, i * 0x1000, PTE_KERNEL_RW | PTE_PRESENT);
	}
	
	// Now mb_info should be accessible at its original address
	multiboot_info = mb_info;
	
	// Map the framebuffer to high memory for easier access
	uint32_t fb_phys = (uint32_t)multiboot_info->framebuffer_addr;
	uint32_t fb_virt = 0xC2000000; // virtual address for framebuffer

	uint32_t fb_size = multiboot_info->framebuffer_width * multiboot_info->framebuffer_height * (multiboot_info->framebuffer_bpp / 8);
	uint32_t fb_pages = (fb_size + 0xFFF) / 0x1000; // round up to nearest page

	for (uint32_t i = 0; i < fb_pages; i++) {
		map_page(fb_virt + (i * 0x1000), fb_phys + (i * 0x1000), PTE_KERNEL_RW | PTE_PRESENT);
	}

	// Update framebuffer address to use the mapped virtual address
	framebuffer_address = fb_virt;

	//multiboot_info = mb_info;
	kprint("Init GDT\n");
	gdt_install();
	kprint("Init IDT\n");
	idt_install();
	kprint("Init ISR\n");
	isr_install();
	kprint("Init HEAP\n");
	heap_init();
	kprint("Init IRQ\n");
	irq_install();
	kprint("Init Timer\n");
	extern void init_timer(uint32_t frequency);
	init_timer(10);
	kprint("Enable Interrupt\n");
	asm volatile("sti");
	// Inisialisasi uACPI library
	// asm volatile("cli");
	// kprint("Init APIC\n");
	// init_apic();
	// asm volatile("sti");
	//initTasking();
	//init_paging();
	

	// extern void init_keyboard();
	
	// kprint("Init Keyboard\n");
	// init_keyboard();

	kprint("Init Shell\n");
	init_shell();
	
	kprint("Init FAT16\n");
	init_fat16();
	// kprint("test network\n");
	// pci_scan_RTL8139();
	// kprint("\n");
	
	// Disable interrupts selama uACPI initialization
	// karena parsing AML tables tidak re-entrant safe
	kprint("Init uACPI...\n");
	asm volatile("cli");
	
	uacpi_status status = uacpi_initialize(0);
	if (status == UACPI_STATUS_OK) {
		kprint("uACPI initialized successfully!\n");
		
		// Load ACPI namespace (parse AML tables)
		status = uacpi_namespace_load();
		if (status == UACPI_STATUS_OK) {
			kprint("ACPI namespace loaded!\n");
			
			// Initialize ACPI namespace (execute _INI, _STA methods, etc)
			status = uacpi_namespace_initialize();
			if (status == UACPI_STATUS_OK) {
				kprint("ACPI namespace initialized!\n");
			} else {
				kprint("ACPI namespace init failed: ");
				kprint_hex(status);
				kprint("\n");
			}
		} else {
			kprint("ACPI namespace load failed: ");
			kprint_hex(status);
			kprint("\n");
		}
	} else {
		kprint("uACPI init failed: ");
		kprint_hex(status);
		kprint("\n");
	}
	
	// Re-enable interrupts setelah uACPI selesai
	asm volatile("sti");
	kprint("Interrupts re-enabled\n");
	kprint("run testacpi\n");
	testacpi();
	
	
	for (;;) {
		shell_run();
		asm volatile("hlt");
		//kprint("main task running\n", multiboot_info);
		//sleep_ms(1000);
		//yield();
	}

}
