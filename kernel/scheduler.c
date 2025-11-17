#include "scheduler.h"

static Task *runningTask;
static Task mainTask;
static Task secondTask;
static Task thirdTask;

static void secondMain() {
    kprint("Hello multitasking world!", multiboot_info); // Not implemented here...
	while(1) {
		kprint("task 2 running!\n", multiboot_info);
		sleep_ms(1000);
    		yield();
	}
}

static void thirdMain() {
    kprint("Hello multitasking world!", multiboot_info); // Not implemented here...
	while(1) {
		kprint("task 3 running!\n", multiboot_info);
		sleep_ms(1000);
    		yield();
	}
}

void initTasking() {
    // Get EFLAGS and CR3
    asm volatile("movl %%cr3, %%eax; movl %%eax, %0;":"=m"(mainTask.regs.cr3)::"%eax");
    asm volatile("pushfl; movl (%%esp), %%eax; movl %%eax, %0; popfl;":"=m"(mainTask.regs.eflags)::"%eax");

    createTask(&secondTask, secondMain, mainTask.regs.eflags, (uint32_t*)mainTask.regs.cr3);
    createTask(&thirdTask, thirdMain, mainTask.regs.eflags, (uint32_t*)mainTask.regs.cr3);
    mainTask.next = &secondTask;
    secondTask.next = &thirdTask;
    thirdTask.next = &mainTask;

    runningTask = &mainTask;
}

void createTask(Task *task, void (*main)(), uint32_t flags, uint32_t *pagedir) {
    task->regs.eax = 0;
    task->regs.ebx = 0;
    task->regs.ecx = 0;
    task->regs.edx = 0;
    task->regs.esi = 0;
    task->regs.edi = 0;
    task->regs.eflags = flags;
    task->regs.eip = (uint32_t) main;
    task->regs.cr3 = (uint32_t) pagedir;
    task->regs.esp = (uint32_t) alloc_page() + 0x1000; // Not implemented here
    task->next = 0;
}

void yield() {
    Task *last = runningTask;
    runningTask = runningTask->next;
    switchTask(&last->regs, &runningTask->regs);
}

