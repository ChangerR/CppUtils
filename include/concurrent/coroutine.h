#ifndef __MC_COROUTINE_H
#define __MC_COROUTINE_H
#include <functional>

class CoroutineImpl;

class Coroutine
{
public:
	Coroutine(int32_t stack_size = 128 * 1024);
	virtual ~Coroutine();
	
	bool init(const std::function<void (Coroutine* co)>& func);
	
	bool enter();
	
	void yield();
	
	bool is_done();
	
private:
	CoroutineImpl* impl;
};

#endif