#include "isolation.h"

#define FUNCTION_MEMORY_BASE 0x00800000
#define FUNCTION_MEMORY_SIZE 0x00010000

/* page size for mapping loops */
#define PAGE_SIZE 0x1000

/* func_page_dir will point to a freshly allocated page-directory for the
 * isolated function space. We keep it NULL until created. current_page_dir
 * stores the previous CR3 value so we can restore it when leaving.
 */
uint32_t *func_page_dir = NULL;
uint32_t *current_page_dir = NULL;

void enter_function_space() {
    /* create page directory if not yet created */
    if (func_page_dir == NULL) {
        uint32_t pd_page = alloc_page();
        if (pd_page == 0) {
            for (;;) asm("hlt");
        }
        func_page_dir = (uint32_t *)pd_page;
        /* alloc_page zeroes the page, but be explicit */
        memset(func_page_dir, 0, PAGE_SIZE);

        /* copy kernel's present PDEs so kernel remains accessible in the
         * function page directory (prevents invalid pointer accesses after
         * switching CR3). We expect `page_directory` to be defined in
         * paging.c; declare it extern here.
         */
        extern uint32_t page_directory[];
        for (int i = 0; i < 1024; ++i) {
            if (page_directory[i] & 0x1) {
                func_page_dir[i] = page_directory[i];
            }
        }

        /* map code region: FUNCTION_MEMORY_BASE .. + FUNCTION_MEMORY_SIZE */
        for (uint32_t addr = FUNCTION_MEMORY_BASE; addr < FUNCTION_MEMORY_BASE + FUNCTION_MEMORY_SIZE; addr += PAGE_SIZE) {
            map_page_custom_page_dir(func_page_dir, addr, addr, PTE_FUNC_CODE);
        }

        /* map data region (next chunk) */
        for (uint32_t addr = FUNCTION_MEMORY_BASE + FUNCTION_MEMORY_SIZE; addr < FUNCTION_MEMORY_BASE + 2 * FUNCTION_MEMORY_SIZE; addr += PAGE_SIZE) {
            map_page_custom_page_dir(func_page_dir, addr, addr, PTE_FUNC_DATA);
        }

        /* map stack region (following chunk) */
        for (uint32_t addr = FUNCTION_MEMORY_BASE + 2 * FUNCTION_MEMORY_SIZE; addr < FUNCTION_MEMORY_BASE + 3 * FUNCTION_MEMORY_SIZE; addr += PAGE_SIZE) {
            map_page_custom_page_dir(func_page_dir, addr, addr, PTE_FUNC_STACK);
        }
    }

    /* switch to function page directory: save current CR3 and load new PD
     * disable interrupts around the switch to avoid getting an interrupt
     * vectors while some mappings may still be in flux.
     */
    asm volatile("cli");
    asm volatile("mov %%cr3, %0" : "=r"(current_page_dir));
    asm volatile("mov %0, %%cr3" :: "r"(func_page_dir));
    asm volatile("sti");
}

void leave_function_space() {
    if (current_page_dir != NULL) {
        asm volatile("cli");
        asm volatile("mov %0, %%cr3" :: "r"(current_page_dir));
        asm volatile("sti");
        current_page_dir = NULL;
    }
}

/* helper: run a function in the isolated function-space with a fresh
 * stack placed at the top of the function stack region.
 */
void run_in_function_space(void (*func)(void)) {
    // enter and prepare PD if needed
    
    uintptr_t function_addr = (uintptr_t)func;
    
    enter_function_space();

    // save old stack
    uint32_t old_esp;
    asm volatile("mov %%esp, %0" : "=r"(old_esp));

    // set stack to top of function stack region
    uint32_t func_stack_top = FUNCTION_MEMORY_BASE + 3 * FUNCTION_MEMORY_SIZE;
    asm volatile("mov %0, %%esp" :: "r"(func_stack_top));

    // call the function
    func();

    // restore stack and leave
    asm volatile("mov %0, %%esp" :: "r"(old_esp));
    leave_function_space();
}