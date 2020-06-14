#ifndef __STCALL_H
#define __STCALL_H

struct st_context;
typedef void(*st_pf)(struct st_context*, void*);

#ifdef __cplusplus
extern "C" {
#endif

extern struct st_context* st_context_new(int stack_size);

extern int st_context_init(struct st_context* pctx, st_pf pf, void* args);

extern int st_resume(struct st_context* ctx);

extern void st_yield(struct st_context* ctx);

extern int st_alive(struct st_context* ctx);

extern int st_context_free(struct st_context* ctx);

#ifdef __cplusplus
}
#endif
#endif