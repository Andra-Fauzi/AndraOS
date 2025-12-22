#include "PCI.h"

extern multiboot_info_t *multiboot_info;

uint16_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
	uint32_t address;
	uint32_t lbus = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;
	uint16_t tmp = 0;

	address = (uint32_t)((lbus << 16) | (lslot << 11) |
			(lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
	outl(0xCF8, address);

	tmp = (uint16_t)((inl(0XCFC) >> ((offset & 2) * 8)) & 0xFFFF);
	return tmp;
}

void pciConfigWriteWord(
    uint8_t bus,
    uint8_t slot,
    uint8_t func,
    uint8_t offset,
    uint16_t value
) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    address = (uint32_t)(
        (lbus << 16) |
        (lslot << 11) |
        (lfunc << 8) |
        (offset & 0xFC) |
        0x80000000
    );

    // Tulis alamat config
    outl(0xCF8, address);

    // Baca data 32-bit lama
    uint32_t data = inl(0xCFC);

    // Tentukan posisi word (lower / upper)
    uint32_t shift = (offset & 2) * 8;

    // Hapus word lama
    data &= ~(0xFFFF << shift);

    // Masukkan word baru
    data |= ((uint32_t)value << shift);

    // Tulis balik
    outl(0xCFC, data);
}


uint16_t pciCheckVendor(uint8_t bus, uint8_t slot) {
	uint16_t vendor, device;
	if ((vendor = pciConfigReadWord(bus, slot, 0, 0)) != 0xFFFF) {
		device = pciConfigReadWord(bus, slot, 0, 2);
	}
	return (vendor);
}

void pci_scan_simple() {
    for (uint8_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint16_t vendor = pciConfigReadWord(bus, slot, 0, 0);

            if (vendor != 0xFFFF) {
                uint16_t device = pciConfigReadWord(bus, slot, 0, 2);

                // device ditemukan
                // print / simpan vendor & device
            }
        }
    }
}

// testing not real using it

void pci_scan_RTL8139() {
	uint8_t rtl_bus, rtl_slot, rtl_func;
	uint16_t rtl_io_base;
	bool found = false;
    for (uint8_t bus = 0; bus < 256; bus++) {
		if(found) {
			break;
		}
        for (uint8_t slot = 0; slot < 32; slot++) {
		if(found) {
			break;
		}
		for(uint8_t func = 0; func < 8; func++) {
            	uint16_t vendor = pciConfigReadWord(bus, slot, func, 0);

            if (vendor != 0xFFFF) {
                uint16_t device = pciConfigReadWord(bus, slot, func, 2);

		if(vendor == 0x10EC && device == 0x8139) {
			found = true;
			rtl_bus = bus;
			rtl_slot = slot;
			rtl_func = func;
			break;
		}

                // device ditemukan
                // print / simpan vendor & device
		}
            }
        }
    }
    if(found == true) {
	    kprint("dapat nih network\n", multiboot_info);
    } else {
	    kprint("yah ga dapat nih network\n", multiboot_info);
	    return;
    }

    uint32_t bar0 = 
	    pciConfigReadWord(rtl_bus, rtl_slot, rtl_func, 0x10) |
	    (pciConfigReadWord(rtl_bus, rtl_slot, rtl_func, 0x12) << 16);
    rtl_io_base = bar0 & 0xFFFC;

    uint16_t cmd = 
	    pciConfigReadWord(rtl_bus, rtl_slot, rtl_func, 0x04);
    cmd |= 0x0001; // i/o space
    cmd |= 0x0004; // Bus master
    
    pciConfigWriteWord(rtl_bus, rtl_slot, rtl_func, 0x04, cmd);

    outb(rtl_io_base + 0x37, 0x10);
    while (inb(rtl_io_base + 0x37) & 0x10);

    uint8_t mac[6];
    for(int i = 0; i < 6; i++) {
	    mac[i] = inb(rtl_io_base + i);
    }

    kprint("mac address cihuyyy\n", multiboot_info);

    for(int i = 0; i < 6; i++) {
	    /*
	    char buffer[255];
	    to_string(mac[i], buffer);
	    kprint(buffer, multiboot_info);
	    if(i < 5) {
	    	kprint(":", multiboot_info);
	    }
	*/
	kprint_hex(mac[i], multiboot_info);
	if(i < 5) {
	    kprint(":", multiboot_info);
	}
    }
	    kprint("\n", multiboot_info);
	#define RX_BUF_SIZE (8192 + 16 + 1500)
	uint8_t rtl_rx_buffer[RX_BUF_SIZE] __attribute__((aligned(16)));
	outl(rtl_io_base + 0x30, (uint32_t)rtl_rx_buffer);
	outl(rtl_io_base + 0x44, 0x0000000F);
	outb(rtl_io_base + 0x37, 0x0C);
	
	uint16_t rx_offset = 0;

	uint16_t status = *(uint16_t *)(rtl_rx_buffer + rx_offset);
	uint16_t length = *(uint16_t *)(rtl_rx_buffer + rx_offset + 2);

	uint8_t *packet = rx_buffer + rx_offset + 4;

	rx_offset = (rx_offset + length + 4 + 3) & ~3;
	outw(rtl_io_base + 0x38, rx_offset - 16);

	uint8_t tx_buffer[4][1536] __attribute__((aligned(16)));
	int tx_cur = 0;

	uint8_t *pkt = tx_buffer[tx_cur];

	// dst MAC: broadcast
	memset(pkt, 0xFF, 6);

	// src MAC
	memset(pkt + 6, mac, 6);

	// ethertype = ARP
	pkt[12] + 0x08;
	pkt[13] + 0x06;

	outl(rtl_io_base + 0x20 + tx_cur*4, (uint32_t)pkt);
	outl(rtl_io_base + 0x10 + tx_cur*4, (uint32_t)length);

	tx_cur = (tx_cur + 1) % 4;
}
