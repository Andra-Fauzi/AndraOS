#pragma once

#include "irq.h"
#include "pic.h"

bool check_apic();

void init_apic();

void lapic_eoi();
void ioapic_enable_keyboard();
