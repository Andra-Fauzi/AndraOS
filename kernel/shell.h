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

extern int64_t terminal_x;
extern int64_t terminal_y;

void init_shell(multiboot_info_t *mb_info);

char shell_getchar();

void shell_run();
