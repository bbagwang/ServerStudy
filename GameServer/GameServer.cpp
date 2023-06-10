#include "pch.h"
#include <iostream>
#include <memory>
#include "ConcurrentQueue.h"
#include "ConcurrentStack.h"

LockFreeQueue<int32> q;
LockFreeStack<int32> s;

void Push()
{
    while (true)
    {
        int32 value = rand() % 100;
        q.Push(value);

        //!!! sleep 이 없이, 너무 빠르게 Push 하면 Pop이 느려서 데이터가 쌓이게 된다.
        //!!! 그래서 Memory Leak 가 발생할 수 있음. 다만, 이 경우는 일반적이지 않은 경우라고 가정해야함.
        this_thread::sleep_for(1ms);
    }
}

void Pop()
{
    while (true)
    {
        auto data = q.TryPop();
        if (data)
        {
			cout << *data << endl;
        }
    }
}

int main()
{
    thread t1(Push);
    thread t2(Push);
    thread t3(Pop);
    thread t4(Pop);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
}
