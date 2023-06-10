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
		
		// Exception �� �߻����� �ʴ´ٰ� ����.
		Value = oldHead->data;

		//��� ���� ����.
		//�̰� ���� �����ع�����, ������ Ÿ�ֿ̹� TryPop�� ȣ���� �����尡 ���� �� ����.
		// �Ʒ��� delete oldHead �� �����ϸ�, ������ Ÿ�ֿ̹� TryPop�� ȣ���� �����尡 ����ϴ� oldHead ��
		// �̹� ������ �޸𸮸� ����Ű�� ��.
		// �� ������ �ذ��ؾ���.
		delete oldHead;

		return true;
	}

private:
	atomic<Node*> _head;
};
