start:
	push 'a'
	call print
	ret
print:
	mov %esp+4, %ebx
	mov $0x1, %eax
	int $0x80
	ret