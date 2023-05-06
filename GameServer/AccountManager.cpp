#include "pch.h"
#include "AccountManager.h"
#include "UserManager.h"

void AccountManager::ProcessLogin()
{
	// Account Lock
	lock_guard<mutex> guard(_mutex);

	// User Lock
	User* user = UserManager::GetInstance()->GetUser(100);
}
