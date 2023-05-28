﻿#pragma once

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

