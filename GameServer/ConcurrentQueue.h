#pragma once

#include <mutex>

template <typename T>
class LockQueue
{
public:
	LockQueue() {}
	LockQueue(const LockQueue&) = delete;
	LockQueue& operator=(const LockQueue&) = delete;

	void Push(T Value)
	{
		lock_guard<mutex> lock(_mutex);
		_queue.push(std::move(Value));
		_condVar.notify_one();
	}

	bool TryPop(T& Value)
	{
		lock_guard<mutex> lock(_mutex);
		if (_queue.empty())
		{
			return false;
		}

		Value = std::move(_queue.front());
		_queue.pop();
		return true;
	}

	void WaitPop(T& Value)
	{
		unique_lock<mutex> lock(_mutex);
		_condVar.wait(lock, [this] {return !_queue.empty(); });

		Value = std::move(_queue.front());
		_queue.pop();
	}

private:
	queue<T> _queue;
	mutex _mutex;
	condition_variable _condVar;
};

template <typename T>
class LockFreeQueue
{
	struct Node
	{
		shared_ptr<T> data;
		Node* next = nullptr;
	};

public:
	// 빈 데이터를 하나 두고 처리 하는 방식으로 만들자.
	LockFreeQueue() : _head(new Node), _tail(_head) {}

	//Copy 방지
	LockFreeQueue(const LockFreeQueue&) = delete;
	LockFreeQueue& operator=(const LockFreeQueue&) = delete;

	void Push(const T& Value)
	{
		shared_ptr<T> newData(make_shared<T>(Value));
		Node* dummy = new Node();

		Node* oldTail = _tail;
		oldTail->data->swap(newData);
		oldTail->next = dummy;
		_tail = dummy;

		// 1. Dummy Node 를 만든다.
		// [New Dummy]
		// [Data]->[Data]->[Dummy]
		// [Head]          [Tail]

		// 2. oldTail 에 현재 Tail 이 가르키는 Dummy Node 를 가져와 정보를 넣는다.
		// [Data]->[Data]->[Dummy]
		// [Head]          [Tail]
		
		// oldTail = [Dummy] (_tail)
		// [Dummy] 에 데이터 입력.
		// [Dummy] 의 다음을 [New Dummy] 로 설정.
		// [Dummy]->[New Dummy]

		// 3. Tail 을 [New Dummy] 로 설정.
		// [Data]->[Data]->[Dummy]->[New Dummy]
		// [Head]                   [Tail]
	}

	shared_ptr<T> TryPop()
	{
		Node* oldHead = PopHead();
		if (oldHead == nullptr)
			return shared_ptr<T>();

		shared_ptr<T> res(oldHead->data);
		delete oldHead;

		return res;
	}

private:
	Node* PopHead()
	{
		Node* oldHead = _head;
		if (oldHead == _tail)
			return nullptr;

		_head = oldHead->next;
		return oldHead;
	}

private:
	// 데이터 추가는 tail 에서.
	// 데이터 삭제는 head 에서.
	// tail 은 항상 Dummy Node 를 물고 있도록.
	// [Data]->[Data]->[Data]->[Dummy]
	// [Head]                  [Tail]

	Node* _head = nullptr;
	Node* _tail = nullptr;
};
