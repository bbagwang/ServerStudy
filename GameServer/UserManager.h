#pragma once
#include <mutex>

class User
{
	// TODO
};

class UserManager
{
public:
	static UserManager* GetInstance()
	{
		static UserManager instance;
		return &instance;
	}

	User* GetUser(int32 Id)
	{
		lock_guard<mutex> guard(_mutex);
		//¹º°¡ °®°í ¿È
		return nullptr;
	}

	void ProcessSave();

private:
	mutex _mutex;
};

