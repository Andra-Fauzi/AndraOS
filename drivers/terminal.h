#pragma once 
#include <stdint.h>
#include <stdbool.h>
#include "port_io.h"
#include "shell.h"
#include "multiboot_header.h"
#include "kernel_state.h"

//void clear_line(int line);
void clear_screen();
void print_char(char c);
void kprint(char* str);
void draw_pixel(int x, int y, uint32_t color);
void terminal_init();
void kprint_hex(uintptr_t value);
