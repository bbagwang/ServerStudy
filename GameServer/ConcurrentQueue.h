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
	struct Node;

	struct CountedNodePtr
	{
		int32 externalCount; //참조권
		Node* ptr = nullptr;
	};

	struct NodeCounter
	{
		uint32 internalCount : 30; //참조권 반환 관련
		uint32 externalCountRemaining : 2; //Push & Pop 다중 참조권 관련
	};

	struct Node
	{
		Node()
		{
			NodeCounter newCount;
			newCount.internalCount = 0;
			newCount.externalCountRemaining = 2;
			count.store(newCount);

			next.ptr = nullptr;
			next.externalCount = 0;
		}

		void ReleaseRef()
		{
			NodeCounter oldCounter = count.load();

			while (true)
			{
				NodeCounter newCounter = oldCounter;
				--newCounter.internalCount;

				//끼어들 수 있음
				if (count.compare_exchange_strong(oldCounter, newCounter))
				{
					//건드리는 애가 아무도 없을 때
					if (newCounter.internalCount == 0 && newCounter.externalCountRemaining == 0)
						delete this;
					
					break;
				}
			}
		}

		atomic<T*> data;
		atomic<NodeCounter> count;
		CountedNodePtr next;
	};

public:
	// 빈 데이터를 하나 두고 처리 하는 방식으로 만들자.
	LockFreeQueue()
	{
		CountedNodePtr node;
		node.ptr = new Node;
		node.externalCount = 1;

		//Head 와 Tail 의 Count 는 따로 따로 관리됨
		_head.store(node);
		_tail.store(node);
	}

	//Copy 방지
	LockFreeQueue(const LockFreeQueue&) = delete;
	LockFreeQueue& operator=(const LockFreeQueue&) = delete;

	void Push(const T& Value)
	{
		unique_ptr<T> newData = make_unique<T>(Value);

		CountedNodePtr dummy;
		dummy.ptr = new Node;
		dummy.externalCount = 1;

		CountedNodePtr oldTail = _tail.load();
		// oldTail 의 data 는 nullptr.
		// 기존의 Dummy 를 가르키고 있기 때문. (위에 있는 dummy 아님)

		while (true)
		{
			//참조권 획득 (externalCount 를 현시점 기준 +1 한 애가 이김)
			IncreaseExternalCount(_tail, oldTail);

			//소유권 획득 (data 를 먼저 교환한 애가 이김)
			T* oldData = nullptr;
			if (oldTail.ptr->data.compare_exchange_strong(oldData, newData.get()))
			{
				//Push 상태에서 oldTail 조작중에 Pop 이 일어나거나 해서 oldTail 이 바뀌었을 수 있음.
				//다른 스레드에서 oldTail의 데이터를 삭제하거나 변경된 경우 문제가 됨.
				//그래서 Push 중간에 Pop 하거나 Pop 중에 Push 를 하는 경우 처리를 위해 NodeCounter 의 externalCountRemaining 이 사용됨.
				//externalCountRemaining 이 0 이 된 경우, Push 와 Pop 의 경쟁이 모두 끝났다는 의미.
				//공유하는 Node 데이터에 대한 레퍼런스 카운팅을 따로 한다고 보면 됨.

				oldTail.ptr->next = dummy;
				oldTail = _tail.exchange(dummy);
				
				FreeExternalCount(oldTail);

				newData.release(); //데이터에 대한 unique_ptr 의 소유권 포기.

				break;
			}

			// 여기서 부터 소유권 경쟁 패배한 경우...
			oldTail.ptr->ReleaseRef();
		}
	}

	shared_ptr<T> TryPop()
	{
		CountedNodePtr oldHead = _head.load();

		while (true)
		{
			//참조권 획득 (externalCount 를 현시점 기준 +1 한 애가 애김)
			IncreaseExternalCount(_head, oldHead);

			Node* ptr = oldHead.ptr;
			if (ptr == _tail.load().ptr)
			{
				ptr->ReleaseRef();
				return shared_ptr<T>();
			}

			//소유권 획득 (head = ptr->next)
			if (_head.compare_exchange_strong(oldHead, ptr->next))
			{
				//exchange 로 하면 버그 생김. load 로 하면 괜찮음.
				//T* res = ptr->data.exchange(nullptr);
				T* res = ptr->data.load();
				FreeExternalCount(oldHead);
				return shared_ptr<T>(res);
			}

			//소유권 획득 실패한 애가 뒷정리를 한다.
			ptr->ReleaseRef();
		}
	}

private:
	static void IncreaseExternalCount(atomic<CountedNodePtr>& counter, CountedNodePtr& oldCounter)
	{
		while (true)
		{
			CountedNodePtr newCounter = oldCounter;
			++newCounter.externalCount;

			if (counter.compare_exchange_strong(oldCounter, newCounter))
			{
				oldCounter.externalCount = newCounter.externalCount;
				break;
			}
		}
	}

	static void FreeExternalCount(CountedNodePtr& oldNodePtr)
	{
		Node* ptr = oldNodePtr.ptr;
		const int32 countIncrease = oldNodePtr.externalCount - 2;

		NodeCounter oldCounter = ptr->count.load();

		while (true)
		{
			NodeCounter newCounter = oldCounter;
			--newCounter.externalCountRemaining;
			newCounter.internalCount += countIncrease;

			if (ptr->count.compare_exchange_strong(oldCounter, newCounter))
			{
				//건드리는 애가 아무도 없을 때
				if (newCounter.internalCount == 0 && newCounter.externalCountRemaining == 0)
					delete ptr;

				break;
			}
		}
	}

private:
	// 데이터 추가는 tail 에서.
	// 데이터 삭제는 head 에서.
	// tail 은 항상 Dummy Node 를 물고 있도록.
	// [Data]->[Data]->[Data]->[Dummy]
	// [Head]                  [Tail]

	atomic<CountedNodePtr> _head;
	atomic<CountedNodePtr> _tail;
};
