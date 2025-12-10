#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <stdint.h>
#include "isr.h"
#include "terminal.h"

void syscall_handler(struct regs *r);
void init_syscalls();

#endif
