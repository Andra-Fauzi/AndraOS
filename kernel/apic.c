#include "apic.h"

#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_BSP 0x100
#define IA32_APIC_BASE_MSR_ENABLE 0x800
#define CPUID_FEAT_EDX_APIC (1 << 9)

extern multiboot_info_t *multiboot_info;

bool check_apic() {
	uint32_t eax, ebx, ecx, edx;
	asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx): "a"(1));
	return (edx & CPUID_FEAT_EDX_APIC) != 0;
}

/*
void cpuSetMSR(uint32_t msr, uint32_t low, uint32_t high) {
	uint32_t eax, ebx, ecx, edx;
	asm volatile("wrmsr" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(low), "b"(0), "c"(msr), "d"(high));
}
*/

void cpuSetMSR(uint32_t msr, uint32_t low, uint32_t high) {
    asm volatile (
        "wrmsr"
        :
        : "c"(msr), "a"(low), "d"(high)
    );
}


/*
void cpuGetMSR(uint32_t msr, uint32_t *low, uint32_t *high) {
	uint32_t eax, ebx, ecx, edx;
	asm volatile("rdmsr" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0), "b"(0), "c"(msr), "d"(0));
	*low = eax;
	*high = edx;
}
*/

void cpuGetMSR(uint32_t msr, uint32_t *low, uint32_t *high) {
    uint32_t eax, edx;
    asm volatile (
        "rdmsr"
        : "=a"(eax), "=d"(edx)
        : "c"(msr)
    );
    *low = eax;
    *high = edx;
}


void cpu_set_apic_base(uintptr_t apic) {
	uint32_t edx = 0;
	uint32_t eax = (apic & 0xFFFFF000) | IA32_APIC_BASE_MSR_ENABLE;
	#ifdef __PHYSICAL_MEMORY_EXTENSION__
   	edx = (apic >> 32) & 0x0f;
	#endif
	cpuSetMSR(IA32_APIC_BASE_MSR, eax, edx);
}

uintptr_t cpu_get_apic_base() {
	uint32_t eax, edx;
	cpuGetMSR(IA32_APIC_BASE_MSR, &eax, &edx);
	#ifdef __PHYSICAL_MEMORY_EXTENSION__
   	return (eax & 0xfffff000) | ((edx & 0x0f) << 32);
	#else
   	return (eax & 0xfffff000);
	#endif
}

static uintptr_t apic_base;

static inline uint32_t ReadRegister(uint32_t reg) {
    return *(volatile uint32_t *)(apic_base  + reg);
}

static inline void write_reg(uint32_t reg, uint32_t val) {
    *(volatile uint32_t *)(apic_base + reg) = val;
}

#define LAPIC_EOI 0xB0

void lapic_eoi() {
    write_reg(LAPIC_EOI, 0);
}

#define IOAPIC_BASE 0XFEC00000

#define IOREGSEL 0x00
#define IOWIN 0x10

static inline void ioapic_write(uint8_t reg, uint32_t val) {
	*(volatile uint32_t *)(IOAPIC_BASE + IOREGSEL) = reg;
	*(volatile uint32_t *)(IOAPIC_BASE + IOWIN) = val;
}

static inline uint32_t ioapic_read(uint8_t reg) {
	*(volatile uint32_t *)(IOAPIC_BASE + IOREGSEL) = reg;
	return *(volatile uint32_t *)(IOAPIC_BASE + IOWIN);
}

void init_apic() {
	asm volatile("cli");
	if(check_apic()) {
		kprint("apic support\n");
	} else {
		kprint("apic is not supported\n");
		return;
	}
	uintptr_t apic_phys = cpu_get_apic_base();
	#define PAGE_PRESENT 0x001
	#define PAGE_RW      0x002
	#define PAGE_PWT     0x008
	#define PAGE_PCD     0x010
	map_page(
			apic_phys, 
			apic_phys, 
			PAGE_PRESENT | PAGE_RW | PAGE_PWT | PAGE_PCD
			);
	map_page(IOAPIC_BASE, IOAPIC_BASE,
			PAGE_PRESENT | PAGE_RW | PAGE_PWT | PAGE_PCD
		);
			
	apic_base = apic_phys;
	PIC_disable();
	cpu_set_apic_base(apic_base);
	write_reg(0xF0, ReadRegister(0xF0) | 0x100);
	asm volatile("sti");
}

// IO APIC

void ioapic_enable_keyboard() {
	uint8_t irq = 1;
	uint8_t vector = 0x21;

	uint32_t low = 
		vector |
		(0 << 8) | // delivery = fixed
		(0 << 11) | // physical mode
		(0 << 13) | // active_high
		(0 << 15); // unmasked
			   //
			   //
	uint32_t high = 0 << 24; // target cpu 0
	
	// 0x10 + 2*irq   (low)
	// 0x10 + 2*irq+1 (high)


	ioapic_write(0x10 + irq * 2, low);
	ioapic_write(0x10 + irq * 2 + 1, high);
}
