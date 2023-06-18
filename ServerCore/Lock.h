#pragma once

#include "Types.h"

/* Reader-Writer Spin Lock */

/*
상위 16 Bit 는 Writer Count
하위 16 Bit 는 Reader Count
[WWWWWWWW][WWWWWWWW][RRRRRRRR][RRRRRRRR]
W : Write Flag (Exclusive Lock Owner ThreadId)
R : Read Flag (Shared Lock Count)

정책

동일 스레드가
WriteLock 잡고 WriteLock (O : 가능)
WriteLock 잡고 ReadLock (O : 가능)
ReadLock 잡고 ReadLock (O : 가능)
ReadLock 잡고 WriteLock (X : 불가능)
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

	void WriteLock(const char* name);
	void WriteUnlock(const char* name);

	void ReadLock(const char* name);
	void ReadUnlock(const char* name);

private:
	Atomic<uint32> _lockFlag = EMPTY_FLAG;
	uint16 _writeCount = 0;
};


#pragma region LockGuard

class ReadLockGuard
{
public:
ReadLockGuard(Lock& lock, const char* name)
		: _lock(lock), _name(name) { _lock.ReadLock(_name); }
	~ReadLockGuard() { _lock.ReadUnlock(_name); }

private:
	Lock& _lock;
	const char* _name;
};

class WriteLockGuard
{
public:
	WriteLockGuard(Lock& lock, const char* name)
		: _lock(lock), _name(name) { _lock.WriteLock(_name); }
	~WriteLockGuard() { _lock.WriteUnlock(_name); }

private:
	Lock& _lock;
	const char* _name;
};

#pragma endregion LockGuard
