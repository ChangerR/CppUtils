#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "stcall.h"
#ifdef __APPLE__
#define __unix__ 1
#endif
#define kEIP 0
#define kESP 7

#define kRDI 7
#define kRSI 8
#define kRIP 9
#define kRSP 13
#define kRCX 11

struct st_param_t
{
	const void *s1;
};

struct st_regs
{
#ifdef _MSC_VER  
#if defined(_WIN32) && !defined(_WIN64)
	void *regs[8];
#else
	void *regs[14];
#endif
#elif __GNUC__
#if defined(__i386__)
	void *regs[8];
#else
	void *regs[14];
#endif
#endif	

#ifdef SAVE_FPU
	char fpu[528];
#endif
	size_t ss_size;
	char *ss_sp;
};

struct st_context
{
	struct st_regs reg;
	struct st_regs reg_origin;
	size_t ss_size;
	void* ss_stack;
	st_pf pf;
	void* pf_args;
	short s_start;
	short s_inroutine;
};

static void stcall_routine_func(struct st_context* ctx);

#ifdef __unix__
extern int st_swap(struct st_regs* current, struct st_regs* target) asm("st_swap");

extern int st_save_fpu(void* point) asm("st_save_fpu");

extern int st_restore_fpu(void* point) asm("st_restore_fpu");
#else
extern int st_swap(struct st_regs* current, struct st_regs* target);

extern int st_save_fpu(void* point);

extern int st_restore_fpu(void* point);
#endif


static void st_swap_wrap(struct st_regs* current, struct st_regs* target)
{
#ifdef SAVE_FPU
	st_save_fpu(current->fpu);
	st_restore_fpu(target->fpu);
#endif
	st_swap(current, target);
}

struct st_context* st_context_new(int stack_size)
{
	struct st_context* pctx;

	pctx = (struct st_context*)malloc(sizeof(struct st_context));
	memset(pctx, 0, sizeof(struct st_context));

	if (stack_size <= 0)
	{
		stack_size = 128 * 1024;
	}
	else if (stack_size > 1024 * 1024 * 8)
	{
		stack_size = 1024 * 1024 * 8;
	}

	if (stack_size & 0xFFF)
	{
		stack_size &= ~0xFFF;
		stack_size += 0x1000;
	}

	pctx->ss_size = stack_size;
	pctx->ss_stack = malloc(pctx->ss_size);

	pctx->reg.ss_size = stack_size;
	pctx->reg.ss_sp = (char*)pctx->ss_stack;

	return pctx;
}

int st_context_init(struct st_context* pctx, st_pf pf, void* args)
{
	pctx->pf_args = args;
	pctx->pf = pf;
	pctx->s_start = 0;
	pctx->s_inroutine = 0;
	
	return 0;
}

int st_resume(struct st_context* ctx)
{
	char* sp;
	struct st_param_t* param;
	
	if (ctx->s_start == 0)
	{
#if (defined(_MSC_VER) && defined(_WIN32) && !defined(_WIN64)) || (defined(__i386__) && defined(__unix__))
		sp = ctx->reg.ss_sp + ctx->reg.ss_size - sizeof(struct st_param_t);
		sp = (char*)((unsigned long)sp & -16l);
		
		param = (struct st_param_t*)sp ;
		param->s1 = ctx;

		memset(ctx->reg.regs, 0, sizeof(ctx->reg.regs));
#ifdef SAVE_FPU
		st_save_fpu(ctx->reg.fpu);
		memset(ctx->reg_origin.fpu, 0, sizeof(ctx->reg_origin.fpu));
#endif

		ctx->reg.regs[ kESP ] = (char*)(sp) - sizeof(void*);
		ctx->reg.regs[ kEIP ] = (char*)stcall_routine_func;
#elif defined(_WIN64) || defined(__MINGW64__)
		(void)param;
		sp = ctx->reg.ss_sp + ctx->reg.ss_size - sizeof(struct st_param_t);
		sp = (char*)((unsigned long long)sp & -16ll);
		
		memset(ctx->reg.regs, 0, sizeof(ctx->reg.regs));
#ifdef SAVE_FPU
		st_save_fpu(ctx->reg.fpu);
		memset(ctx->reg_origin.fpu, 0, sizeof(ctx->reg_origin.fpu));
#endif
		ctx->reg.regs[ kRSP ] = (char*)(sp) - sizeof(void*);
		ctx->reg.regs[ kRIP ] = (char*)stcall_routine_func;
		ctx->reg.regs[ kRCX ] = (char*)ctx;
#elif defined(__unix__) && defined(__x86_64__)
		(void)param;
		sp = ctx->reg.ss_sp + ctx->reg.ss_size - sizeof(struct st_param_t);
		sp = (char*)((unsigned long)sp & -16ll);
		
		memset(ctx->reg.regs, 0, sizeof(ctx->reg.regs));
#ifdef SAVE_FPU
		st_save_fpu(ctx->reg.fpu);
		memset(ctx->reg_origin.fpu, 0, sizeof(ctx->reg_origin.fpu));
#endif
		ctx->reg.regs[ kRSP ] = (char*)(sp) - sizeof(void*);
		ctx->reg.regs[ kRIP ] = (char*)stcall_routine_func;
		ctx->reg.regs[ kRDI ] = (char*)ctx;
#else
#error "unknow platform,please check!"
#endif
		ctx->s_start = 1;
	}
	
	ctx->s_inroutine = 1;
	st_swap_wrap(&ctx->reg_origin, &ctx->reg);
	ctx->s_inroutine = 0;
	
	return 0;
}

void st_yield(struct st_context* ctx)
{
	if (ctx->s_inroutine == 1)
	{
		st_swap_wrap(&ctx->reg, &ctx->reg_origin);
	}
}

extern int st_alive(struct st_context* ctx)
{
	return ctx->s_start;
}

int st_context_free(struct st_context* ctx)
{
	if (ctx->ss_stack)
	{
		free(ctx->ss_stack);
	}
	free(ctx);
	return 0;
}

static void stcall_routine_func(struct st_context* ctx)
{
	if( ctx->pf != NULL )
	{
		(ctx->pf)(ctx, ctx->pf_args);
	}
	ctx->s_start = 0;
	st_yield(ctx);
}

#if defined(_MSC_VER) && defined(_WIN32) && !defined(_WIN64)
__declspec( naked ) int st_swap(struct st_regs* current, struct st_regs* target)
{
	__asm
	{
		lea    eax,[esp + 4]
		mov    esp,DWORD PTR [esp + 4]
		lea    esp,[esp + 32]
		push   eax
		push   ebp
		push   esi
		push   edi
		push   edx
		push   ecx
		push   ebx
		push   DWORD PTR [eax - 4]
		mov    esp,DWORD PTR [eax + 4]
		pop    eax
		pop    ebx
		pop    ecx
		pop    edx
		pop    edi
		pop    esi
		pop    ebp
		pop    esp
		push   eax
		xor    eax,eax
		ret
	}
}

__declspec( naked ) int st_save_fpu(void* point)
{
	__asm
	{
		mov eax,[esp + 4]
		add eax,15
		and eax,0xfffffff0
		fxsave [eax]
		xor eax,eax
		ret
	}
}

__declspec( naked ) int st_restore_fpu(void* point)
{
	__asm
	{
		mov eax,[esp + 4]
		add eax,15
		and eax,0xfffffff0
		fxrstor [eax]
		xor eax,eax
		ret
	}
}
#elif defined(__i386__)
__asm(
"st_swap:\n"
"	lea    0x4(%esp),%eax\n"
"	mov    0x4(%esp),%esp\n"
"	lea    0x20(%esp),%esp\n"
"	push   %eax\n"
"	push   %ebp\n"
"	push   %esi\n"
"	push   %edi\n"
"	push   %edx\n"
"	push   %ecx\n"
"	push   %ebx\n"
"	pushl  -0x4(%eax)\n"
"	mov    0x4(%eax),%esp\n"
"	pop    %eax\n"
"	pop    %ebx\n"
"	pop    %ecx\n"
"	pop    %edx\n"
"	pop    %edi\n"
"	pop    %esi\n"
"	pop    %ebp\n"
"	pop    %esp\n"
"	push   %eax\n"
"	xor    %eax,%eax\n"
"	ret\n"
);
__asm(
"st_save_fpu:\n"
"	mov    0x4(%esp),%eax\n"
"	add    $0xf,%eax\n"
"	and    $0xfffffff0,%eax\n"
"	fxsave  (%eax)\n"
"	xor    %eax,%eax\n"
"	ret\n"
);
__asm(
"st_restore_fpu:\n"
"	mov    0x4(%esp),%eax\n"
"	add    $0xf,%eax\n"
"	and    $0xfffffff0,%eax\n"
"	fxrstor (%eax)\n"
"	xor    %eax,%eax\n"
"	ret\n"
);
#elif defined(__MINGW64__)
__asm(
"st_swap:\n"
"	lea    0x8(%rsp),%rax\n"
"	lea    0x70(%rcx),%rsp\n"
"	push   %rax\n"
"	push   %rbx\n"
"	push   %rcx\n"
"	push   %rdx\n"
"	pushq  -0x8(%rax)\n"
"	push   %rsi\n"
"	push   %rdi\n"
"	push   %rbp\n"
"	push   %r8\n"
"	push   %r9\n"
"	push   %r12\n"
"	push   %r13\n"
"	push   %r14\n"
"	push   %r15\n"
"	mov    %rdx,%rsp\n"
"	pop    %r15\n"
"	pop    %r14\n"
"	pop    %r13\n"
"	pop    %r12\n"
"	pop    %r9\n"
"	pop    %r8\n"
"	pop    %rbp\n"
"	pop    %rdi\n"
"	pop    %rsi\n"
"	pop    %rax\n"
"	pop    %rdx\n"
"	pop    %rcx\n"
"	pop    %rbx\n"
"	pop    %rsp\n"
"	push   %rax\n"
"	xor    %eax,%eax\n"
"	retq\n"
);
__asm(
"st_save_fpu:\n"
"	add    $0xf,%rcx\n"
"	and    $0xfffffffffffffff0,%rcx\n"
"	fxsave  (%rcx)\n"
"	xor    %eax,%eax\n"
"	retq\n"
"\n"
"st_restore_fpu:\n"
"	add    $0xf,%rcx\n"
"	and    $0xfffffffffffffff0,%rcx\n"
"	fxrstor (%rcx)\n"
"	xor    %eax,%eax\n"
"	retq\n"
);
#elif defined(__unix__) && defined(__x86_64__)
__asm(
"st_swap:\n"
"	lea    0x8(%rsp),%rax\n"
"	lea    0x70(%rdi),%rsp\n"
"	push   %rax\n"
"	push   %rbx\n"
"	push   %rcx\n"
"	push   %rdx\n"
"	pushq  -0x8(%rax)\n"
"	push   %rsi\n"
"	push   %rdi\n"
"	push   %rbp\n"
"	push   %r8\n"
"	push   %r9\n"
"	push   %r12\n"
"	push   %r13\n"
"	push   %r14\n"
"	push   %r15\n"
"	mov    %rsi,%rsp\n"
"	pop    %r15\n"
"	pop    %r14\n"
"	pop    %r13\n"
"	pop    %r12\n"
"	pop    %r9\n"
"	pop    %r8\n"
"	pop    %rbp\n"
"	pop    %rdi\n"
"	pop    %rsi\n"
"	pop    %rax\n"
"	pop    %rdx\n"
"	pop    %rcx\n"
"	pop    %rbx\n"
"	pop    %rsp\n"
"	push   %rax\n"
"	xor    %eax,%eax\n"
"	retq\n"
);
__asm(
"st_save_fpu:\n"
"	add    $0xf,%rdi\n"
"	and    $0xfffffffffffffff0,%rdi\n"
"	fxsave  (%rdi)\n"
"	xor    %eax,%eax\n"
"	retq\n"
"\n"
"st_restore_fpu:\n"
"	add    $0xf,%rdi\n"
"	and    $0xfffffffffffffff0,%rdi\n"
"	fxrstor (%rdi)\n"
"	xor    %eax,%eax\n"
"	retq\n"
);
#endif
