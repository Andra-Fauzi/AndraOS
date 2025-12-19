#pragma once

void pic_remap(int offset1, int offset2);
void send_eoi(uint8_t irq);
void PIC_disable();
void IRQ_clear_mask(uint8_t IRQline);
void IRQ_set_mask(uint8_t IRQline);
