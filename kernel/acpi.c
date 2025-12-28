#include "acpi.h"
#include <uacpi/kernel_api.h>
#include "PCI.h"
#include "port_io.h"
#include "memory.h"
#include <stdint.h>

RSDP_t *found_rsdp = NULL;
uint32_t rsdp_addr = 0;

void find_RSDP() {
    uint16_t ebda_seg = *(uint16_t*)0x40E;
    uint32_t ebda = ebda_seg << 4;

    // Map EBDA (1 KB)
    for (uint32_t p = ebda & 0xFFFFF000;
         p < ((ebda + 1024) & 0xFFFFF000) + 0x1000;
         p += 0x1000)
        map_page(p, p, PTE_KERNEL_RW | PTE_PRESENT);

    for (uint32_t addr = ebda; addr < ebda + 1024; addr += 16) {
        RSDP_t *r = (RSDP_t*)addr;
        if (!memcmp(r->Signature, "RSD PTR ", 8)) {
            found_rsdp = r;
	    rsdp_addr = addr;
            return;
        }
    }

    // Map BIOS area (E0000-FFFFF = 128KB)
    for (uint32_t p = 0x000E0000; p < 0x00100000; p += 0x1000)
        map_page(p, p, PTE_KERNEL_RW | PTE_PRESENT);

    for (uint32_t addr = 0x000E0000; addr < 0x00100000; addr += 16) {
        RSDP_t *r = (RSDP_t*)addr;
        if (!memcmp(r->Signature, "RSD PTR ", 8)) {
            found_rsdp = r;
	    rsdp_addr = addr;
            return;
        }
    }
    
    // Memory akan di-map on-demand oleh uacpi_kernel_map()
    // saat uACPI library perlu akses ACPI tables
}

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
	find_RSDP();
	*out_rsdp_address = (uacpi_phys_addr)rsdp_addr;
	
	// Debug: verify RSDP is valid
	printf("RSDP found at: 0x%x\n", (uint32_t)rsdp_addr);
	if (found_rsdp) {
		printf("RSDP Signature: %.8s\n", found_rsdp->Signature);
		printf("RSDP RSDT Address: 0x%x\n", found_rsdp->RsdtAddress);
	}
	
	return UACPI_STATUS_OK;
}

extern uint32_t boot_page_directory;
#define MIN_BLOCK_SIZE 16
#define ALIGNMENT 4
#define PAGE_SIZE 4096
#define HEAP_REGION 2048 * 1024

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
	// Round down address to page boundary
	uacpi_phys_addr page_offset = addr & (PAGE_SIZE - 1);
	uacpi_phys_addr aligned_addr = addr & ~(PAGE_SIZE - 1);
	
	// Calculate total length including offset
	uacpi_size total_len = len + page_offset;
	
	// Round up length to page boundary
	uacpi_size len_aligned = (total_len + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	
	// Debug: track what we're mapping
	printf("uACPI map: 0x%x+0x%x -> ", (uint32_t)aligned_addr, (uint32_t)len_aligned);
	
	// Map all pages (identity mapping) dengan flag yang benar
	// PENTING: Ini yang benar-benar map memory untuk ACPI tables!
	for(uacpi_size j = 0; j < len_aligned; j += PAGE_SIZE) {
		uint32_t phys = aligned_addr + j;
		// Map dengan PTE_KERNEL_RW untuk kernel access
		// Identity mapping: virtual addr = physical addr
		uint32_t result = map_page(phys, phys, PTE_KERNEL_RW | PTE_PRESENT);
		
		// Note: map_page returns virtual address (which is phys here)
		// If map_page fails it returns 0. But 0 is also valid address.
		// For now we assume failure if 0 returns for non-zero input
		if (phys != 0 && result == 0) {
			printf("FAIL at 0x%x\n", phys);
			return NULL; // Mapping failed!
		}
	}
	
	printf("OK\n");
	
	// Return virtual address with preserved offset
	// Karena identity mapping, virtual addr = physical addr
	return (void *)(aligned_addr + page_offset);
}

void uacpi_kernel_unmap(void *addr, uacpi_size len) {
	// Debug: track unmapping
	printf("uACPI unmap: 0x%x (len 0x%x)\n", (uint32_t)addr, (uint32_t)len);

	// Round down address to page boundary
	uacpi_phys_addr virt_addr = (uacpi_phys_addr)addr;
	uacpi_phys_addr page_offset = virt_addr & (PAGE_SIZE - 1);
	uacpi_phys_addr aligned_addr = virt_addr & ~(PAGE_SIZE - 1);
	
	// Calculate total length including offset
	uacpi_size total_len = len + page_offset;
	
	// Round up length to page boundary
	uacpi_size len_aligned = (total_len + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	
	// Unmap all pages
	for(uacpi_size j = 0; j < len_aligned; j += PAGE_SIZE) {
		unmap_page(aligned_addr + j);
	}
}

#ifndef UACPI_FORMATTED_LOGGING
void uacpi_kernel_log(uacpi_log_level level, const uacpi_char* msg) {
	(void)level;
	printf("%s", msg);
}
#else
void uacpi_kernel_log(uacpi_log_level level, const uacpi_char* fmt, ...) {
	(void)level;

	char buffer[512];  // ukuran aman, ACPI log tidak panjang

	va_list args;
    	va_start(args, fmt);
    	vsnprintf(buffer, sizeof(buffer), fmt, args);
    	va_end(args);

    	printf("%s", buffer);
}
void uacpi_kernel_vlog(uacpi_log_level level, const uacpi_char* fmt, uacpi_va_list args) {
	(void)level;

    	char buffer[512];
    	vsnprintf(buffer, sizeof(buffer), fmt, args);
    	printf("%s", buffer);
}
#endif


// ============================================================================
// Memory Management
// ============================================================================

void *uacpi_kernel_alloc(uacpi_size size) {
	return malloc(size);
}

void uacpi_kernel_free(void *mem) {
	free(mem);
}

// ============================================================================
// Timing Functions
// ============================================================================

// Simple tick counter (akan di-increment oleh timer interrupt)
static volatile uint64_t system_ticks = 0;

// Fungsi helper untuk increment ticks (harus dipanggil dari timer IRQ handler)
void uacpi_tick_increment(void) {
	system_ticks++;
}

uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
	// Asumsi: 1 tick = 1ms (PIT default)
	// Konversi ke nanoseconds: 1ms = 1,000,000 ns
	return system_ticks * 1000000ULL;
}

void uacpi_kernel_stall(uacpi_u8 usec) {
	// Busy-wait delay untuk microseconds
	// Gunakan PIT atau RDTSC untuk timing yang lebih akurat
	// Implementasi sederhana: loop based
	uint64_t target = system_ticks + ((usec + 999) / 1000);
	while(system_ticks < target) {
		__asm__ volatile("pause");
	}
}

void uacpi_kernel_sleep(uacpi_u64 msec) {
	// Sleep dengan blocking
	uint64_t target = system_ticks + msec;
	while(system_ticks < target) {
		__asm__ volatile("hlt"); // Hemat power
	}
}

// ============================================================================
// PCI Functions
// ============================================================================

typedef struct {
	uint8_t bus;
	uint8_t slot;
	uint8_t func;
} pci_device_t;

uacpi_status uacpi_kernel_pci_device_open(
	uacpi_pci_address address, uacpi_handle *out_handle
) {
	pci_device_t *dev = (pci_device_t *)malloc(sizeof(pci_device_t));
	if (!dev) {
		return UACPI_STATUS_OUT_OF_MEMORY;
	}
	
	dev->bus = address.segment * 256 + address.bus;
	dev->slot = address.device;
	dev->func = address.function;
	
	*out_handle = (uacpi_handle)dev;
	return UACPI_STATUS_OK;
}

void uacpi_kernel_pci_device_close(uacpi_handle handle) {
	free((void *)handle);
}

uacpi_status uacpi_kernel_pci_read8(
	uacpi_handle device, uacpi_size offset, uacpi_u8 *value
) {
	pci_device_t *dev = (pci_device_t *)device;
	uint16_t word = pciConfigReadWord(dev->bus, dev->slot, dev->func, offset & ~1);
	*value = (offset & 1) ? (word >> 8) : (word & 0xFF);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_read16(
	uacpi_handle device, uacpi_size offset, uacpi_u16 *value
) {
	pci_device_t *dev = (pci_device_t *)device;
	*value = pciConfigReadWord(dev->bus, dev->slot, dev->func, offset);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_read32(
	uacpi_handle device, uacpi_size offset, uacpi_u32 *value
) {
	pci_device_t *dev = (pci_device_t *)device;
	uint16_t low = pciConfigReadWord(dev->bus, dev->slot, dev->func, offset);
	uint16_t high = pciConfigReadWord(dev->bus, dev->slot, dev->func, offset + 2);
	*value = ((uint32_t)high << 16) | low;
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write8(
	uacpi_handle device, uacpi_size offset, uacpi_u8 value
) {
	pci_device_t *dev = (pci_device_t *)device;
	// Read word, modify byte, write word
	uint16_t word = pciConfigReadWord(dev->bus, dev->slot, dev->func, offset & ~1);
	if (offset & 1) {
		word = (word & 0x00FF) | ((uint16_t)value << 8);
	} else {
		word = (word & 0xFF00) | value;
	}
	pciConfigWriteWord(dev->bus, dev->slot, dev->func, offset & ~1, word);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write16(
	uacpi_handle device, uacpi_size offset, uacpi_u16 value
) {
	pci_device_t *dev = (pci_device_t *)device;
	pciConfigWriteWord(dev->bus, dev->slot, dev->func, offset, value);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write32(
	uacpi_handle device, uacpi_size offset, uacpi_u32 value
) {
	pci_device_t *dev = (pci_device_t *)device;
	pciConfigWriteWord(dev->bus, dev->slot, dev->func, offset, value & 0xFFFF);
	pciConfigWriteWord(dev->bus, dev->slot, dev->func, offset + 2, value >> 16);
	return UACPI_STATUS_OK;
}

// ============================================================================
// IO Port Functions
// ============================================================================

typedef struct {
	uacpi_io_addr base;
	uacpi_size len;
} io_mapping_t;

uacpi_status uacpi_kernel_io_map(
	uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle
) {
	io_mapping_t *mapping = (io_mapping_t *)malloc(sizeof(io_mapping_t));
	if (!mapping) {
		return UACPI_STATUS_OUT_OF_MEMORY;
	}
	
	mapping->base = base;
	mapping->len = len;
	
	*out_handle = (uacpi_handle)mapping;
	return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle) {
	free((void *)handle);
}

uacpi_status uacpi_kernel_io_read8(
	uacpi_handle handle, uacpi_size offset, uacpi_u8 *out_value
) {
	io_mapping_t *mapping = (io_mapping_t *)handle;
	if (offset >= mapping->len) {
		return UACPI_STATUS_INVALID_ARGUMENT;
	}
	*out_value = inb(mapping->base + offset);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read16(
	uacpi_handle handle, uacpi_size offset, uacpi_u16 *out_value
) {
	io_mapping_t *mapping = (io_mapping_t *)handle;
	if (offset + 1 >= mapping->len) {
		return UACPI_STATUS_INVALID_ARGUMENT;
	}
	*out_value = inw(mapping->base + offset);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read32(
	uacpi_handle handle, uacpi_size offset, uacpi_u32 *out_value
) {
	io_mapping_t *mapping = (io_mapping_t *)handle;
	if (offset + 3 >= mapping->len) {
		return UACPI_STATUS_INVALID_ARGUMENT;
	}
	*out_value = inl(mapping->base + offset);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write8(
	uacpi_handle handle, uacpi_size offset, uacpi_u8 in_value
) {
	io_mapping_t *mapping = (io_mapping_t *)handle;
	if (offset >= mapping->len) {
		return UACPI_STATUS_INVALID_ARGUMENT;
	}
	outb(mapping->base + offset, in_value);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write16(
	uacpi_handle handle, uacpi_size offset, uacpi_u16 in_value
) {
	io_mapping_t *mapping = (io_mapping_t *)handle;
	if (offset + 1 >= mapping->len) {
		return UACPI_STATUS_INVALID_ARGUMENT;
	}
	outw(mapping->base + offset, in_value);
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write32(
	uacpi_handle handle, uacpi_size offset, uacpi_u32 in_value
) {
	io_mapping_t *mapping = (io_mapping_t *)handle;
	if (offset + 3 >= mapping->len) {
		return UACPI_STATUS_INVALID_ARGUMENT;
	}
	outl(mapping->base + offset, in_value);
	return UACPI_STATUS_OK;
}

// ============================================================================
// Synchronization - Mutex
// ============================================================================

typedef struct {
	volatile uint32_t locked;
	uacpi_thread_id owner;
} uacpi_mutex_t;

uacpi_handle uacpi_kernel_create_mutex(void) {
	uacpi_mutex_t *mutex = (uacpi_mutex_t *)malloc(sizeof(uacpi_mutex_t));
	if (mutex) {
		mutex->locked = 0;
		mutex->owner = 0;
	}
	return (uacpi_handle)mutex;
}

void uacpi_kernel_free_mutex(uacpi_handle handle) {
	free((void *)handle);
}

uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle handle, uacpi_u16 timeout) {
	uacpi_mutex_t *mutex = (uacpi_mutex_t *)handle;
	
	// Simple spinlock implementation
	// TODO: Implement proper timeout handling
	(void)timeout;
	
	while (__sync_lock_test_and_set(&mutex->locked, 1)) {
		__asm__ volatile("pause");
	}
	
	mutex->owner = uacpi_kernel_get_thread_id();
	return UACPI_STATUS_OK;
}

void uacpi_kernel_release_mutex(uacpi_handle handle) {
	uacpi_mutex_t *mutex = (uacpi_mutex_t *)handle;
	mutex->owner = 0;
	__sync_lock_release(&mutex->locked);
}

// ============================================================================
// Synchronization - Event
// ============================================================================

typedef struct {
	volatile uint32_t counter;
} uacpi_event_t;

uacpi_handle uacpi_kernel_create_event(void) {
	uacpi_event_t *event = (uacpi_event_t *)malloc(sizeof(uacpi_event_t));
	if (event) {
		event->counter = 0;
	}
	return (uacpi_handle)event;
}

void uacpi_kernel_free_event(uacpi_handle handle) {
	free((void *)handle);
}

uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle handle, uacpi_u16 timeout) {
	uacpi_event_t *event = (uacpi_event_t *)handle;
	
	// Simple polling implementation
	// TODO: Implement proper timeout handling
	(void)timeout;
	
	while (event->counter == 0) {
		__asm__ volatile("pause");
	}
	
	__sync_fetch_and_sub(&event->counter, 1);
	return UACPI_TRUE;
}

void uacpi_kernel_signal_event(uacpi_handle handle) {
	uacpi_event_t *event = (uacpi_event_t *)handle;
	__sync_fetch_and_add(&event->counter, 1);
}

void uacpi_kernel_reset_event(uacpi_handle handle) {
	uacpi_event_t *event = (uacpi_event_t *)handle;
	event->counter = 0;
}

// ============================================================================
// Synchronization - Spinlock
// ============================================================================

typedef struct {
	volatile uint32_t locked;
} uacpi_spinlock_t;

uacpi_handle uacpi_kernel_create_spinlock(void) {
	uacpi_spinlock_t *lock = (uacpi_spinlock_t *)malloc(sizeof(uacpi_spinlock_t));
	if (lock) {
		lock->locked = 0;
	}
	return (uacpi_handle)lock;
}

void uacpi_kernel_free_spinlock(uacpi_handle handle) {
	free((void *)handle);
}

uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle handle) {
	uacpi_spinlock_t *lock = (uacpi_spinlock_t *)handle;
	
	// Save and disable interrupts
	uacpi_cpu_flags flags;
	__asm__ volatile(
		"pushf\n"
		"pop %0\n"
		"cli\n"
		: "=r"(flags)
	);
	
	// Acquire spinlock
	while (__sync_lock_test_and_set(&lock->locked, 1)) {
		__asm__ volatile("pause");
	}
	
	return flags;
}

void uacpi_kernel_unlock_spinlock(uacpi_handle handle, uacpi_cpu_flags flags) {
	uacpi_spinlock_t *lock = (uacpi_spinlock_t *)handle;
	
	// Release spinlock
	__sync_lock_release(&lock->locked);
	
	// Restore interrupt state
	if (flags & 0x200) { // IF flag
		__asm__ volatile("sti");
	}
}

// ============================================================================
// Threading
// ============================================================================

uacpi_thread_id uacpi_kernel_get_thread_id(void) {
	// Untuk single-threaded kernel, return constant ID
	// TODO: Implement proper threading support
	return (uacpi_thread_id)1;
}

// ============================================================================
// Interrupt Handling
// ============================================================================

typedef struct irq_handler_node {
	uacpi_u32 irq;
	uacpi_interrupt_handler handler;
	uacpi_handle ctx;
	struct irq_handler_node *next;
} irq_handler_node_t;

static irq_handler_node_t *irq_handlers = NULL;

uacpi_status uacpi_kernel_install_interrupt_handler(
	uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx,
	uacpi_handle *out_irq_handle
) {
	irq_handler_node_t *node = (irq_handler_node_t *)malloc(sizeof(irq_handler_node_t));
	if (!node) {
		return UACPI_STATUS_OUT_OF_MEMORY;
	}
	
	node->irq = irq;
	node->handler = handler;
	node->ctx = ctx;
	node->next = irq_handlers;
	irq_handlers = node;
	
	*out_irq_handle = (uacpi_handle)node;
	
	// TODO: Register with actual IRQ system
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(
	uacpi_interrupt_handler handler, uacpi_handle irq_handle
) {
	(void)handler;
	
	irq_handler_node_t *node = (irq_handler_node_t *)irq_handle;
	irq_handler_node_t **prev = &irq_handlers;
	
	while (*prev) {
		if (*prev == node) {
			*prev = node->next;
			free(node);
			return UACPI_STATUS_OK;
		}
		prev = &(*prev)->next;
	}
	
	return UACPI_STATUS_INVALID_ARGUMENT;
}

// ============================================================================
// Work Scheduling
// ============================================================================

typedef struct work_node {
	uacpi_work_type type;
	uacpi_work_handler handler;
	uacpi_handle ctx;
	struct work_node *next;
} work_node_t;

static work_node_t *work_queue = NULL;

uacpi_status uacpi_kernel_schedule_work(
	uacpi_work_type type, uacpi_work_handler handler, uacpi_handle ctx
) {
	work_node_t *node = (work_node_t *)malloc(sizeof(work_node_t));
	if (!node) {
		return UACPI_STATUS_OUT_OF_MEMORY;
	}
	
	node->type = type;
	node->handler = handler;
	node->ctx = ctx;
	node->next = work_queue;
	work_queue = node;
	
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_wait_for_work_completion(void) {
	// Execute all pending work
	while (work_queue) {
		work_node_t *node = work_queue;
		work_queue = node->next;
		
		node->handler(node->ctx);
		free(node);
	}
	
	return UACPI_STATUS_OK;
}

// ============================================================================
// Firmware Requests
// ============================================================================

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *req) {
	// Handle breakpoint or fatal requests
	(void)req;
	// TODO: Implement proper handling
	return UACPI_STATUS_OK;
}

// ============================================================================
// uACPI Standard Library Functions
// ============================================================================

// uACPI requires memcpy - simple byte-by-byte implementation
// Addresses should already be mapped via uacpi_kernel_map before calling this
void *uacpi_memcpy(void *dest, const void *src, uacpi_size count) {
	// NULL pointer checks - uACPI should NEVER pass NULL!
	if (!dest || !src) {
		printf("ERROR: uacpi_memcpy NULL pointer! dest=0x%x src=0x%x count=%u\n",
		       (uint32_t)dest, (uint32_t)src, (uint32_t)count);
		return dest;
	}
	
	if (count == 0) return dest;
	
	unsigned char *d = (unsigned char *)dest;
	const unsigned char *s = (const unsigned char *)src;
	
	// Debug: print memcpy call for tracking
	printf("uacpi_memcpy: 0x%x <- 0x%x (%u bytes)\n", 
	       (uint32_t)dest, (uint32_t)src, (uint32_t)count);
	
	for (uacpi_size i = 0; i < count; i++) {
		d[i] = s[i];  // If page fault here, address not mapped!
	}
	
	return dest;
}

void testacpi() {
	find_RSDP();
	if(found_rsdp != NULL) {
		kprint("\n");
		kprint(found_rsdp->Signature);
		kprint("\n");
		kprint("ketemu cuy\n");
	}
}

