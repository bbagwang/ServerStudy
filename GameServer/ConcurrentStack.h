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

// Lock Free 는 좋은 것일까?
// Lock Free 는 락을 사용하지 않는 것이지, 스레드 경쟁 상황을 피하는 것은 아니다.
// 아래 구현에서 TryPop 의 경우는 참조권과 소유권에 대한 스레드 경쟁 상황이 일어난다.
// 참조권을 얻었지만, 소유권을 얻지 못하면 다시 뺑뺑이를 돌아야한다.
// 다수의 스레드가 접근한다면, 경쟁은 더욱 심화될 것이다. 계속 while 을 돌며 체크해야 하므로, CPU 시간도 잡아먹는다.
// 실질적인 Lock 만 안쓴 것이지, Spin Lock 과 비슷하고, 코드만 더 복잡한 것 같다.
// 빠른지도 잘 모르겠다. 그냥 Lock 을 잘 쓰는게 맞는 것 같다...

template <typename T>
class LockFreeStack
{
	struct Node;

	struct CountedNodePtr
	{
		int32 externalCount = 0;
		Node* ptr = nullptr;
	};

	struct Node
	{
		Node(const T& Value) : data(make_shared<T>(Value)) {}

		shared_ptr<T> data;
		atomic<int32> internalCount = 0;
		CountedNodePtr next;
	};

public:
	void Push(T& Value)
	{
		CountedNodePtr node;
		node.ptr = new Node(Value);
		node.externalCount = 1;

		// ! 여기서부터 위험. 위 코드는 스택에 만들어둔 것이기 때문에 아직까지 괜찮.
		// 하지만 아래 코드부터는 head 조작이 들어가므로 스레드 경쟁 상황을 신경써야함.
		
		node.ptr->next = _head;
		while(_head.compare_exchange_weak(node.ptr->next,node) == false){}
	}

	shared_ptr<T> TryPop()
	{
		CountedNodePtr oldHead = _head;
		while (true)
		{
			//1 라운드 : 참조권 획득 (externalCount 를 현 시점 기준 +1 한 애가 이김)
			IncreaseHeadCount(oldHead);
			// 통과한 경우, 최소한 externalCount 는 2 이상 일테니 삭제X (안전하게 접근할 수 있는 상태.)
			Node* ptr = oldHead.ptr;

			//데이터 체크
			if(ptr == nullptr)
			{
				return shared_ptr<T>();
			}

			// 2 라운드 : 소유권 획득 (ptr->next 로 head 를 바꿔치기 한 애가 이김)
			if (_head.compare_exchange_strong(oldHead, ptr->next))
			{
				// 소유권 획득 (_head 와 oldHead 가 같음)
				// CountedNodePtr 이 가르키는 ptr 과 externalCount 둘다 동일한 경우임.
				// 여길 통과한다면 다음 노드를 head 로 인정하게됨.
				// 지금부터 oldHead 는 이 스레드의 것임.

				shared_ptr<T> res;
				res.swap(ptr->data);

				// 나 말고 또 누가 있나?
				// externalCount 는 1 로 시작.
				// internalCount 는 0 으로 시작.
				// 2 를 빼는 이유는 externalCount 에서 기본 1 에서 1 을 늘린 상태를 빼는 것.
				
				// 만약 2 개의 스레드가 함께 참조권을 물었다고 하면, externalCount 는 3 이 될 것.
				// 이 경우 countIncrease 는 1.
				// internalCount 의 기존 값이 0 이라면, fetch_add 는 0 을 반환하고, 1을 넣을 것.
				
				// 만약 지금 스레드 혼자 참조권을 얻었다면, externalCount 는 2 가 될 것.
				// 이 경우 countIncrease 는 0.
				// internalCount 의 기존 값이 0 이라면, fetch_add 는 0 을 반환하고, 0을 넣을 것.
				// 근데 두 값이 동일한 경우는, 이 스레드만 소유권과 참조권을 획득했다는 것을 의미함.
				// 그러므로 이 스레드가 뽑아온 노드의 삭제를 담당해도 괜찮음.
				const int32 countIncrease = oldHead.externalCount - 2;
				if (ptr->internalCount.fetch_add(countIncrease) == -countIncrease)
				{
					delete ptr;
				}
				
				return res;
			}
			else if (ptr->internalCount.fetch_sub(1) == 1)
			{
				// 참조권은 얻었으나, 소유권은 실패 -> 뒷수습은 내가 한다.
				// internalCount 를 1 감소시킨 결과가 0 이라면, fetch_sub 의 반환값은 그 전의 값인 1임.
				// 그러므로 내가 마지막 참조자임.

				delete ptr;
			}
		}
	}

private:
	void IncreaseHeadCount(CountedNodePtr& oldCounter)
	{
		while (true)
		{
			CountedNodePtr newCounter = oldCounter;
			// externalCount 는 일종의 번호표.
			// 늘어나기만 할 것.
			// 사용이 끝나더라도 카운트를 줄이지는 않을 것.
			// 은행에서 번호표를 뽑는 것과 비슷한 개념.
			// 10번 손님의 업무가 끝났다고 해서 새로운 번호표에 9번 손님을 부르면 번호가 겹치고 꼬일 것.
			// 11번 손님을 부르는 것과 같은 느낌.
			++newCounter.externalCount;

			if(_head.compare_exchange_strong(oldCounter, newCounter))
			{
				//Head 에 newCounter 값으로 넣어놨으므로, oldCounter 값으로 참조권 획득
				oldCounter.externalCount = newCounter.externalCount;
				break;
			}
		}
	}

private:
	atomic<CountedNodePtr> _head;
};
