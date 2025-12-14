#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "terminal.h"
#include "keyboard.h"
#include "sleep.h"
#include "ata.h"
#include "kernel_state.h"
#include "multiboot_header.h"
#include "fat.h"
#include "tools_command/cd.h"
#include "tools_command/ls.h"
#include "elf.h"
#include "tools_command/cat.h"
#include "tools_command/create_file.h"
#include "tools_command/as.h"

typedef struct {
	char name[255];
	void (*function)(char *, int, multiboot_info_t *);
	int length_command;
} command_args_t;

extern int32_t terminal_x;
extern int32_t terminal_y;

void c_cd(char* buffer, int length, multiboot_info_t *mb_info);
void c_ls(char* buffer, int length, multiboot_info_t *mb_info);
void c_elf(char* buffer, int length, multiboot_info_t *mb_info);
void c_compile(char* buffer, int length, multiboot_info_t *mb_info);

/* Compiler interface */
int subc_compile(char *src, int src_len, char *dest, size_t dest_max, size_t *dest_len);

void init_shell(multiboot_info_t *mb_info);

char shell_getchar();

void shell_run(multiboot_info_t *mb_info);
