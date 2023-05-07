#include "pch.h"
#include <thread>
#include <mutex>
#include <atomic>

class SpinLock
{
public:
	void lock()
	{
		bool expected = false;
		bool desired = true;

		while (_locked.compare_exchange_strong(expected, desired) == false)
		{
			expected = false;
		}
	}

	void unlock()
	{
		_locked.store(false);
	}

private:
	atomic<bool> _locked = false;
};

int32 sum = 0;
mutex m;
SpinLock spinLock;

void Add()
{
	for (int32 i = 0; i < 10'0000; ++i)
	{
		lock_guard<SpinLock> guard(spinLock);
		++sum;
	}
}

void Sub()
{
	for (int32 i = 0; i < 10'0000; ++i)
	{
		lock_guard<SpinLock> guard(spinLock);
		--sum;
	}
}

int main()
{
	thread t1(Add);
	thread t2(Sub);
	
	t1.join();
	t2.join();
	
	cout << sum << endl;
}
