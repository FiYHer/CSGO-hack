
#define _CRT_SECURE_NO_WARNINGS

//BlackBone
#include "BlackBone/Process/Process.h"
#include "BlackBone/Patterns/PatternSearch.h"
#include "BlackBone/Process/RPC/RemoteFunction.hpp"
#include "BlackBone/Syscalls/Syscall.h"
#pragma comment(lib,"BlackBone/BlackBone.lib")
//Win32
#include <Windows.h>
#include <assert.h>
#include <process.h>
#include <stdio.h>
//C++
#include <iostream>
#include <string>
#include <list>
#include <exception>
//D3D9
#include <d3d9.h>
#pragma comment(lib,"d3d9.lib")
//imgui
#include "imgui/imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
//detours
#include "detours.h"
#pragma comment(lib,"detours/detours.lib")

//#define Load_Fonts
const char szFontPath[MAX_PATH] = "c:\\Windows\\Fonts\\msyh.ttc";	//字体文件路径，最好还是绝对路径吧，也可以放到游戏目录下

typedef HRESULT(_stdcall *Hook_DrawIndexedPrimitive)(LPDIRECT3DDEVICE9 pDevice,
	D3DPRIMITIVETYPE Type,
	INT BaseVertexIndex, 
	UINT MinVertexIndex, 
	UINT NumVertices, 
	UINT startIndex, 
	UINT primCount);
typedef HRESULT(_stdcall *Hook_Reset)(LPDIRECT3DDEVICE9 pDevice,
	D3DPRESENT_PARAMETERS* pPresentationParameters);
typedef HRESULT(_stdcall *Hook_EndScene)(LPDIRECT3DDEVICE9 pDevice);

//全局变量
IDirect3D9* g_pDirect3D = nullptr;									//D3D9指针
IDirect3DDevice9* g_pDirect3DDevice = nullptr;						//D3D设备指针

Hook_DrawIndexedPrimitive g_fnDrawIndexedPrimitive = nullptr;		//原始DrawIndexedPrimitive函数地址
Hook_Reset g_fnReset = nullptr;										//原始Reset函数地址
Hook_EndScene g_fnEndScene = nullptr;								//原始EndScene函数地址				

blackbone::Process g_cTargetProcess;								//当前游戏进程信息

HMODULE g_hModule = NULL;											//当前DLL的module	
HWND g_hTargetWindow = NULL;										//游戏窗口句柄
WNDPROC g_fnOriProc = nullptr;										//原始窗口过程

bool g_bCallOne = true;												//只调用一次代码标识
bool g_bShowMenu = true;											//显示菜单
bool g_bDrawTargetBox = false;										//绘制人物透视方框
bool g_bPerspectiveDefenders = false;								//透视保卫者
bool g_bPerspectiveSleeper = false;									//透视潜伏者

int g_nMatrixOffset = 0x4CF86E4;									//矩阵偏移
float g_fMatrix[4][4] = { 0.0f };									//矩阵
uint64_t g_nMetrixBaseAddress = 0;									//矩阵基址

std::list<UINT> g_cDefenders;										//保卫者3D模型
std::list<UINT> g_cSleeper;											//潜伏者3D模型

int g_nTargetOffset = 0xA61F84;										//敌人坐标偏移
uint64_t g_nTargetBaseAddress = 0;									//敌人坐标基址


//函数声明
//
HRESULT _stdcall MyDrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice,
	D3DPRIMITIVETYPE Type,
	INT BaseVertexIndex,
	UINT MinVertexIndex,
	UINT NumVertices,
	UINT startIndex,
	UINT primCount);

//
HRESULT _stdcall MyReset(LPDIRECT3DDEVICE9 pDevice,
	D3DPRESENT_PARAMETERS* pPresentationParameters);

//
HRESULT _stdcall MyEndScene(LPDIRECT3DDEVICE9 pDevice);

//窗口过程
LRESULT CALLBACK ModifyProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//初始化Hook
void InitializeHook(void* pBuf);

//退出
void FreeDll();

//创建一个调试控制台
void CreateDebugConsole();

//检测注入目标，非目标游戏不注入
bool CheckTarget(HWND hWnd);

//初始化ImGui
void InitializeImGui();

//Imgui相关绘制
void DrawMenu();

//初始化游戏人物模型标识
void InitPlayerVerBuferNumbers();

//绘制一个方框
void DrawBox(int nPosX, int nPosY, int nWidth, int nHeight, D3DCOLOR dwColor, IDirect3DDevice9* pDevice);

//初始化敌人信息
void InitTargetInfo();

//绘制人物透视方框
void DrawPlayerBox(IDirect3DDevice9* pDevice);


#define Load_Fonts	//启用指定字体
BOOL WINAPI DllMain(
	_In_ void* _DllHandle,
	_In_ unsigned long _Reason,
	_In_opt_ void* _Reserved)
{
	if (_Reason == DLL_PROCESS_ATTACH)
	{
		/********************************************************************************/
		/**						 这里设置游戏窗口的窗口名                               */
		/**/const char szGameWindow[MAX_PATH] = "Counter-Strike: Global Offensive";
		/********************************************************************************/
		DisableThreadLibraryCalls(static_cast<HMODULE>(_DllHandle));
		g_hModule = static_cast<HMODULE>(_DllHandle);
		g_hTargetWindow = FindWindowA(0, szGameWindow);
		if (g_hTargetWindow)
		{
			CreateDebugConsole();
			_beginthread(InitializeHook, 0, 0);
		}
	}
	else if (_Reason == DLL_PROCESS_DETACH)
	{
		FreeDll();
	}
	return TRUE;
}

HRESULT _stdcall MyDrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice,
	D3DPRIMITIVETYPE Type,	//所要绘制的图元类型
	INT BaseVertexIndex,	//为索引增加的一个基数
	UINT MinVertexIndex,	//索引缓冲区中最小的索引号
	UINT NumVertices,		//渲染过程需要处理的顶点数
	UINT startIndex,		//索引缓冲区中从那个索引开始渲染顶点
	UINT primCount)			//渲染图元的个数
{
	IDirect3DVertexBuffer9* pVertexBuffer = nullptr;	//顶点缓存区
	UINT nStride = 0, nOffset = 0;						//模型的标识
	bool bTargetModel = false;							//是否是指定3D模型
	HRESULT hRet = 0;
	if (g_bPerspectiveDefenders)
	{
		for (auto it : g_cDefenders)
		{
			if (it == NumVertices)
			{
				bTargetModel = true;
				break;
			}
		}
	}
	if (g_bPerspectiveSleeper)
	{
		for (auto it : g_cSleeper)
		{
			if (it == NumVertices)
			{
				bTargetModel = true;
				break;
			}
		}
	}
	if(bTargetModel)
	{
		pDevice->GetStreamSource(0, &pVertexBuffer, &nOffset, &nStride);
		pDevice->SetRenderState(D3DRS_ZENABLE, false);
		hRet = g_fnDrawIndexedPrimitive(pDevice, Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	}
	if (bTargetModel)
	{
		pDevice->SetRenderState(D3DRS_ZENABLE, true);
		if (pVertexBuffer)
			pVertexBuffer->Release();
	}
	hRet = g_fnDrawIndexedPrimitive(pDevice, Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	return hRet;
}

HRESULT _stdcall MyReset(LPDIRECT3DDEVICE9 pDevice,
	D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hRet = g_fnReset(pDevice, pPresentationParameters);
	ImGui_ImplDX9_CreateDeviceObjects();
	return hRet;
}

HRESULT _stdcall MyEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	if (g_bCallOne)
	{
		g_bCallOne = false;					//确保只调用一次这些函数
		g_pDirect3DDevice = pDevice;		//如果没有这一句，Imgui界面将显示不出来
		InitializeImGui();					//初始化ImGui
		g_fnOriProc = (WNDPROC)SetWindowLongPtrA(g_hTargetWindow, GWLP_WNDPROC, (LONG)ModifyProc);	//替换窗口过程

		//初始化人物模型标识
		InitPlayerVerBuferNumbers();

		InitTargetInfo();
	}
	
	DrawPlayerBox(pDevice);
	DrawMenu();								//绘制Imgui

	return g_fnEndScene(pDevice);
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ModifyProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	if (uMsg == WM_KEYDOWN)
	{
		switch (wParam)
		{
		case VK_F12:
			g_bShowMenu = !g_bShowMenu;
			break;
		}
	}

	if (g_fnOriProc)
		return CallWindowProcA(g_fnOriProc, hWnd, uMsg, wParam, lParam);

	return 1;
}

void InitializeHook(void* pBuf)
{
	auto RunTimeError = [&](const char* szText) ->void
	{
		throw std::runtime_error(szText);
	};

	try
	{
		if (CheckTarget(g_hTargetWindow) == false)
			RunTimeError("注入进程与目标进程不匹配");

		g_pDirect3D = Direct3DCreate9(D3D_SDK_VERSION);
		if (g_pDirect3D == NULL)
			RunTimeError("创建D3D失败");

		D3DPRESENT_PARAMETERS stPresent;
		ZeroMemory(&stPresent, sizeof(D3DPRESENT_PARAMETERS));
		stPresent.Windowed = TRUE;
		stPresent.SwapEffect = D3DSWAPEFFECT_DISCARD;
		stPresent.BackBufferFormat = D3DFMT_UNKNOWN;
		stPresent.EnableAutoDepthStencil = TRUE;
		stPresent.AutoDepthStencilFormat = D3DFMT_D16;
		if (FAILED(g_pDirect3D->CreateDevice(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			g_hTargetWindow,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			&stPresent,
			&g_pDirect3DDevice)))
			RunTimeError("创建D3D设备失败");

		LPDWORD pDeviceTable = (LPDWORD)*(LPDWORD)g_pDirect3DDevice;
		g_fnReset = reinterpret_cast<Hook_Reset>(pDeviceTable[16]);
		g_fnEndScene = reinterpret_cast<Hook_EndScene>(pDeviceTable[42]);
		g_fnDrawIndexedPrimitive = reinterpret_cast<Hook_DrawIndexedPrimitive>(pDeviceTable[82]);

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(LPVOID&)g_fnReset, reinterpret_cast<void*>(MyReset));
		DetourAttach(&(LPVOID&)g_fnEndScene, reinterpret_cast<void*>(MyEndScene));
		DetourAttach(&(LPVOID&)g_fnDrawIndexedPrimitive, reinterpret_cast<void*>(MyDrawIndexedPrimitive));
		DetourTransactionCommit();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}

void FreeDll()
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	if (g_pDirect3D)
	{
		g_pDirect3D->Release();
		g_pDirect3D = nullptr;
	}
	if (g_pDirect3DDevice)
	{
		g_pDirect3DDevice->Release();
		g_pDirect3DDevice = nullptr;
	}

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(LPVOID&)g_fnReset, reinterpret_cast<void*>(MyReset));
	DetourDetach(&(LPVOID&)g_fnEndScene, reinterpret_cast<void*>(MyEndScene));
	DetourDetach(&(LPVOID&)g_fnDrawIndexedPrimitive, reinterpret_cast<void*>(MyDrawIndexedPrimitive));
	DetourTransactionCommit();
}

void CreateDebugConsole()
{
#ifdef _DEBUG
	assert(AllocConsole() != FALSE);
	assert(SetConsoleTitleA("CS:GO Log") != FALSE);
	freopen("CON", "w", stdout);
#endif // _DEBUG
}

bool CheckTarget(HWND hWnd)
{
	DWORD dwCurrentID = 0, dwTargetID = 0;
	GetWindowThreadProcessId(hWnd, &dwTargetID);
	dwCurrentID = GetCurrentProcessId();
	return (dwCurrentID == dwTargetID);
}

void InitializeImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsLight();

	ImGui_ImplWin32_Init(g_hTargetWindow);
	ImGui_ImplDX9_Init(g_pDirect3DDevice);	//这里确保Device指针的正确性,惨痛教训害我调试是一个晚上！！！
#ifdef Load_Fonts
	ImFont* font = io.Fonts->AddFontFromFileTTF(szFontPath, 15.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
	IM_ASSERT(font != NULL);
#endif // Load_Fonts
}

void DrawMenu()
{
	if (g_bShowMenu)
	{
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin(u8"CS:GO游戏辅助模块", &g_bShowMenu);
		ImGui::Text(u8"我们不生产代码,我们只是代码的搬运工");
		ImGui::Checkbox(u8"保卫者透视",&g_bPerspectiveDefenders);
		ImGui::Checkbox(u8"潜伏者透视", &g_bPerspectiveSleeper);
		ImGui::Checkbox(u8"绘制人物方框", &g_bDrawTargetBox);
		ImGui::SliderFloat(u8"矩阵第一个数", &g_fMatrix[0][0], -20.0f, -20.0f);
		ImGui::End();

		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	}
}

void InitPlayerVerBuferNumbers()
{
	//保卫者和潜伏者的人物模型的VerBufferNumber
	UINT nDefenderArray[] = { 140,1310,1383,1432,1761,1677,2052,2118,3763 };
	UINT nSleeperArray[] = { 622,903,984,1006,2369,2663,3225,3692,3742,3932,3845,5008,5781 };

	for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
		g_cDefenders.emplace_back(nDefenderArray[i]);
	for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
		g_cSleeper.emplace_back(nSleeperArray[i]);
}

void DrawBox(int nPosX, int nPosY, int nWidth, int nHeight, D3DCOLOR dwColor, IDirect3DDevice9* pDevice)
{
	auto FillRgbColor = [&](int x,int y,int w,int h) -> void
	{
		x = (x <= 0) ? 1 : x;
		y = (y <= 0) ? 1 : y;
		w = (w <= 0) ? 1 : w;
		h = (h <= 0) ? 1 : h;

		D3DRECT stRect = { x,y,x + w,y + h };
		pDevice->Clear(1, &stRect, D3DCLEAR_TARGET, dwColor, 0.0f, 0);
	};
	auto DrawBorder = [&](int x, int y, int w, int h,int nPixel = 1) -> void
	{
		FillRgbColor(x, y + h, w, nPixel);			//绘制下边
		FillRgbColor(x, y, nPixel, h);				//绘制左边
		FillRgbColor(x, y, w, nPixel);				//绘制上边
		FillRgbColor(x + w, y, nPixel, h);			//绘制右边
	};

	DrawBorder(nPosX, nPosY, nWidth, nHeight);
}

void InitTargetInfo()
{
	try
	{
		if (!NT_SUCCESS(g_cTargetProcess.Attach(GetCurrentProcessId())))
			throw std::runtime_error("获取进程信息失败");

		blackbone::ProcessModules& cModule = g_cTargetProcess.modules();
		blackbone::ProcessMemory& cMemory = g_cTargetProcess.memory();
		g_nMetrixBaseAddress = cModule.GetModule(std::wstring(L"client_panorama.dll"))->baseAddress + g_nMatrixOffset;
		g_nTargetBaseAddress = cModule.GetModule(std::wstring(L"server.dll"))->baseAddress + g_nTargetOffset;
		cMemory.Protect(g_nMetrixBaseAddress, sizeof(g_fMatrix), PAGE_EXECUTE_READWRITE);
		cMemory.Protect(g_nTargetBaseAddress, sizeof(float) * 90, PAGE_EXECUTE_READWRITE);
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}

void DrawPlayerBox(IDirect3DDevice9* pDevice)
{
	if (g_bDrawTargetBox)
	{
		D3DVIEWPORT9 stView;
		pDevice->GetViewport(&stView);
		DWORD dwHalfWidth = stView.Width / 2;
		DWORD dwHalfHeight = stView.Height / 2;

		int nTargetIndex = 9;
		float stTargetPos[90];
		g_cTargetProcess.memory().Read(g_nTargetBaseAddress, sizeof(float) * 90, stTargetPos);
		g_cTargetProcess.memory().Read(g_nMetrixBaseAddress, sizeof(g_fMatrix), &g_fMatrix);

		for (int i = 0; i <= 10; i++, nTargetIndex += 9)
		{
			if (stTargetPos[nTargetIndex + 2] > 300.0f)
				continue;

			//朝向
			float fTarget =
				g_fMatrix[2][0] * stTargetPos[nTargetIndex]
				+ g_fMatrix[2][1] * stTargetPos[nTargetIndex + 1]
				+ g_fMatrix[2][2] * stTargetPos[nTargetIndex + 2]
				+ g_fMatrix[2][3];

			if (fTarget < 0.01f)//我们后面的敌人不需要方框透视
				continue;

			fTarget = 1.0f / fTarget;

			float fBoxX = dwHalfWidth
				+ (g_fMatrix[0][0] * stTargetPos[nTargetIndex]
					+ g_fMatrix[0][1] * stTargetPos[nTargetIndex + 1]
					+ g_fMatrix[0][2] * stTargetPos[nTargetIndex + 2]
					+ g_fMatrix[0][3])*fTarget*dwHalfWidth;

			float fBoxY_H = dwHalfHeight
				- (g_fMatrix[1][0] * stTargetPos[nTargetIndex]
					+ g_fMatrix[1][1] * stTargetPos[nTargetIndex + 1]
					+ g_fMatrix[1][2] * (stTargetPos[nTargetIndex + 2] + 75.0f)
					+ g_fMatrix[1][3])*fTarget*dwHalfHeight;

			float fBoxY_W = dwHalfHeight
				- (g_fMatrix[1][0] * stTargetPos[nTargetIndex]
					+ g_fMatrix[1][1] * stTargetPos[nTargetIndex + 1]
					+ g_fMatrix[1][2] * (stTargetPos[nTargetIndex + 2] - 5.0f)
					+ g_fMatrix[1][3])*fTarget*dwHalfHeight;

			DrawBox(fBoxX - ((fBoxY_W - fBoxY_H) / 4), fBoxY_H,
				(fBoxY_W - fBoxY_H) / 2, (fBoxY_W - fBoxY_H),
				D3DCOLOR_XRGB(0, 255, 0), pDevice);
		}
	}
}
//0xA61F84