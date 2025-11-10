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

void kernel_main(multiboot_info_t *mb_info) {
	if(mb_info == NULL) {
		for (;;) asm("hlt");
	}
	if(mb_info->framebuffer_addr == 0) {
		for (;;) asm("hlt");
	}
	multiboot_info = mb_info;
	gdt_install();
	idt_install();
	isr_install();
	irq_install();
    heap_init();
	init_paging();

	// asm volatile("sti");

	//extern void init_keyboard();
	//extern void init_timer(uint32_t frequency);

	//init_keyboard();
	//init_timer(100);



	//uint32_t *fb = (uint32_t *) mb_info->framebuffer_addr;
	//uint32_t width = mb_info->framebuffer_width;
	//uint32_t height = mb_info->framebuffer_height;

	/*
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			fb[y * (mb_info->framebuffer_pitch / 4) + x] = 0x000000ff;
		}
	}
	*/

	// draw_fullscreen(mb_info);
	/*
	for (uint32_t y = 0; y < 8; y++) {
		for(uint32_t x = 0; x < 8; x++) {
			draw_pixel(mb_info, x, y, 0x00ff0000);

		}
	}
	*/

	/* Map a virtual slot for the multiboot info and framebuffer, then
	 * update the kernel's multiboot_info to the virtual address so all
	 * callers use the mapped data. We capture the physical framebuffer
	 * address first (mb_info is still valid here) and map it to a
	 * virtual region starting at 0x20000000.
	 */
	uintptr_t mb_phys = (uintptr_t)mb_info;
	uintptr_t fb_phys = mb_info->framebuffer_addr;

	map_page(0x10000000, (uint32_t)mb_phys, 3);

	/* set global multiboot_info to the new virtual address */
	multiboot_info = (multiboot_info_t *)0x10000000;

	/* map framebuffer frames into virtual region 0x20000000 */
	for (int i = 0; i < 4096; i++) {
		map_page((i * 0x1000) + 0x20000000, (uint32_t)(fb_phys + (i * 0x1000)), 3);
	}

	// /* update the multiboot_info structure so its framebuffer_addr points
	//  * to the virtual mapping we just created. After this, drawing routines
	//  * using multiboot_info->framebuffer_addr will access the framebuffer
	//  * via the virtual address 0x20000000.
	//  
	multiboot_info->framebuffer_addr = (uintptr_t)0x20000000;

	// // char *sigma = "andra";
	// // kprint(sigma, multiboot_info);
	// extern uint8_t _end;
	// kprint_hex((uintptr_t)&_end, multiboot_info);
	// print_char('\n', multiboot_info);
	// kprint('0')
	// kprint("\n", mb_info);
	// kprint_hex((uintptr_t)buffer, mb_info);
	// kprint("\n", mb_info);
	//print_char('a', mb_info);

	//init_shell(mb_info);
	//
	kprint_hex((uintptr_t)multiboot_info->framebuffer_addr, multiboot_info);


	#define VIRTUALADDRESS 0xDEAD0000

	uintptr_t physaddress = (uintptr_t)0x00500000;
	
	for (int i = 0; i < 512; i++) {
		map_page((i * 0x1000) + 0x20000000, (uint32_t)(fb_phys + (i * 0x1000)), 3);
		map_page(VIRTUALADDRESS + (i * 0x1000), (uint32_t)(physaddress + (i * 0x1000)), 3);
	}


	char *andra = (char *)VIRTUALADDRESS;
	
	andra[0] = 'c';
	andra[1] = 'a';
	andra[2] = 'c';
	andra[3] = 'a';
	andra[4] = 'c';
	andra[5] = 'a';

	print_char('\n', multiboot_info);
	
	kprint((char *)VIRTUALADDRESS, multiboot_info);
	print_char('\n', multiboot_info);
	
	char *a = "jawa\n";
	
	kprint(a, multiboot_info);
	kprint_hex((uintptr_t)a, multiboot_info);
	
	// uint32_t pa;
	
	// uint32_t page_table = get_page_table(VIRTUALADDRESS);
	// uint32_t pt_index = (VIRTUALADDRESS >> 22) & 0x3FF;
	// pa = page_table[pt_index] & 0xFFFFF000;
	
	
	print_char('\n', multiboot_info);

	/*
	 * SAFE TEST (identity mapping while paging ON):
	 * Map a physical address to itself (virtual == physical) and
	 * then read/write it while paging is enabled. This lets you
	 * verify you can access the "physical" address without
	 * disabling paging.
	 */
	uintptr_t test_phys = 0x00500000; /* choose the physical address you want to test */

	/* map a single page identity (phys -> same virt) */
	map_page((uint32_t)test_phys, (uint32_t)test_phys, 3);

	/* Access via the physical numeric address (now also a valid virtual address)
	 * and write a small string, then print it using the normal kprint that
	 * writes to the (virtual) framebuffer.
	 */
	// volatile char *p = (volatile char *)test_phys;
	// p[0] = 'H';
	// p[1] = 'i';
	// p[2] = '\0';

	/* Print the string we just wrote at the physical address */
	kprint((char *)test_phys, multiboot_info);
	print_char('\n', multiboot_info);

	/* Also print the physical address for verification */
	kprint_hex((uintptr_t)test_phys, multiboot_info);
	print_char('\n', multiboot_info);

	/* run the test function that the linker placed into .func_mem via the
	 * section attribute. Use run_in_function_space so the code runs inside
	 * the isolated page directory with its own stack.
	 */
	// extern void test_function_isolation(void) __attribute__((section(".func_mem")));

	// int *adalah = (int *)malloc(sizeof(int) * 4);
	// adalah[0] = 1;
	// adalah[1] = 2;
	// adalah[2] = 3;
	// adalah[3] = 4;
	// kprint_hex((uintptr_t)adalah, multiboot_info);
	// print_char('\n', multiboot_info);
	// char *buffer = (char *)malloc(sizeof(char) * 4);
	// to_string(adalah[0], buffer);
	// kprint_hex((uintptr_t)buffer, multiboot_info);
	// print_char('\n', multiboot_info);
	// kprint(buffer, multiboot_info);
	// print_char('\n', multiboot_info);
	// kprint_hex((uintptr_t)&adalah[1], multiboot_info);
	// print_char('\n', multiboot_info);

	//uint32_t *test = (uint32_t *)0x00100000;
	//*test = 0xDEADBEEF;
	//char* test_buffer = (char *)malloc(256 * sizeof(char));
	//to_string((int)test, test_buffer);
	//kprint(test_buffer, mb_info);
	
	for (;;) {
		//shell_run(mb_info);
	}

}
