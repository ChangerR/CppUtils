#include "coroutine.h"
#include "assert.h"
#ifndef __linux__
#include "stcall.h"
class CoroutineImpl
{
public:
	CoroutineImpl(Coroutine* parent, int32_t stack_size = 128 * 1024);
	
	virtual ~CoroutineImpl();
	
	bool init(const std::function<void(Coroutine* co)>& func);
	
	bool enter();
	
	void yield();
	
	bool is_done();
	
	static void callback(st_context* ctx, void* arg);
	
private:
	Coroutine* parent_;
	st_context* ctx_;
	std::function<void (Coroutine* co)> func_;
	int32_t stack_size_;
};

CoroutineImpl::CoroutineImpl(Coroutine* parent, int32_t stack_size) 
	: parent_(parent), ctx_(NULL), func_(nullptr), stack_size_(stack_size)
{
	
}

CoroutineImpl::~CoroutineImpl()
{
	if (ctx_)
	{
		st_context_free(ctx_);
		ctx_ = NULL;
	}
}

bool CoroutineImpl::init(const std::function<void (Coroutine* co)>& func)
{
	func_ = func;
	if (ctx_ == NULL)
	{
		ctx_ = st_context_new(stack_size_);
	}
	
	if (st_alive(ctx_) == 1)
	{
		return false;
	}
	
	st_context_init(ctx_, CoroutineImpl::callback, (void*)this);
	
	return true;
}

bool CoroutineImpl::enter()
{
	if (ctx_)
	{
		st_resume(ctx_);
		return st_alive(ctx_) == 1;
	}
	return false;
}

void CoroutineImpl::yield()
{
	if (ctx_)
	{
		st_yield(ctx_);
	}
}

bool CoroutineImpl::is_done()
{
	if (ctx_)
	{
		return st_alive(ctx_) == 0;
	}
	return true;
}

void CoroutineImpl::callback(st_context* ctx, void* arg)
{
	CoroutineImpl* impl = (CoroutineImpl*)arg;
	(void)ctx;
	
	if(impl->func_)
	{
		impl->func_(impl->parent_);	
	}
}

#else
#include <ucontext.h>

class CoroutineImpl
{
public:
	CoroutineImpl(Coroutine* parent, int32_t stack_size = 128 * 1024);
	
	virtual ~CoroutineImpl();
	
	bool init(const std::function<void(Coroutine* co)>& func);
	
	bool enter();
	
	void yield();
	
	bool is_done();
	
	static void callback(void* arg);
	
private:
	Coroutine* parent_;
	std::function<void (Coroutine* co)> func_;
	ucontext_t uctx_main;
	ucontext_t uctx_child;
	char* stack_;
	int32_t stack_size_;
	int32_t s_start_;
	int32_t s_in_main_;
};	

CoroutineImpl::CoroutineImpl(Coroutine* parent, int32_t stack_size) : parent_(parent), func_(nullptr)
	, stack_(nullptr), stack_size_(stack_size), s_start_(0), s_in_main_(1)
{
	stack_ = new char[stack_size];
}
	
CoroutineImpl::~CoroutineImpl()
{
	delete[] stack_;
	stack_ = nullptr;
}
	
bool CoroutineImpl::init(const std::function<void(Coroutine* co)>& func)
{
	if (s_start_ != 0 || s_in_main_ != 1)
	{
		return false;
	}
	if (getcontext(&uctx_child) == -1)
	{
		return false;
	}
	func_ = func;
	uctx_child.uc_stack.ss_sp = stack_;
	uctx_child.uc_stack.ss_size = stack_size_;
	uctx_child.uc_link = &uctx_main;
	
	makecontext(&uctx_child, (void (*)(void))callback, 1, this);
	return true;
}

bool CoroutineImpl::enter()
{
	s_in_main_ = 0;
	swapcontext(&uctx_main, &uctx_child);
	s_in_main_ = 1;
	
	return s_start_ == 1;
}

void CoroutineImpl::yield()
{
	if (s_in_main_ == 0)
	{
		swapcontext(&uctx_child, &uctx_main);
	}
	else
	{
		assert(0 || "TRY IN MAIN CALL YIELD");
	}
}

bool CoroutineImpl::is_done()
{
	return s_start_ == 0;
}

void CoroutineImpl::callback(void* arg)
{
	CoroutineImpl* impl = (CoroutineImpl*)arg;
	impl->s_start_ = 1;
	if(impl->func_)
	{
		impl->func_(impl->parent_);	
	}
	impl->s_start_ = 0;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////

Coroutine::Coroutine(int32_t stack_size)
{
	impl = new CoroutineImpl(this, stack_size);
}

Coroutine::~Coroutine()
{
	if (impl != nullptr)
	{
		delete impl;
		impl = nullptr;
	}
}

bool Coroutine::init(const std::function<void (Coroutine* co)>& func)
{
	return impl->init(func);
}

bool Coroutine::enter()
{
	return impl->enter();
}

void Coroutine::yield()
{
	impl->yield();
}

bool Coroutine::is_done()
{
	return impl->is_done();
}
