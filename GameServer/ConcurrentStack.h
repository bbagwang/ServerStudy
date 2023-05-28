#pragma once

#include <mutex>
#include <atomic>

template <typename T>
class LockStack
{
public:
	LockStack() {}
	LockStack(const LockStack&) = delete;
	LockStack& operator=(const LockStack&) = delete;

	void Push(T Value)
	{
		lock_guard<mutex> lock(_mutex);
		_stack.push(std::move(Value));
		_condVar.notify_one();
	}

	bool TryPop(T& Value)
	{
		lock_guard<mutex> lock(_mutex);
		if (_stack.empty())
		{
			return false;
		}

		Value = std::move(_stack.top());
		_stack.pop();
		return true;
	}

	void WaitPop(T& Value)
	{
		unique_lock<mutex> lock(_mutex);
		_condVar.wait(lock, [this] {return !_stack.empty(); });
		
		Value = std::move(_stack.top());
		_stack.pop();
	}

private:
	stack<T> _stack;
	mutex _mutex;
	condition_variable _condVar;
};

template <typename T>
class LockFreeStack
{
	struct Node
	{
		Node(const T& Value) : data(make_shared<T>(Value)) {}

		shared_ptr<T> data;
		shared_ptr<Node> next;
	};

public:
	void Push(T& Value)
	{
		shared_ptr<Node> node = make_shared<Node>(Value);
		node->next = std::atomic_load(&_head);

		while (std::atomic_compare_exchange_weak(&_head, &node->next, node) == false) {}
	}

	shared_ptr<T> TryPop()
	{
		// Head 를 꺼내는 행위가 Atomic 하지 않다.
		// Shared Pointer 자체가 Atomic 하지 않기 때문이다.
		// 그냥 가져오면 Shared Pointer 를 사용하는 현재 구현에서, head 의 레퍼런스 카운팅이 제대로 이뤄지지 않는다.
		// copy 가 이뤄지는 도중에 레퍼런스 카운트가 아직 오르지 않은 상황에서, 다른 스레드에서 운영하던 포인터의 레퍼런스 카운팅이 0 이 된다면, 데이터가 삭제될 가능성이 있다.
		// 따라서, head 를 꺼내는 행위는 반드시 atomic 하게 처리되어야 한다.
		shared_ptr<Node> oldHead = std::atomic_load(&_head);

		while (oldHead && std::atomic_compare_exchange_weak(&_head, &oldHead, oldHead->next) == false) {}

		if (oldHead == nullptr)
			return shared_ptr<T>();

		return oldHead->data;
	}

private:
	shared_ptr<Node> _head;
};
