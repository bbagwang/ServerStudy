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
		++_popCount;
		Node* oldHead = _head.load();

		while (oldHead && _head.compare_exchange_weak(oldHead, oldHead->next) == false) {}

		if (oldHead == nullptr)
		{
			--_popCount;
			return false;
		}
		
		// Exception 이 발생하지 않는다고 가정.
		Value = oldHead->data;

		//잠시 삭제 보류.
		//이걸 지금 삭제해버리면, 동일한 타이밍에 TryPop을 호출한 스레드가 있을 수 있음.
		// 아래의 delete oldHead 를 실행하면, 동일한 타이밍에 TryPop을 호출한 스레드가 사용하는 oldHead 는
		// 이미 삭제된 메모리를 가리키게 됨.
		// 이 문제를 해결해야함.
		TryDelete(oldHead);

		return true;
	}
	// 1) 데이터 분리
	// 2) Count 체크
	// 3) 나 혼자면 삭제
	void TryDelete(Node* oldHead)
	{
		// 나 외에 누가 있는가?
		if (_popCount == 1)
		{
			// 나 혼자네?

			// 이왕 혼자인거, 삭제 예약된 다른 데이터들도 삭제해보자
			Node* node = _pendingList.exchange(nullptr);

			// 여전히 나 혼자인지, 중간에 끼어든 애가 있는지 체크
			// popCount 가 0 이면 지금 나밖에 없음.
			// popCount 가 1 이상이면 끼어든 애가 있음.
			if (--_popCount == 0)
			{
				// 끼어든 애가 없음 -> 삭제 진행
				// 이제와서 끼어들어도, 어차피 데이터는 위에서 exchange 를 통해 분리해둔 상태.
				// 삭제 해도 상관 없음.
				DeleteNodes(node);
			}
			else if (node)
			{
				// 누가 끼어들었으니 다시 갖다 놓자
				ChainPendingNodeList(node);
			}
			else
			{
				// 이미 PendingList 가 Null 이다.
				// 삭제할 데이터(노드)가 없다.
			}

			//내 데이터는 삭제
			delete oldHead;
		}
		else
		{
			// 누가 있네? 그럼 지금 삭제하지 않고, 삭제 예약만 하자.
			ChainPendingNode(oldHead);
			--_popCount;
		}
	}

	void ChainPendingNodeList(Node* first, Node* last)
	{
		last->next = _pendingList.load();

		while (_pendingList.compare_exchange_weak(last->next, first) == false) {}
	}

	void ChainPendingNodeList(Node* node)
	{
		Node* last = node;
		while (last->next)
			last = last->next;

		ChainPendingNodeList(node, last);
	}

	void ChainPendingNode(Node* node)
	{
		ChainPendingNodeList(node, node);
	}

	static void DeleteNodes(Node* node)
	{
		while (node)
		{
			Node* next = node->next;
			delete node;
			node = next;
		}
	}

private:
	atomic<Node*> _head;
	atomic<uint32> _popCount = 0; //Pop 을 실행중인 쓰레드 개수
	atomic<Node*> _pendingList; // 삭제 되어야 할 노드들 (첫번째 노드)[LinkedList 계열의 구현이기 때문]
};
