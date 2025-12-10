#pragma once

#include <stdint.h>
#include "memory.h"

extern void switch_to_userland(uint32_t entry, uint32_t stack);

void switchToUserland();
void syscall_handler(struct regs *r);
void init_userland();