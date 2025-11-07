#include <stdint.h>
#include "pic.h"
#include "port_io.h"

#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20

void pic_remap(int offset1, int offset2) {
	uint8_t a1 = inb(PIC1_DATA);
	uint8_t a2 = inb(PIC2_DATA);

	outb(PIC1_CMD, 0x11);
	outb(PIC2_CMD, 0x11);

	outb(PIC1_DATA, offset1);
	outb(PIC2_DATA, offset2);

	outb(PIC1_DATA, 0x01);
	outb(PIC2_DATA, 0x01);

	outb(PIC1_DATA, a1);
	outb(PIC2_DATA, a2);
}

void send_eoi(uint8_t irq) {
	if (irq >= 8) {
		outb(PIC2_CMD, PIC_EOI);
	}
	outb(PIC1_CMD, PIC_EOI);
}
