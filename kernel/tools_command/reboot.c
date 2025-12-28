#include "reboot.h"

#include <uacpi/sleep.h>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>

void c_reboot(char *buffer, int length) {
    kprint("Attempting ACPI Reboot...\n");
    uacpi_status status = uacpi_reboot();
    if (status != UACPI_STATUS_OK) {
        kprint("Failed to reboot. Error: ");
        kprint_hex(status);
        kprint("\n");

        // Fallback to keyboard controller reset
        kprint("Attempting PS/2 Controller Reset...\n");
        outb(0x64, 0xFE);
        
        // Wait a bit
        uacpi_kernel_sleep(50);
        
        // Final fallback: Triple Fault
        kprint("Attempting Triple Fault...\n");
        asm volatile ("lidt 0");
        asm volatile ("int $3");
    }
    
    // Debugging: Print FADT Reset Register info
    // struct acpi_fadt *fadt;
    // if (uacpi_table_fadt(&fadt) == UACPI_STATUS_OK) {
    //     kprint("\n[Debug] FADT Reset Reg:\n");
    //     kprint("  Flags: "); kprint_hex(fadt->flags);
    //     if (fadt->flags & (1<<10)) kprint(" (SUP)"); else kprint(" (UNSUP)");
    //     kprint("\n  Addr: "); kprint_hex(fadt->reset_reg.address);
    //     kprint("\n  Base: "); kprint_hex(fadt->reset_reg.address_space_id);
    //     kprint("\n  Val:  "); kprint_hex(fadt->reset_value);
    //     kprint("\n");
        
    //     // Manual write attempt if supported (just for demonstration)
    //     // if ((fadt->Flags & (1<<10)) && fadt->ResetReg.Address) {
    //     //     outb(fadt->ResetReg.Address, fadt->ResetValue);
    //     // }
    // }
}