#include <stdint.h>
#pragma once

#include "port_io.h"
#include "sleep.h"


int ata_read_sector(uint32_t lba, uint8_t* buffer);
int ata_write_sector(uint32_t lba, uint8_t* buffer);
uint32_t ata_get_total_sectors();
char* char_total_sectors();
