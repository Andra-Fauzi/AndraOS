#pragma once

#include "irq.h"
#include <stdbool.h>

uint16_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_scan_RTL8139();
