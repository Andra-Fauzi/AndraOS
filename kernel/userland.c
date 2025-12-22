#include "userland.h"
#include "terminal.h"
#include "idt.h"
#include "gdt.h"
#include "paging.h"

extern multiboot_info_t *multiboot_info;
extern void switch_to_userland(uint32_t entry, uint32_t stack);

// Setup syscall interrupt
void init_userland() {
    // idt_set_gate(0x80, (uint32_t)syscall_stub, 0x08, 0xEE);
    kprint("Syscall handler installed at INT 0x80\n");
}

// Main userland switch function
void switchToUserland() {
    kprint("=== USERLAND SETUP START ===\n");
    
    // Define user space virtual addresses (must be < 0xC0000000)
    #define USER_CODE_VIRT  0x00400000  // 4MB mark - standard for user programs
    #define USER_STACK_VIRT 0x00800000  // 8MB mark - user stack
    
    // 1. Allocate physical pages for user code
    uint32_t phys_code_page = alloc_page();
    if (phys_code_page == 0) {
        kprint("ERROR: Code page allocation failed\n");
        return;
    }
    uint32_t phys_code = phys_code_page - KERNEL_BASE;  // Convert to physical
    
    // 2. Allocate physical pages for user stack (2 pages = 8KB)
    uint32_t phys_stack_page1 = alloc_page();
    uint32_t phys_stack_page2 = alloc_page();
    if (phys_stack_page1 == 0 || phys_stack_page2 == 0) {
        kprint("ERROR: Stack allocation failed\n");
        return;
    }
    uint32_t phys_stack1 = phys_stack_page1 - KERNEL_BASE;
    uint32_t phys_stack2 = phys_stack_page2 - KERNEL_BASE;
    
    // 3. Allocate kernel stack for TSS (for interrupts from user mode)
    uint32_t kernel_stack = (uint32_t)alloc_page() + 4096;
    set_kernel_stack(kernel_stack);
    
    // 4. Map physical pages to USER SPACE virtual addresses
    map_page(USER_CODE_VIRT, phys_code, PTE_PRESENT | PTE_USER | PTE_WRITE);
    map_page(USER_STACK_VIRT, phys_stack1, PTE_PRESENT | PTE_USER | PTE_WRITE);
    map_page(USER_STACK_VIRT + 4096, phys_stack2, PTE_PRESENT | PTE_USER | PTE_WRITE);
    
    // 5. Write user code to the physical page (access via kernel mapping)
    unsigned char *code = (unsigned char *)phys_code_page;  // Access via kernel virtual address
    
    // Machine code: mov eax, 1; mov ebx, 'a'; int 0x80; jmp $ (infinite loop)
    code[0] = 0xB8;  code[1] = 0x01; code[2] = 0x00; code[3] = 0x00; code[4] = 0x00; // mov eax, 1
    code[5] = 0xBB;  code[6] = 0x61; code[7] = 0x00; code[8] = 0x00; code[9] = 0x00; // mov ebx, 'a'
    code[10] = 0xCD; code[11] = 0x80;                                               // int 0x80
    code[12] = 0xEB; code[13] = 0xF2;                                               // jmp $ (infinite loop)
    
    kprint("User code virtual addr: ");
    kprint_hex(USER_CODE_VIRT);
    kprint("\nUser code physical addr: ");
    kprint_hex(phys_code);
    kprint("\nUser stack virtual addr: ");
    kprint_hex(USER_STACK_VIRT + 8192 - 4);
    kprint("\n=== SWITCHING TO USERLAND ===\n");
    
    // 6. Switch to userland using USER SPACE addresses
    switch_to_userland(USER_CODE_VIRT, USER_STACK_VIRT + 8192 - 4);
    
    // Never returns
    kprint("ERROR: Returned from userland!\n");
}
