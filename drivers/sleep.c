#include "sleep.h"

void sleep_ticks(uint32_t ticks) {
	uint32_t start = timer_ticks;
	while(timer_ticks - start < ticks) {
		asm volatile("hlt");
	}
}

void sleep_ms(uint32_t ms) {
	uint32_t ticks_needed = ms / 10;
	if (ticks_needed == 0) ticks_needed = 1;
	sleep_ticks(ticks_needed);
}
