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

#if defined(__linux__)
#error "You are not using cross compiler you will run to some trouble"
#endif

#if !defined(__i386)
#error "this kernel need compile with ix86-elf compiler"
#endif

//#define VGA_WIDTH 80
//#define VGA_HEIGHT 25
//#define VGA_MEMORY 0XB8000

void draw_fullscreen(multiboot_info_t *mb_info) {
    if (!mb_info || mb_info->framebuffer_addr == 0) {
        return;
    }

    uint8_t bpp = mb_info->framebuffer_bpp;
    uint32_t pitch = mb_info->framebuffer_pitch;
    uint32_t width = mb_info->framebuffer_width;
    uint32_t height = mb_info->framebuffer_height;
    uintptr_t fb_addr = (uintptr_t) mb_info->framebuffer_addr;

    /* contoh: fill blue then red rectangle */
    if (bpp == 32) {
        uint32_t *fb = (uint32_t *) fb_addr;
        uint32_t pitch_pixels = pitch / 4;
        /* fill blue */
        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                fb[y * pitch_pixels + x] = 0x000000FF; /* 0x00RRGGBB little-endian = Blue */
            }
        }
        /* draw red rectangle 50..100,50..150 */
        for (uint32_t y = 50; y < 100 && y < height; y++) {
            for (uint32_t x = 50; x < 150 && x < width; x++) {
                fb[y * pitch_pixels + x] = 0x00FF0000; /* Red */
            }
        }
    } else if (bpp == 24) {
        uint8_t *fb = (uint8_t *) fb_addr;
        for (uint32_t y = 0; y < height; y++) {
            uint8_t *row = fb + y * pitch;
            for (uint32_t x = 0; x < width; x++) {
                uint8_t *pix = row + x * 3;
                pix[0] = 0xFF; /* Blue */
                pix[1] = 0x00; /* Green */
                pix[2] = 0x00; /* Red  => blue color because BGR order in many implementations */
            }
        }
        /* rectangle */
        for (uint32_t y = 50; y < 100 && y < height; y++) {
            uint8_t *row = fb + y * pitch;
            for (uint32_t x = 50; x < 150 && x < width; x++) {
                uint8_t *pix = row + x * 3;
                pix[0] = 0x00; pix[1] = 0x00; pix[2] = 0xFF; /* Red if RGB order, or adjust if BGR */
            }
        }
    } else {
        /* jenis framebuffer lain atau paletted; kamu harus tangani sesuai framebuffer_type */
    }
}

multiboot_info_t *multiboot_info;

extern uint8_t _end;

uint32_t framebuffer_address;

void kernel_main(multiboot_info_t *mb_info) {
	/*map_page((uint32_t)0xC1000000, (uint32_t)mb_info, 3);
	for(int i = 0; i < 1024; i++) {
		map_page((uint32_t)(0xC2000000 + (i * 0x1000)), (uint32_t)(mb_info->framebuffer_addr + (i * 0x1000)), 3);
	}
	multiboot_info = (multiboot_info_t *)0xC1000000;
	multiboot_info->framebuffer_addr = 0xC2000000;
	*/
	
	// multiboot_info_t *mb_new = phys_to_virt((uint32_t)mb_info);
	// lili = (uint32_t)mb_new->framebuffer_addr;
	// multiboot_info->framebuffer_addr = lili;

	uint32_t mb_info_phys = (uint32_t)mb_info;
	uint32_t mb_info_virt = 0xC1000000; // virtual address to map multiboot info

	map_page(mb_info_virt, mb_info_phys, PTE_KERNEL_RW | PTE_PRESENT);
	multiboot_info = (multiboot_info_t *)mb_info_virt;

	uint32_t fb_phys = (uint32_t)multiboot_info->framebuffer_addr;
	uint32_t fb_virt = 0xC2000000; // virtual address to

	uint32_t fb_size = multiboot_info->framebuffer_width * multiboot_info->framebuffer_height * (multiboot_info->framebuffer_bpp / 8);
	uint32_t fb_pages = (fb_size + 0xFFF) / 0x1000; // round up to nearest page

	for (uint32_t i = 0; i < fb_pages; i++) {
		map_page(fb_virt + (i * 0x1000), fb_phys + (i * 0x1000), PTE_KERNEL_RW | PTE_PRESENT);
	}

	// for(uintptr_t i = 0; i < 0xC1000000; i += 0x1000) {
	// 	map_page(i, i, PTE_KERNEL_RW | PTE_PRESENT);
	// }

	// map_page(fb_virt, fb_phys, PTE_KERNEL_RW | PTE_PRESENT);
	multiboot_info->framebuffer_addr = fb_virt;
	framebuffer_address = fb_virt;

	kprint("halo", multiboot_info);
	/*if(mb_info == NULL) {
		for (;;) asm("hlt");
	}
	if(mb_info->framebuffer_addr == 0) {
		for (;;) asm("hlt");
	}
	*/
	//multiboot_info = mb_info;
	gdt_install();
	idt_install();
	isr_install();
	irq_install();
    	//heap_init();
	//init_paging();

	asm volatile("sti");

	extern void init_keyboard();
	extern void init_timer(uint32_t frequency);

	init_keyboard();
	init_timer(10);



	initTasking();

	
	//init_shell(multiboot_info);
	for (;;) {
		//shell_run(multiboot_info);
		// asm volatile("hlt");
		kprint("main task running\n", multiboot_info);
		sleep_ms(1000);
		yield();
	}

}
