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

	# Build iret stack frame FIRST before touching esp
	# iret needs: eip, cs, eflags
	mov 24(%eax), %esp   # Switch to new task's stack
	mov 36(%eax), %ebx   # Load eflags
	push %ebx            # Push eflags for iret
	push $0x08           # Push code segment
	mov 32(%eax), %ebx   # Load eip  
	push %ebx            # Push eip for iret
	
	# Now restore all GPRs (except esp which is already set)
	# We need to be careful - use ebx temporarily, then restore it last
	mov 4(%eax), %ebx    # Load task's ebx value (temporarily in ebx - will fix)
	push %ebx            # Save ebx on stack temporarily
	
	mov 0(%eax), %ebx    # Load task's eax into ebx (temp)
	push %ebx            # Save eax on stack temporarily
	
	# Restore other registers
	mov 8(%eax), %ecx
	mov 12(%eax), %edx
	mov 16(%eax), %esi
	mov 20(%eax), %edi
	mov 28(%eax), %ebp
	
	# Finally restore eax and ebx from stack
	pop %eax             # Restore eax
	pop %ebx             # Restore ebx
	
	# Stack frame is: [eip] [cs] [eflags] <- esp
	iret 

