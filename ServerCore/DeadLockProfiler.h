#pragma once

#include <stack>
#include <map>
#include <vector>

class DeadLockProfiler
{
public:
	void PushLock(const char* name);
	void PopLock(const char* name);
	void CheckCycle();

private:
	void DFS(int32 here);

private:
	unordered_map<const char*, int32> _nameToId;
	unordered_map<int32, const char*> _idToName;
	stack<int32> _lockStack;
	map<int32, set<int32>> _lockHistory;

	Mutex _lock;

private:
	//노드가 발견된 순서를 기록하는 배열
	vector<int32> _discoveredOrder;
	//노드가 발견된 순서
	int32 _discoveredCount = 0;
	//DFS(i) 가 종료 되었는지 여부
	vector<bool> _finished;
	//내가 발견된 이후 부모 노드들은 누군지 기록하는 배열
	vector<int32> _parent;
};
