#include "acpi.h"

void testacpi() {
	for(uint32_t i = 0x000E0000; i < 0x000FFFFF; i += 0x1000) {
		map_page(i, i, PTE_KERNEL_RW | PTE_PRESENT);
	}
	bool found = false;
	for(uint32_t i = 0x000E0000; i < 0x000FFFFF; i += 0x1) {
		RSDP_t *rsdp = (RSDP_t *)&i;
		if((memcmp(rsdp->Signature, "RSD PTR ", 8)) == 0) {
			kprint("dapat nih RSDPnya\n");
			found = true;
			break;
		}
	}
	if(found == false) {
		kprint("yah ga dapet\n");
	} else {
		kprint("yah dapet\n");
		//kprint_hex((uintptr_t)rsdp);
		kprint("\n");
	}
}
