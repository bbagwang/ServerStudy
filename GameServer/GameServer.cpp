#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <thread>
#include <atomic>
#include <mutex>
#include "AccountManager.h"
#include "UserManager.h"

void Func1()
{
	for (int32 i = 0; i < 1000; ++i)
	{
		UserManager::GetInstance()->ProcessSave();
	}
}

void Func2()
{
	for (int32 i = 0; i < 1000; ++i)
	{
		AccountManager::GetInstance()->ProcessLogin();
	}
}
int main()
{
    thread t1(Func1);
	thread t2(Func2);

	t1.join();
	t2.join();

	std::cout << "Jobs Done!\n";
}
