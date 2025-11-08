#include "keyboard.h"

#define KEYBOARD_BUFFER_SIZE 256

volatile uint8_t keyboard_buffer[KEYBOARD_BUFFER_SIZE];
volatile uint32_t kb_tail = 0;
volatile uint32_t kb_head = 0;

void keyboard_callback(struct regs *r) {

	uint8_t status = inb(0x64);

	if(status & 0x01) {
		uint8_t scancode = inb(0x60);
		if(scancode & 0x80)
			return;
		uint32_t next = (kb_head + 1) % KEYBOARD_BUFFER_SIZE;
		if(next != kb_tail) {
			keyboard_buffer[kb_head] = scancode;
			kb_head = next;
		}
	}
	send_eoi(1);
}

int keyboard_getchar() {
	if (kb_head == kb_tail) return -1;

	uint8_t scancode = keyboard_buffer[kb_tail];
	kb_tail = (kb_tail + 1) % KEYBOARD_BUFFER_SIZE;

	return scancode;
}

void init_keyboard() {
	register_irq_handler(1, keyboard_callback);
}
