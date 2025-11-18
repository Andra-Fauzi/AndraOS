#include <stdint.h>
#include "irq.h"
#include "port_io.h"
#include "terminal.h"
#include "scheduler.h"

volatile uint32_t timer_ticks = 0;

extern multiboot_info_t *multiboot_info;
extern void switch_context(void);

void timer_callback(struct regs *r) {
	timer_ticks++;
	kprint(".", multiboot_info);
	//
}

void init_timer(uint32_t frequency) {
	register_irq_handler(0, timer_callback);

	uint32_t divisor = 1193180 / frequency;
	outb(0x43, 0x36);
	outb(0x40, divisor & 0xFF);
	outb(0x40, (divisor >> 8) & 0xFF);
}
