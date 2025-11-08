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
	//init_paging();

	asm volatile("sti");

	extern void init_keyboard();
	extern void init_timer(uint32_t frequency);

	init_keyboard();
	init_timer(100);



	uint32_t *fb = (uint32_t *) mb_info->framebuffer_addr;
	uint32_t width = mb_info->framebuffer_width;
	uint32_t height = mb_info->framebuffer_height;

	/*
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			fb[y * (mb_info->framebuffer_pitch / 4) + x] = 0x000000ff;
		}
	}
	*/

	//draw_fullscreen(mb_info);
	/*
	for (uint32_t y = 0; y < 8; y++) {
		for(uint32_t x = 0; x < 8; x++) {
			draw_pixel(mb_info, x, y, 0x00ff0000);

		}
	}
	*/

	kprint("abcabcabcabcabcabcabcabcacbababcabcbabcabcabbcbabcbabcababcbcbacabcabbb\n", mb_info);
	print_char('a', mb_info);

	init_shell(mb_info);

	int *adalah = (int *)malloc(sizeof(int) * 4);
	adalah[0] = 1;
	adalah[1] = 2;
	adalah[2] = 3;
	adalah[3] = 4;
	kprint_hex((uintptr_t)adalah, mb_info);
	print_char('\n', mb_info);
	char *buffer = (char *)malloc(sizeof(char) * 4);
	to_string(adalah[0], buffer);
	kprint_hex((uintptr_t)buffer, mb_info);
	print_char('\n', mb_info);
	kprint(buffer, mb_info);
	print_char('\n', mb_info);
	kprint_hex((uintptr_t)&adalah[1], mb_info);
	print_char('\n', mb_info);

	//uint32_t *test = (uint32_t *)0x00100000;
	//*test = 0xDEADBEEF;
	//char* test_buffer = (char *)malloc(256 * sizeof(char));
	//to_string((int)test, test_buffer);
	//kprint(test_buffer, mb_info);
	
	for (;;) {
		shell_run(mb_info);
	}

}
