.global switch_context
.extern runningTask
switch_context:
	push %ebp
	mov %esp, %ebp

	mov 8(%ebp), %eax	# eax = pointer ke task

	mov runningTask, %ebx	# ebx = pointer ke runningTask

	# field-by-field copy
	mov 16(%eax), %ecx
	mov %ecx, 20(%ebx)

	mov 20(%eax), %ecx
	mov %ecx, 16(%ebx)
	
	mov 24(%eax), %ecx
	mov %ecx, 28(%ebx)

	mov 28(%eax), %ecx
	mov %ecx, 24(%ebx)

	mov 32(%eax), %ecx
	mov %ecx, 4(%ebx)

	mov 36(%eax), %ecx
	mov %ecx, 12(%ebx)

	mov 40(%eax), %ecx
	mov %ecx, 8(%ebx)

	mov 44(%eax), %ecx
	mov %ecx, 0(%ebx)

	mov 56(%eax), %ecx
	mov %ecx, 32(%ebx)

	#mov 64(%eax), %ecx
	#mov %ecx, 36(%ebx)

	mov runningTask, %eax
	mov 44(%eax), %eax
	mov %eax, runningTask
	
	# field-by-field copy

	mov runningTask, %eax

	mov 4(%eax), %ebx
	mov 8(%eax), %ecx
	mov 12(%eax), %edx
	mov 16(%eax), %esi
	mov 20(%eax), %edi
	mov 24(%eax), %esp
	mov 28(%eax), %ebp

	push 36(%eax)
	push $0x08
	push 32(%eax)

	iret 

