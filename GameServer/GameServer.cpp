#include "pch.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <Windows.h>

mutex m;
queue<int32> q;
HANDLE handle;

// CV 는 커널 오브젝트가 아닙니다. 유저 레벨 오브젝트입니다.
condition_variable cv;

void Producer()
{
	while (true)
	{
		{
			unique_lock<mutex> lock(m);
			q.push(100);
		}

		cv.notify_one(); // Wait 중인 스레드가 있으면 딱 1개를 깨운다.
	}
}

void Consumer()
{
	while (true)
	{
		unique_lock<mutex> lock(m);
		cv.wait(lock, []() {return !q.empty(); });
		//1) Lock을 잡고
		//2) 조건 확인
		//3) 조건이 만족되지 않으면 Wait
		//4) 조건이 만족되면 Wait를 빠져나온다.

		// 그런데 notify_one을 했으면 항상 조건식을 만족하는거 아닐까?
		// Spurious Wakeou (가짜 기상?)
		// notify_one 할 때, Lock 을 잡고 있는 것이 아니기 때문.
		// 중간에 다른 애가 Lock을 잡고, 조건을 바꿔버릴 수 있음.
		// 그래서 조건식을 다시 확인해야 함.

		int32 data = q.front();
		q.pop();
		cout << data << endl;
	}
}

int main()
{
	thread t1(Producer);
	thread t2(Consumer);

	t1.join();
	t2.join();
}
