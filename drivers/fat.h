#pragma once

#include <stdint.h>
#include <stddef.h>
#include "util.h"
#include "terminal.h"
#include "ata.h"

typedef struct fat_BS {
	//				offset
	uint8_t 	bootjmp[3]; // 0
	uint8_t		oem_name[8]; // 3
	uint16_t	bytes_per_sector; // 11
	uint8_t		sector_per_cluster; // 13
	uint16_t	reserved_sector_count; // 14
	uint8_t		table_count; // 16
	uint16_t	root_entry_count; // 17
	uint16_t	total_sector_16; // 19
	uint8_t		media_type; // 21
	uint16_t	table_size_16; // 22
	uint16_t	sectors_per_track; // 24
	uint16_t	head_side_count; // 26
	uint32_t	hidden_sector_count; // 28
	uint32_t	total_sectors_32; // 32
} __attribute__((packed)) fat_BS_t;

typedef struct fat_extBS_16{
	uint8_t		drive_number; // 36
	uint8_t		reserved1; // 37
	uint8_t		boot_signature; // 38 must be 0x28 or 29
	uint32_t	svolume_id;
	uint8_t		volume_label[11];
	uint8_t		fat_type_label[8];
} __attribute__((packed)) fat_extBS_16_t;

typedef struct __attribute__((packed)) fat_dir_entry {
    uint8_t name[11];              // offset 0x00, 11 byte (8+3)
    uint8_t attr;                  // offset 0x0B, 1 byte
    uint8_t reserved;              // offset 0x0C, 1 byte
    uint8_t create_time_tenth;     // offset 0x0D, 1 byte (tenths of second)
    uint16_t create_time;          // offset 0x0E, 2 byte
    uint16_t create_date;          // offset 0x10, 2 byte
    uint16_t last_access_date;     // offset 0x12, 2 byte
    uint16_t first_cluster_hi;     // offset 0x14, 2 byte (0 for FAT16)
    uint16_t write_time;            // offset 0x16, 2 byte (last modified time)
    uint16_t write_date;            // offset 0x18, 2 byte (last modified date)
    uint16_t first_cluster_lo;     // offset 0x1A, 2 byte
    uint32_t file_size;            // offset 0x1C, 4 byte
} fat_dir_entry_t;


fat_BS_t parse_BS(const char *buffer);

void init_fat16();

uint16_t readFATTable(uint16_t active_cluster);

uint32_t cluster_to_LBA(uint16_t cluster);

void list_root_dir();

void list_dir(uint16_t active_cluster);

void readFATuntilEOC(uint16_t active_cluster);
