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
#include "tools_command/compile.h"
#include "tools_command/shutdown.h"
#include "tools_command/reboot.h"

typedef struct {
	char name[255];
	void (*function)(char *, int);
	int length_command;
} command_args_t;

extern int32_t terminal_x;
extern int32_t terminal_y;

/* Compiler interface */
int subc_compile(char *src, int src_len, char *dest, size_t dest_max, size_t *dest_len);

void init_shell();

char shell_getchar();

void shell_run();
