.global switch_to_userland
switch_to_userland:
    mov 4(%esp), %ebx    # Entry point (EIP)
    mov 8(%esp), %ecx    # User Stack (ESP)

    # Set up data segments for user mode (RPL = 3)
    # GDT entry 4 is User Data (0x20), so 0x20 | 3 = 0x23
    mov $0x23, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    # Prepare stack for IRET
    # Stack layout for IRET (inter-privilege):
    # [SS] [ESP] [EFLAGS] [CS] [EIP]

    push $0x23      # SS (User Data Segment with RPL 3)
    push %ecx       # ESP (User Stack Pointer)
    
    pushf           # EFLAGS
    pop %eax
    or $0x200, %eax # Enable Interrupts (IF bit)
    push %eax       # Push updated EFLAGS

    push $0x1B      # CS (User Code Segment with RPL 3)
                    # GDT entry 3 is User Code (0x18), so 0x18 | 3 = 0x1B
    push %ebx       # EIP (Entry Point)

    iret            # Jump to user mode!