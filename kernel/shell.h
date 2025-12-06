#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#pragma once

#include "terminal.h"
#include "keyboard.h"
#include "sleep.h"
#include "ata.h"
#include "kernel_state.h"
#include "multiboot_header.h"
#include "fat.h"
#include "tools_command/cd.h"
#include "tools_command/ls.h"

typedef struct {
	char name[255];
	void (*function)(char *, int, multiboot_info_t *);
	int length_command;
} command_args_t;

extern int32_t terminal_x;
extern int32_t terminal_y;

void init_shell(multiboot_info_t *mb_info);

char shell_getchar();

void shell_run();
