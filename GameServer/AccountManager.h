#pragma once
#include <mutex>

class Account
{
	//TODO
};

class AccountManager
{
public:
	static AccountManager* GetInstance()
	{
		static AccountManager instance;
		return &instance;
	}

	Account* GetAccount(int32 Id)
	{
		lock_guard<mutex> guard(_mutex);
		//¹º°¡ °®°í ¿È
		return nullptr;
	}

	void ProcessLogin();

private:
	mutex _mutex;
};

