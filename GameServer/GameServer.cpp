#include "pch.h"
#include "ThreadManager.h"

void ThreadMain()
{
	while (true)
	{
		cout << "Hello I'm Thread #" << LThreadId << endl;
		this_thread::sleep_for(1s);
	}
}

int main()
{
	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch(ThreadMain);
	}

	GThreadManager->Join();
}
