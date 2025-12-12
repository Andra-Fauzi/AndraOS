#pragma once
#include <stdint.h>

extern volatile uint32_t timer_ticks;

void sleep_ticks(uint32_t ticks);
void sleep_ms(uint32_t ms);
