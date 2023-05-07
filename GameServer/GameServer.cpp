#include "pch.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>

int64 Calculate()
{
	int64 sum = 0;
	for (int32 i = 0; i < 100'000; ++i)
	{
		sum += i;
	}

	return sum;
}

void PromiseWorker(std::promise<string>&& promise)
{
	promise.set_value("Hello, Babe");
}

void TaskWorker(std::packaged_task<int64()>&& task)
{
	task();
}

int main()
{
	// 동기 (Synchronous) 실행
	int64 sum = Calculate();

	//std::future
	// 비동기 (Asynchronous) 실행

	// 1) deferred -> Lazy Evaluation 지연해서 실행
	// 2) Async -> 별도의 스레드를 만들어서 실행
	// 3) Async | Deferred -> 둘중에 하나를 선택해서 실행

	// 언젠가 미래에 결과물을 뱉어줄거야!
	std::future<int64> future = std::async(std::launch::async, Calculate);
	
	// TODO

	// future.get() -> 결과물이 나올때까지 대기. 이미 나왔으면 바로 ㄱㄱ.
	// future 는 get 이후에 무효화됨. 그러므로 다시 get 을 호출하면 안됨.
	sum = future.get();

	std::cout << "sum: " << sum << std::endl;



	//객체의 멤버 함수도 호출 가능
	class Knight
	{
	public:
		int64 GetHp() { return 100; }
	};

	Knight knight;
	std::future<int64> future2 = std::async(std::launch::async, &Knight::GetHp, &knight);

	int64 hp = future2.get();
	std::cout<< "hp: " << hp << std::endl;

	//std::promise
	{
		//매래(std::future)에 결과물을 반환해줄꺼라 약속(std::promise) 해줘~
		//계약서 같은 느낌? 약속을 지키면 결과물을 줄꺼야!
		std::promise<string> promise;
		std::future<string> future = promise.get_future();

		thread t(PromiseWorker, std::move(promise));
		//여기서 promise 는 move 되었으므로 소유권이 넘겨졌고 이제부터 사용 불가.

		string message = future.get();
		// future 는 get 이후에 무효화됨. 그러므로 다시 get 을 호출하면 안됨.

		//여기서 message 를 써도 되는 이유는 이미 future 에서 다 기다렸기(Wait) 때문.
		cout << message << endl;

		t.join();
	}

	//std::packaged_task
	{
		std::packaged_task<int64()> task(Calculate);
		std::future<int64> future = task.get_future();
		thread t(TaskWorker, std::move(task));
		int64 sum = future.get();
		cout << "sum: " << sum << endl;
		t.join();
	}
}
