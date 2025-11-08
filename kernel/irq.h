#include <stdint.h>
#pragma once

#include "pic.h"
#include "port_io.h"
#include "terminal.h"
#include "idt.h"
#include "isr.h"

typedef void (*irq_callback_t)(struct regs*);
void register_irq_handler(int irq, irq_callback_t handler);
void unregister_irq_handler(int irq);
void irq_install();

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();
