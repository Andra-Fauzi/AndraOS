.global getchar
getchar:
	mov %eax, 2
	int $0x80
	ret

