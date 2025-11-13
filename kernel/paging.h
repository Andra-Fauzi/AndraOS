#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#pragma once

#include "util.h"
#include "memory.h"

// === BASIC PERMISSIONS ===
#define PTE_PRESENT    (1 << 0)   // Page ada di memory
#define PTE_READ       (0)        // Read-only (default)
#define PTE_WRITE      (1 << 1)   // Bisa write
#define PTE_USER       (1 << 2)   // User-accessible (kalau 0 = kernel only)
#define PTE_EXECUTE    (1 << 3)   // Bisa execute code

// === CACHE CONTROL ===  
#define PTE_CACHE_WB   (0)        // Write-back (normal, fastest)
#define PTE_CACHE_WT   (1 << 4)   // Write-through
#define PTE_CACHE_UC   (1 << 5)   // Uncached (untuk memory-mapped devices)
#define PTE_NO_CACHE   PTE_CACHE_UC

// === MEMORY MANAGEMENT ===
#define PTE_ACCESSED   (1 << 6)   // CPU set ini ketika page diakses
#define PTE_DIRTY      (1 << 7)   // CPU set ini ketika page di-write
#define PTE_GLOBAL     (1 << 8)   // Global page (tidak flush dari TLB)
#define PTE_SHARED     (1 << 9)   // Shared antara processes

#define PTE_KERNEL_CODE  (PTE_PRESENT | PTE_EXECUTE)                    // Kernel code (execute only)
#define PTE_KERNEL_DATA  (PTE_PRESENT | PTE_WRITE)                      // Kernel data (read/write)
#define PTE_KERNEL_RW    (PTE_PRESENT | PTE_WRITE | PTE_CACHE_WB)       // Kernel read/write cached

#define PTE_USER_CODE    (PTE_PRESENT | PTE_USER | PTE_EXECUTE)         // User code (execute only)
#define PTE_USER_DATA    (PTE_PRESENT | PTE_USER | PTE_WRITE)           // User data (read/write)  
#define PTE_USER_RO      (PTE_PRESENT | PTE_USER)                       // User read-only

#define PTE_FUNC_CODE    (PTE_PRESENT | PTE_EXECUTE | PTE_CACHE_WB)     // Execute-only code
#define PTE_FUNC_DATA    (PTE_PRESENT | PTE_WRITE | PTE_CACHE_WB)       // Function data (RW)
#define PTE_FUNC_STACK   (PTE_PRESENT | PTE_WRITE | PTE_CACHE_WB)       // Function stack   

void init_paging();
uint32_t map_page(uint32_t virtual_address, uint32_t physical_address, uint32_t flags);
uint32_t *get_page_table(uint32_t virtual_address);
uint32_t get_physical_address(uint32_t virtual_address);
void disable_paging();
void invalidate_tlb(uint32_t virtual_address);
void map_page_custom_page_dir(uint32_t *page_dir, uint32_t virtual_address, uint32_t physical_address, uint32_t flags);
