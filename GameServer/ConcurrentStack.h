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
		Node(const T& Value) : data(Value) {}

		T data;
		Node* next;
	};

public:
	void Push(T& Value)
	{
		Node* newNode = new Node(Value);
		newNode->next = _head.load();

		while (!_head.compare_exchange_weak(newNode->next, newNode)) {}
	}

	bool TryPop(T& Value)
	{
		Node* oldHead = _head.load();

		while (oldHead && _head.compare_exchange_weak(oldHead, oldHead->next) == false) {}

		if (oldHead == nullptr)
			return false;
		
		// Exception 이 발생하지 않는다고 가정.
		Value = oldHead->data;

		//잠시 삭제 보류.
		//이걸 지금 삭제해버리면, 동일한 타이밍에 TryPop을 호출한 스레드가 있을 수 있음.
		// 아래의 delete oldHead 를 실행하면, 동일한 타이밍에 TryPop을 호출한 스레드가 사용하는 oldHead 는
		// 이미 삭제된 메모리를 가리키게 됨.
		// 이 문제를 해결해야함.
		delete oldHead;

		return true;
	}

private:
	atomic<Node*> _head;
};
