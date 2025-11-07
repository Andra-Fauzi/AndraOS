#include "ata.h"

int ata_wait_busy() {
	uint32_t t = 100000000;
	while(inb(0x1F7) & 0x80){
		if(--t == 0) return -1;
	}
}

int ata_wait_drq() {
	uint32_t t = 100000000;
	while(!(inb(0x1F7) & 0x08)){
		if(--t == 0) return -1;
	}
}

int ata_read_sector(uint32_t lba, uint8_t* buffer) {
	const uint32_t TIMEOUT = 10000000;
	uint32_t t;

	t = TIMEOUT;

	while(inb(0x1F7) & 0x80) {
		if(--t == 0) return -1;
	}

	ata_wait_busy();

	outb(0x1F2, 1);
	outb(0x1F3, (uint8_t)(lba & 0xFF));
	outb(0x1F4, (uint8_t)((lba >> 8) & 0xFF));
	outb(0x1F5, (uint8_t)((lba >> 16) & 0xFF));
	outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
	outb(0x1F7, 0x20);

	ata_wait_busy();
	ata_wait_drq();

	for(int i = 0; i < 256; i++) {
		uint16_t data = inw(0x1F0);
		buffer[i*2] = data & 0xFF;
		buffer[i*2+1] = (data >> 8) & 0xFF;
	}
	return 0;
}

int ata_write_sector(uint32_t lba, uint8_t* buffer) {
	const uint32_t TIMEOUT = 10000000;
	uint32_t t;

	t = TIMEOUT;

	while(inb(0x1F7) & 0x80) {
		if(--t == 0) return -1;
	}
	ata_wait_busy();

	outb(0x1F2, 1);
	outb(0x1F3, (uint8_t)(lba & 0xFF));
	outb(0x1F4, (uint8_t)((lba >> 8) & 0xFF));
	outb(0x1F5, (uint8_t)((lba >> 16) & 0xFF));
	outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
	outb(0x1F7, 0x30);

	ata_wait_busy();
	ata_wait_drq();

	for (int i = 0; i < 256; i++) {
		uint16_t data = buffer[i*2] | (buffer[i*2+1] << 8);
		outw(0x1F0, data);
	}
	outb(0x1F7, 0xE7);
	ata_wait_busy();
	return 0;
}

uint32_t ata_get_total_sectors() {
	outb(0x1F6, 0xA0);
	outb(0x1F2, 0);
	outb(0x1F4, 0);
	outb(0x1F5, 0);
	outb(0x1F7, 0xEC);

	uint8_t status = inb(0x1F7);
	if (status == 0) return 0;

	while((status & 0x80) || !(status & 0x08)) {
		status = inb(0x1F7);
	}

	uint16_t data[256];
	for (int i = 0; i < 256; i++) {
		data[i] = inw(0x1F0);
	}

	uint32_t total_sectors = data[60] | ((uint32_t)data[61] << 16);

	return total_sectors;
}

char* char_total_sectors() {
	uint32_t total = ata_get_total_sectors();

	static char chartotal[256];
	int i = 0;

	// Kalau total = 0, langsung simpan '0'
	if (total == 0) {
    		chartotal[i++] = '0';
	}
	else {
    		while (total != 0) {
        		chartotal[i++] = '0' + (total % 10);
        		total /= 10;
    		}
	}

	// Tambahkan null terminator
	chartotal[i] = '\0';

	// Balikkan string (karena hasilnya kebalik)
	for (int j = 0; j < i / 2; j++) {
    		char tmp = chartotal[j];
    		chartotal[j] = chartotal[i - j - 1];
    		chartotal[i - j - 1] = tmp;
	}

	// Sekarang chartotal berisi angka yang benar
	return chartotal;
}
