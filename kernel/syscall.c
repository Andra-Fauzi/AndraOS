#include "syscall.h"
#include "shell.h"

extern multiboot_info_t *multiboot_info;

void syscall_handler(struct regs *r) {
	// kprint("syscall called", multiboot_info);
    uint32_t syscall_number = r->eax;

    switch (syscall_number) {
        // we work on that later
        // case 0: // exit
        //     kprint("\n[Kernel] Program exited.\n", multiboot_info);
        //     // Since we don't have a scheduler, just hang here or infinite loop
        //     for(;;) asm("hlt");
        //     break;
        case 1: // print char (ebx = char)
            {
                char c = (char)r->ebx;
                print_char(c);
            }
            break;
	 case 2:
	    {
		    asm volatile("sti");
		    char character = shell_getchar();
		    while(character == -1) {
		    	    character = shell_getchar();
		    }
		    uint32_t *ebx = &r->ebx;
		    *ebx = character;

    		    asm volatile("cli");
	    }
	    break;
	case 3: 
	    {
		    clear_screen(multiboot_info);
	    }
	    break;
	case 4: 
	    {
		    uint32_t x = r->ebx;
		    uint32_t y = r->ecx;
		    uint32_t color = 0xFFFFFFFF;
		    draw_pixel(x, y, color);
	    }
	    break;
        default:
            kprint("Unknown syscall\n");
            break;
    }
    //kprint("\nsyscall called\n", multiboot_info);
}

void init_syscalls() {
    // Registrasi handler mungkin dilakukan di isr.c atau di sini jika kita punya akses ke IDT
    // Tapi karena architecture yang ada menggunakan isr_install di isr.c, kita akan modifikasi isr.c
}
