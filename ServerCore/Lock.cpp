#include "pch.h"
#include "Lock.h"
#include "CoreTLS.h"

void Lock::WriteLock()
{
	// ������ �����尡 �����ϰ� �ִٸ� ������ ����.
	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THRAED_MASK) >> 16;
	if (LThreadId == lockThreadId)
	{
		//�ٸ� ������� ���� ���ϴ� �׳� �÷������� ��.
		++_writeCount;
		return;
	}

	const int64 beginTick = ::GetTickCount64();

	const uint32 desired = ((LThreadId << 16) & WRITE_THRAED_MASK);

	while (true)
	{
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; ++spinCount)
		{
			//�ƹ��� ���� �� �����ϰ� ���� ���� ��, �����ؼ� �������� ��´�.
			uint32 expected = EMPTY_FLAG;
			if (_lockFlag.compare_exchange_strong(OUT expected, desired))
			{
				++_writeCount;
				return;
			}
		}

		if (::GetTickCount64() - beginTick >= ACQUIRE_TIMEOUT_TICK)
			CRASH("LOCK_TIMEOUT");

		this_thread::yield();
	}
}

void Lock::WriteUnlock()
{
	//ReadLock �� Ǯ�� ������ WriteUnlock �Ұ���.
	if((_lockFlag.load() & READ_COUNT_MASK) != 0)
		CRASH("INVALID_UNLOCK_ORDER");

	const int32 lockCount = --_writeCount;
	if (lockCount == 0)
	{
		_lockFlag.store(EMPTY_FLAG);
	}
}

void Lock::ReadLock()
{
	//������ �����尡 ����(Write) �ϰ� �ִٸ� ������ ����.
	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THRAED_MASK) >> 16;
	if (LThreadId == lockThreadId)
	{
		//�ٸ� ������� ���� ���ϴ� �׳� �÷������� ��.
		_lockFlag.fetch_add(1);
		return;
	}

	const int64 beginTick = ::GetTickCount64();

	while (true)
	{
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; ++spinCount)
		{
			//�ƹ��� ����(Write) �ϰ� ���� ���� �� �����ؼ� ���� ī��Ʈ�� �ø���.
			uint32 expected = (_lockFlag.load() & READ_COUNT_MASK);
			if(_lockFlag.compare_exchange_strong(OUT expected, expected + 1))
				return;
		}

		if (::GetTickCount64() - beginTick >= ACQUIRE_TIMEOUT_TICK)
			CRASH("LOCK_TIMEOUT");

		this_thread::yield();
	}
}

void Lock::ReadUnlock()
{
	if ((_lockFlag.fetch_sub(1) & READ_COUNT_MASK) == 0)
		CRASH("MULTIPLE_UNLOCK");
}

