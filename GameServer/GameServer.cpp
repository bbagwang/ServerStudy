#include "pch.h"
#include <iostream>
#include <thread>

thread_local int32 LThreadID = 0;

void ThreadMain(const int32 InLThreadID)
{
	LThreadID = InLThreadID;

	while (true)
	{
		std::cout << "Thread ID: " << LThreadID << std::endl;
		this_thread::sleep_for(1s);
	}
}

int main()
{
	vector<thread> Threads;
	for (int32 i = 1; i <= 10; ++i)
	{
		Threads.emplace_back(ThreadMain, i);
	}

	for (int32 i = 1; i <= 10; ++i)
	{
		Threads[i].join();
	}
}
