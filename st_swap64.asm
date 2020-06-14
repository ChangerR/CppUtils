
.CODE

st_swap:
	lea    rax , [rsp + 8]
	lea    rsp , [rcx + 112]
	push   rax
	push   rbx
	push   rcx
	push   rdx
	push   QWORD PTR [rax - 8]
	push   rsi
	push   rdi
	push   rbp
	push   r8
	push   r9
	push   r12
	push   r13
	push   r14
	push   r15
	mov    rsp,rdx
	pop    r15
	pop    r14
	pop    r13
	pop    r12
	pop    r9
	pop    r8
	pop    rbp
	pop    rdi
	pop    rsi
	pop    rax
	pop    rdx
	pop    rcx
	pop    rbx
	pop    rsp
	push   rax
	xor    eax,eax
	ret

st_save_fpu:
	add    rcx,15
	and    rcx,0fffffffffffffff0h
	fxsave  [rcx]
	xor    eax,eax
	ret

st_restore_fpu:
	add    rcx,15
	and    rcx,0fffffffffffffff0h
	fxrstor [rcx]
	xor    eax,eax
	ret

public st_swap, st_save_fpu, st_restore_fpu

END

