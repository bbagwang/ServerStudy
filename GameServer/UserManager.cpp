#include "pch.h"
#include "UserManager.h"
#include "AccountManager.h"

void UserManager::ProcessSave()
{
	//// User Lock
	//lock_guard<mutex> guard(_mutex);

	// Account Lock
	Account* account = AccountManager::GetInstance()->GetAccount(100);

	// User Lock
	lock_guard<mutex> guard(_mutex);
}
