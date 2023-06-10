#include "pch.h"
#include <iostream>
#include <memory>
#include "ConcurrentQueue.h"
#include "ConcurrentStack.h"

LockQueue<int32> q;
LockFreeStack<int32> s;

void Push()
{
    while (true)
    {
        int32 value = rand() % 100;
        s.Push(value);

        //!!! sleep 이 없이, 너무 빠르게 Push 하면 Pop이 느려서 데이터가 쌓이게 된다.
        //!!! 그래서 Memory Leak 가 발생할 수 있음. 다만, 이 경우는 일반적이지 않은 경우라고 가정해야함.
        this_thread::sleep_for(1ms);
    }
}

void Pop()
{
    while (true)
    {
        auto data = s.TryPop();
        if (data)
        {
			cout << *data << endl;
        }
    }
}

int main()
{
    shared_ptr<int32> ptr;
    const bool bIsLockFree = std::atomic_is_lock_free(&ptr);

    // !!! 머신에 따라 다르겠지만, 대부분의 머신에서 shared_ptr 은 lock free 방식으로 동작하지 않는다.
    // !!! 가짜 Lock Free 인 상태이다!

    thread t1(Push);
    thread t2(Push);
    thread t3(Pop);
    thread t4(Pop);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
}
