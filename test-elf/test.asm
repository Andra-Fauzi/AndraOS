.intel_syntax noprefix
.global _start

_start:
    mov eax, 1        # system call number (1 = print char misalnya)
    mov ebx, 'A'      # argument
    int 0x80          # panggil syscall kernel-mu

hang:
    jmp hang

