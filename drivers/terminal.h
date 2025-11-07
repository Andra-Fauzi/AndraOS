#include <stdint.h>
#include <stdbool.h>
#pragma once 
#include "port_io.h"
#include "shell.h"
#include "multiboot_header.h"
#include "kernel_state.h"

void clear_line(int line);
void clear_screen(multiboot_info_t *mb_info);
void print_char(char c, multiboot_info_t *mb_info);
void kprint(char* str, multiboot_info_t *mb_info);
void draw_pixel(multiboot_info_t *mb_info, int x, int y, uint32_t color);
void terminal_init(multiboot_info_t *mb_info);
