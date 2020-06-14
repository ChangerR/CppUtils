#include "stcall.h"
#include "stdio.h"
#include "stdlib.h"
#include "coroutine.h"
#include <string>

class Test
{
public:
	Test(const std::string& name) : name_(name) {}
	virtual ~Test() {}

	void test(int a, Coroutine* co)
	{
		printf("%d\n", a);
		int index = 0;
		float i = (float)(rand() % 1000);

		while (index < 10)
		{
			printf("%s Test function, i = %f\n", name_.c_str(), i++);
			co->yield();
			index++;
		}
		printf("%s Test function end\n", name_.c_str());
	}
private:
	std::string name_;
};

int main(int args, char** argv)
{
	Coroutine co1;
	Test test1("TEST1");

	if (!co1.init((std::function<void (Coroutine*)>)std::bind(&Test::test, &test1, 1000, std::placeholders::_1)))
	{
		printf("Init Coroutine Failed\n");
		return 1;
	}

	do 
	{
		co1.enter();
		printf("Main function\n");
	} while (!co1.is_done());

	return 0;
}

//void test(st_context * ctx, void* arg)
//{
//	int index = 0;
//	float i = (float)(rand() % 1000);
//	const char* p = (const char*)arg;
//	
//	while (index < 10)
//	{
//		printf("%s Test function, i = %f\n", p, i++);
//		st_yield(ctx);
//		index++;
//	}
//	printf("%s Test function end\n", p);
//}
//
//int main(int args, char** argv)
//{
//	struct st_context* ctx;
//	struct st_context* ctx2;
//	float j = 100.0f;
//	char first[] = "FIRST";
//	char secord[] = "SECORD";
//	ctx = st_context_new(0);
//	ctx2 = st_context_new(0);
//	
//	st_context_init(ctx, test, first);
//	st_context_init(ctx2, test, secord);
//	
//	do 
//	{
//		st_resume(ctx);
//		st_resume(ctx2);
//		printf("Main function end, j = %f\n", j++);
//	} while (st_alive(ctx));
//	
//	st_context_free(ctx);
//	st_context_free(ctx2);
//	
//	return 0;
//}