#include "HackFrame.h"

//初始化
void Initialize(void* pBuf);

//判断游戏进程
bool IsGameProcess(HWND hGameWindow);

BOOL WINAPI DllMain(
	_In_ void* _DllHandle,
	_In_ unsigned long _Reason,
	_In_opt_ void* _Reserved)
{
	switch (_Reason)
	{
	case DLL_PROCESS_ATTACH:
		HideModule(_DllHandle);
		DisableThreadLibraryCalls(static_cast<HMODULE>(_DllHandle));
		CreateDebugConsole();
		_beginthread(Initialize, 0, NULL);
		break;
	case DLL_PROCESS_DETACH:
		ReleaseAll();
		break;
	}
	return TRUE;
}

void Initialize(void* pBuf)
{
	const char* szGameTitle = "Counter-Strike: Global Offensive";
	HWND hGameWindow = FindWindowA(NULL, szGameTitle);
	if (hGameWindow == NULL)
	{
		std::cout << "FindWindowA fail" << std::endl;
		return;
	}

	if (IsGameProcess(hGameWindow) == false)
	{
		std::cout << "Game Process fail" << std::endl;
		return;
	}

	InitializeD3DHook(hGameWindow);
}

bool IsGameProcess(HWND hGameWindow)
{
	DWORD dwCurrentID = 0, dwTargetID = 0;
	GetWindowThreadProcessId(hGameWindow, &dwTargetID);
	dwCurrentID = GetCurrentProcessId();
	return (dwCurrentID == dwTargetID);
}
