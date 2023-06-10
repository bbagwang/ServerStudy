#pragma once

#include "Types.h"

/* Reader-Writer Spin Lock */

/*
���� 16 Bit �� Writer Count
���� 16 Bit �� Reader Count
[WWWWWWWW][WWWWWWWW][RRRRRRRR][RRRRRRRR]
W : Write Flag (Exclusive Lock Owner ThreadId)
R : Read Flag (Shared Lock Count)

��å

���� �����尡
WriteLock ��� WriteLock (O : ����)
WriteLock ��� ReadLock (O : ����)
ReadLock ��� ReadLock (O : ����)
ReadLock ��� WriteLock (X : �Ұ���)
*/

class Lock
{
	enum : uint32 {
		ACQUIRE_TIMEOUT_TICK = 10000,
		MAX_SPIN_COUNT = 5000,
		WRITE_THRAED_MASK = 0xFFFF'0000,
		READ_COUNT_MASK = 0x0000'FFFF,
		EMPTY_FLAG = 0x0000'0000,
	};

public:

	void WriteLock();
	void WriteUnlock();

	void ReadLock();
	void ReadUnlock();

private:
	Atomic<uint32> _lockFlag = EMPTY_FLAG;
	uint16 _writeCount = 0;
};


#pragma region LockGuard

class ReadLockGuard
{
public:
ReadLockGuard(Lock& lock) : _lock(lock) { _lock.ReadLock(); }
	~ReadLockGuard() { _lock.ReadUnlock(); }

private:
	Lock& _lock;
};

class WriteLockGuard
{
public:
	WriteLockGuard(Lock& lock) : _lock(lock) { _lock.WriteLock(); }
	~WriteLockGuard() { _lock.WriteUnlock(); }

private:
	Lock& _lock;
};

#pragma endregion LockGuard
