#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <stdint.h>
#include "multiboot_header.h"
#include "terminal.h"
#include "memory.h"
#include "sleep.h"

extern void initTasking();

typedef struct {
    uint32_t eax, ebx, ecx, edx, esi, edi, esp, ebp, eip, eflags, cr3;
//	     0    4    8    12   16   20   24   28   32   36      40
} Registers;

typedef struct Task {
    Registers regs;
    struct Task *next;
} Task;

extern void initTasking();
extern void createTask(Task*, void(*)(), uint32_t, uint32_t*);

extern void yield(); // Switch task frontend
extern void switchTask(Registers *old, Registers *new); // The function which actually switches
extern multiboot_info_t *multiboot_info;
void doIt();

#endif // __SCHEDULER_H__
