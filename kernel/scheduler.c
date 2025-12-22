#include "scheduler.h"

volatile Task *runningTask;
volatile Task mainTask;
volatile Task secondTask;
volatile Task thirdTask;
volatile bool ismultitasking = false;

void secondMain() {
	kprint("Hello multitasking world!");
	while(1) {
		// kprint("task 2 running!\n", multiboot_info);
		// Timer interrupt will switch tasks automatically
		// for(volatile int i = 0; i < 10000000; i++); // Small delay to see output
        // shell_run(multiboot_info);
		// asm volatile("hlt");
        asm volatile("hlt");
	}
}

void thirdMain() {
    kprint("Hello multitasking world!");
    
	while(1) {
		// kprint("task 3 running!\n", multiboot_info);
		// Timer interrupt will switch tasks automatically
		// for(volatile int i = 0; i < 10000000; i++); // Small delay to see output
        asm volatile("hlt");
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
    ismultitasking = true;
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
    task->regs.esp = (uint32_t) malloc(4096) + 0x1000; // Not implemented here
    task->next = 0;
}

void yield() {
    Task *last = runningTask;
    runningTask = runningTask->next;
    switchTask(&last->regs, &runningTask->regs);
}

