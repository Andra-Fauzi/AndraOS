.code32
.section .text
.globl irq0
.type irq0, @function

irq0:
	cli
	pushl $0
	pushl $32
	jmp irq_common_stub
	
.globl irq1
irq1:
	cli
	pushl $0
	pushl $33
	jmp irq_common_stub

.globl irq2
irq2:
	cli
	pushl $0
	pushl $34
	jmp irq_common_stub

.globl irq3
irq3:
	cli
	pushl $0
	pushl $35
	jmp irq_common_stub

.globl irq4
irq4:
	cli
	pushl $0
	pushl $36
	jmp irq_common_stub

.globl irq5
irq5:
	cli
	pushl $0
	pushl $37
	jmp irq_common_stub

.globl irq6
irq6:
	cli
	pushl $0
	pushl $38
	jmp irq_common_stub

.globl irq7
irq7:
	cli
	pushl $0
	pushl $39
	jmp irq_common_stub

.globl irq8
irq8:
	cli
	pushl $0
	pushl $40
	jmp irq_common_stub

.globl irq9
irq9:
	cli
	pushl $0
	pushl $41
	jmp irq_common_stub

.globl irq10
irq10:
	cli
	pushl $0
	pushl $42
	jmp irq_common_stub

.globl irq11
irq11:
	cli
	pushl $0
	pushl $43
	jmp irq_common_stub

.globl irq12
irq12:
	cli
	pushl $0
	pushl $44
	jmp irq_common_stub

.globl irq13
irq13:
	cli
	pushl $0
	pushl $45
	jmp irq_common_stub

.globl irq14
irq14:
	cli
	pushl $0
	pushl $46
	jmp irq_common_stub

.globl irq15
irq15:
	cli
	pushl $0
	pushl $47
	jmp irq_common_stub

.globl irq_common_stub
.extern irq_handler

irq_common_stub:
	pusha
	push %ds
	push %es
	push %fs
	push %gs

	mov $0x10, %eax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	movl %esp, %eax
	pushl %eax
	call irq_handler
	addl $4, %esp

	popl %gs
	popl %fs
	popl %es
	popl %ds
	popa

	add $8, %esp
	iret
