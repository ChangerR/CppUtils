#pragma once
#include <functional>
#include <chrono>
#include <future>
#include <queue>
#include <thread>
#include <memory>

namespace schedule {

struct function_timer
{
	std::function<void()> func;
	std::chrono::system_clock::time_point time;

	function_timer()
	{ }

	function_timer(std::function<void()>&& f,const std::chrono::system_clock::time_point& t)
		: func(f),time(t)
	{ }

	bool operator<(const function_timer& rhs) const
	{
		return time > rhs.time;
	}

	void get() const
	{
		func();
	}
};

class Scheduler
{
public:
	Scheduler()
		:running_(false)	
	{ }

	~Scheduler()
	{
		if (running_)
		{
			stop();
		}
	}

	void start()
	{
		if (!running_)
		{
			running_ = true;
			thread_ = std::make_shared<std::thread>([this]() { run(); });
		}

	}

	void stop()
	{
		running_ = false;
		if (thread_)
		{
			cv_.notify_all();
			thread_->join();
			thread_ = nullptr;
		}
	}

	void run()
	{
		while (running_)
		{
			auto now = std::chrono::system_clock::now();
			lock_.lock();
			while (!tasks_.empty() && tasks_.top().time <= now) {
				const function_timer& f = tasks_.top();
				lock_.unlock();
				f.get();
				lock_.lock();
				tasks_.pop();
			}
			std::chrono::milliseconds sleep_interval = tasks_.empty() ? std::chrono::duration<int>(3600) : 
					std::chrono::duration_cast<std::chrono::milliseconds>( tasks_.top().time - std::chrono::system_clock::now());
			lock_.unlock();
			
			std::unique_lock<std::mutex> lck(lock_);
			cv_.wait_for(lck,sleep_interval);
			//std::this_thread::sleep_for(sleep_interval);	
		}
	}

	void ScheduleAfter(std::int32_t milliseconds, std::function<void()>&& func)
	{
		ScheduleAt(std::chrono::system_clock::now() + std::chrono::duration<int, std::milli>(milliseconds), std::move(func));
	}

	void ScheduleEvery(std::int32_t milliseconds, std::function<void()> func)
	{
		ScheduleEvery(std::chrono::milliseconds(milliseconds), std::move(func));
	}

private:
	void ScheduleAt(const std::chrono::system_clock::time_point& time, std::function<void()>&& func)
	{
		lock_.lock();
		tasks_.push(function_timer(std::move(func), time));
		lock_.unlock();
		
		cv_.notify_all();
	}

	void ScheduleEvery(const std::chrono::system_clock::duration interval, std::function<void()> func)
	{
		std::function<void()> waitFunc = [this, interval, func]()
		{
			func();
			this->ScheduleEvery(interval, func);
		};
		ScheduleAt(std::chrono::system_clock::now() + interval, std::move(waitFunc));
	}
	std::priority_queue<function_timer> tasks_;
	std::shared_ptr<std::thread> thread_;
	std::mutex lock_;
	std::condition_variable cv_;
	bool running_;

	Scheduler& operator=(const Scheduler& rhs) = delete;
	Scheduler(const Scheduler& rhs) = delete;
};

}
