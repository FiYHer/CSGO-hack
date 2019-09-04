#include "HackFrame.h"

IDirect3D9* g_pOwnDirect3D = nullptr;							//自己的IDirect3D9
IDirect3DDevice9* g_pOwnDirect3DDevice = nullptr;				//自己的IDirect3DDevice9

IDirect3D9* g_pGameDirect3D = nullptr;							//游戏里面的IDirect3D9
IDirect3DDevice9* g_pGameDirect3DDevice = nullptr;				//游戏里面的IDirect3DDevice9

Type_Reset g_fReset = nullptr;									//Reset
Type_Present g_fPresent = nullptr;								//Present
Type_EndScene g_fEndScene = nullptr;							//EndScene
Type_DrawIndexedPrimitive g_fDrawIndexedPrimitive = nullptr;	//DrawIndexedPrimitive

HWND g_hGameWindow = NULL;										//游戏窗口句柄
WNDPROC g_fOriginProc = nullptr;								//原窗口函数地址

bool g_bCallOne = true;											//只调用一次
bool g_bShowImgui = true;										//显示imgui菜单
bool g_bShowDefenders = false;									//显示保卫者
bool g_bShowSleeper = false;									//显示潜伏者
bool g_bDrawBox = false;										//显示人物方框
bool g_bTellFirearms = false;									//提示枪械
bool g_bAiMBotDefenders = false;								//保卫自瞄
bool g_bAiMBotSleepers = false;									//潜伏自瞄

std::list<UINT> g_cDefenderStride;								//保卫者标识
std::list<UINT> g_cSleeperStride;								//潜伏者标识
std::list<Firearm> g_cFirearm;									//枪械标识

char g_szFirearm[MAX_PATH] = { 0 };								//枪械文本									

int g_nMetrixBaseAddress = 0;									//矩阵基址
int g_nAngleBaseAddress = 0;									//偏航角，俯仰角基址
int g_nOwnBaseAddress = 0;										//自己基址
int g_nTargetBaseAddress = 0;									//敌人基址

blackbone::Process g_cGame;										//游戏进程信息

void CreateDebugConsole()
{
#if _DEBUG
	AllocConsole();
	SetConsoleTitleA("HackFrame Log");
	freopen("CON", "w", stdout);
#endif
}

bool InitializeD3DHook(_In_ HWND hGameWindow)
{
	g_hGameWindow = hGameWindow;
	try
	{
		g_pOwnDirect3D = Direct3DCreate9(D3D_SDK_VERSION);
		if (g_pOwnDirect3D == nullptr)
			throw std::runtime_error("Direct3DCreate9 fail");

		D3DPRESENT_PARAMETERS stPresent;
		ZeroMemory(&stPresent, sizeof(D3DPRESENT_PARAMETERS));
		stPresent.Windowed = TRUE;
		stPresent.SwapEffect = D3DSWAPEFFECT_DISCARD;
		stPresent.BackBufferFormat = D3DFMT_UNKNOWN;
		stPresent.EnableAutoDepthStencil = TRUE;
		stPresent.AutoDepthStencilFormat = D3DFMT_D16;
		if (FAILED(g_pOwnDirect3D->CreateDevice(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			hGameWindow,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			&stPresent,
			&g_pOwnDirect3DDevice)))
			throw std::runtime_error("CreateDevice fail");

		int* pDirect3DDeviceTable = (int*)*(int*)g_pOwnDirect3DDevice;

		g_fReset = reinterpret_cast<Type_Reset>(pDirect3DDeviceTable[F_Reset]);
		g_fPresent = reinterpret_cast<Type_Present>(pDirect3DDeviceTable[F_Present]);
		g_fEndScene = reinterpret_cast<Type_EndScene>(pDirect3DDeviceTable[F_EndScene]);
		g_fDrawIndexedPrimitive = reinterpret_cast<Type_DrawIndexedPrimitive>(pDirect3DDeviceTable[F_DrawIndexedPrimitive]);

		if (DetourTransactionBegin() != NO_ERROR)
			throw std::runtime_error("DetourTransactionBegin fail");
		if (DetourUpdateThread(GetCurrentThread()) != NO_ERROR)
			throw std::runtime_error("DetourUpdateThread fail");
		if (DetourAttach(&(LPVOID&)g_fReset, MyReset) != NO_ERROR)
			throw std::runtime_error("Hook Reset fail");
		if (DetourAttach(&(LPVOID&)g_fPresent, MyPresent) != NO_ERROR)
			throw std::runtime_error("Hook Present fail");
		if (DetourAttach(&(LPVOID&)g_fEndScene, MyEndScene) != NO_ERROR)
			throw std::runtime_error("Hook EndScene fail");
		if (DetourAttach(&(LPVOID&)g_fDrawIndexedPrimitive, MyDrawIndexedPrimitive) != NO_ERROR)
			throw std::runtime_error("Hook DrawIndexedPrimitive fail");
		if (DetourTransactionCommit() != NO_ERROR)
			throw std::runtime_error("DetourTransactionCommit fail");

	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return false;
	}
	return true;
}

void ReleaseAll()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(LPVOID&)g_fReset, MyReset);
	DetourDetach(&(LPVOID&)g_fPresent, MyPresent);
	DetourDetach(&(LPVOID&)g_fEndScene, MyEndScene);
	DetourDetach(&(LPVOID&)g_fDrawIndexedPrimitive, MyDrawIndexedPrimitive);
	DetourTransactionCommit();

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

HRESULT _stdcall MyReset(IDirect3DDevice9* pDirect3DDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hRet = g_fReset(pDirect3DDevice, pPresentationParameters);
	ImGui_ImplDX9_CreateDeviceObjects();
	return hRet;
}

HRESULT _stdcall MyPresent(IDirect3DDevice9* pDirect3DDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
	return g_fPresent(pDirect3DDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

HRESULT _stdcall MyEndScene(IDirect3DDevice9* pDirect3DDevice)
{
	if (g_bCallOne)
	{
		g_bCallOne = false;
		g_pGameDirect3DDevice = pDirect3DDevice;

		g_fOriginProc = (WNDPROC)SetWindowLongPtrA(g_hGameWindow, GWL_WNDPROC, (LONG)MyWindowProc);
		InitializeImgui();
		InitializeData();
		InitializeFirearm();
	}

	DrawPlayerBox(pDirect3DDevice);
	DrawImgui();

	return g_fEndScene(pDirect3DDevice);
}

HRESULT _stdcall MyDrawIndexedPrimitive(IDirect3DDevice9 * pDirect3DDevice, D3DPRIMITIVETYPE stType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
	//if (NumVertices <= g_nStride)
	//	g_fDrawIndexedPrimitive(pDirect3DDevice, stType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	//return S_OK;
	bool bTargetModel = false;
	auto CheckDefender = [&]()->void
	{
		if (bTargetModel == false && g_bShowDefenders)
		{
			for (auto it : g_cDefenderStride)
			{
				if (it == NumVertices)
				{
					bTargetModel = true;
					break;
				}
			}
		}
	};
	auto CheckSleeper = [&]()->void
	{
		if (bTargetModel == false && g_bShowSleeper)
		{
			for (auto it : g_cSleeperStride)
			{
				if (it == NumVertices)
				{
					bTargetModel = true;
					break;
				}
			}
		}
	};
	auto CheckFirearm = [&]()->void
	{
		if (g_bTellFirearms)
		{
			for (auto it :g_cFirearm)
			{
				if (it.nCount == NumVertices)
				{
					strncpy(g_szFirearm, it.szName, sizeof(it.szName));
					break;
				}
			}
		}
	};
	
	CheckDefender();
	CheckSleeper();
	CheckFirearm();

	IDirect3DBaseTexture9* pOldTexture = nullptr;
	IDirect3DTexture9* pNewTexture = nullptr;
	if (bTargetModel)
	{
		pDirect3DDevice->SetRenderState(D3DRS_ZENABLE, false);
		pDirect3DDevice->GetTexture(0, &pOldTexture);
		pDirect3DDevice->SetTexture(0, pNewTexture);
		g_fDrawIndexedPrimitive(pDirect3DDevice, stType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
		pDirect3DDevice->SetRenderState(D3DRS_ZENABLE, true);
		pDirect3DDevice->SetTexture(0, pOldTexture);
	}
	return g_fDrawIndexedPrimitive(pDirect3DDevice, stType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

bool InitializeImgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsLight();

	ImGui_ImplWin32_Init(g_hGameWindow);
	ImGui_ImplDX9_Init(g_pGameDirect3DDevice);
#ifdef Load_Fonts
	ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\msyh.ttc", 15.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
	IM_ASSERT(font != NULL);
#endif // Load_Fonts
	return true;
}

void DrawImgui()
{
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (g_bShowImgui)
	{
		ImGui::Begin(u8"CSGO透视自瞄辅助模块", &g_bShowImgui);
		ImGui::Text(u8"按键说明:F12		菜单显示开关");
		ImGui::Text(u8"按键说明:Shift	自瞄最近敌人");
		ImGui::Checkbox(u8"透视保卫者", &g_bShowDefenders);
		ImGui::Checkbox(u8"透视潜伏者", &g_bShowSleeper);
		ImGui::Checkbox(u8"自瞄保卫者", &g_bAiMBotDefenders);
		ImGui::Checkbox(u8"自瞄潜伏者", &g_bAiMBotSleepers);
		ImGui::Checkbox(u8"人物的方框", &g_bDrawBox);
		ImGui::Checkbox(u8"枪械的预判", &g_bTellFirearms);

		static int nMapSelect = -1;
		const char* ComBoList[] = { 
			u8"炙热沙城2", u8"荒漠迷城", u8"炼狱小镇", u8"殒命大厦", 
			u8"古堡激战", u8"死城之谜", u8"动物园", u8"红宝石酒镇", 
			u8"泰坦特公司", u8"海滨危机", u8"列车停放站", u8"死亡游乐园",
			u8"核子危机", u8"运河水城" ,u8"办公大楼",u8"佣兵训练营",
			u8"办公室",u8"意大利小镇",u8"仓库突击"};
		if (ImGui::Combo(u8"地图选择", &nMapSelect, ComBoList, IM_ARRAYSIZE(ComBoList)))
			InitializeStride(static_cast<GameMap>(nMapSelect));
		ImGui::End();
	}

	if (g_bTellFirearms)
	{
		ImGui::Begin(u8"枪械提示窗口", &g_bTellFirearms);
		ImGui::Text(g_szFirearm);
		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	if (uMsg == WM_KEYDOWN)
	{
		if (wParam == VK_F12)
			g_bShowImgui = !g_bShowImgui;		
	}

	if (g_fOriginProc)
		return CallWindowProcA(g_fOriginProc, hWnd, uMsg, wParam, lParam);

	return true;
}

void InitializeStride(GameMap eMap)
{
	g_cDefenderStride.clear();
	g_cSleeperStride.clear();

	switch (eMap)
	{
	case Map_HotCity:					//炙热沙城2
	{
		UINT nDefenderArray[] = { 140,1310,1383,1432,1761,1677,2052,2118,3763 };
		UINT nSleeperArray[] = { 622,903,984,1006,2369,2663,3225,3692,3742,3932,3845,5008,5781 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_DesertMaze:				//荒漠迷城
	{	
		UINT nDefenderArray[] = { 136,140,1310,1383,1432,1967,2052,2482,5299 };
		UINT nSleeperArray[] = { 290,622,903,1006,2369,2663,3225,3692,3742,3845,5008,5781,3932,5572 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_PurgatoryTown:				//炼狱小镇
	{
		UINT nDefenderArray[] = { 136,140,1310,1383,1432,1967,2052,2482,5299 };
		UINT nSleeperArray[] = { 290,903,1106,1914,2369,2536 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_DeathBuilding:				//殒命大厦
	{
		UINT nDefenderArray[] = { 140,1222,1439,1735,1702,1310,1374,1432,2052,2254,2266,2489,2856,3093,7163 };
		UINT nSleeperArray[] = { 200,290,903,924,927,1053,1197,2056,2112,3368 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_GubaoFighting:				//古堡激战
	{
		UINT nDefenderArray[] = { 140,256,1258,1432,1601,1611,1645,2052,3630 };
		UINT nSleeperArray[] = { 290,622,903,1395,2112,3164,4410 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_DeadCity:					//死城之谜
	{
		UINT nDefenderArray[] = { 140,256,1258,1310,1383,1432,1601,1611,1645,2052,3630 };
		UINT nSleeperArray[] = { 290,622,903,1395,2112,2369,3164,4410 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_Zoo:						//动物园
	{
		UINT nDefenderArray[] = { 140,1310,1374,1432,1222,1439,1735,1702,2052,3152,7163 };
		UINT nSleeperArray[] = { 290,622,903,1395,2112,2369,3164,4410 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_RubyWineTown:				//红宝石酒镇
	{
		UINT nDefenderArray[] = { 136,140,1310,1383,1432,1967,2052,2482,5299 };
		UINT nSleeperArray[] = { 290,903,1462,1671,1925,2369,2447 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_TangentCompany:			//坦根特公司
	{
		UINT nDefenderArray[] = { 140,1222,1702,1735,1310,1374,1383,1432,2052,2254,2266,2856,7163 };
		UINT nSleeperArray[] = { 200,290,903,924,927,1053,1197,2056,2112,2369,3368 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_CoastalCrisis:				//海滨危机
	{
		UINT nDefenderArray[] = { 140,1222,1439,1702,1735,1310,1374,1383,1432,2052,2254,2266,2489,3093,7163 };
		UINT nSleeperArray[] = { 290,692,903,1003,1183,1314,1380,1996,2369,7236 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_TrainpParkingStation:		//列车停放站
	{
		UINT nDefenderArray[] = { 252,290,903,1214,1215,1345,1624,1273,1525,2112,2369,3836,4159 };
		UINT nSleeperArray[] = { 64,140,1310,1383,1432,2639,3137,3245,4084,2924,4320,4510,4422,4533 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_DeathAmusementPark:		//死亡游乐园
	{
		UINT nDefenderArray[] = { 116,140,1383,1410,1432,2052,2113,2151,2157,2239 };
		UINT nSleeperArray[] = { 290,622,903,1395,2112,2369,3164,4410 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_NuclearCrisis:				//核子危机
	{
		UINT nDefenderArray[] = { 140,1222,1439,1735,1702,1310,1374,1383,1432,2052,2266,2489,2856,7163 };
		UINT nSleeperArray[] = { 290,622,903,1395,2112,2369,3164,4410 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_CanalWaterCity:			//运河水城
	{
		UINT nDefenderArray[] = { 120,1310,1383,1432,2052,2539,2963,3137,3245,2924,3220,4510,4533,4666 };
		UINT nSleeperArray[] = { 290,622,903,1395,2112,2369,3164,4410 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_OfficeBuilding:			//办公大楼
	{
		UINT nDefenderArray[] = { 140,1222,1439,1735,1702,1310,1374,1383,2052,2489,3093,7163 };
		UINT nSleeperArray[] = { 200,290,646,927,1053,1197,903,2056,2112,2369,3368 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_MercenaryTrainingCamp:		//佣兵训练营
	{
		UINT nDefenderArray[] = { 140,1222,1439,1702,1310,1374,1383,2052,2254,2489,2856,3093,7163 };
		UINT nSleeperArray[] = { 290,622,903,1395,2112,2369,3164,4410 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_Office:					//办公室
	{
		UINT nDefenderArray[] = { 140,1222,1439,1735,1702,1310,1383,1374,2052,2266,2856,7163 };
		UINT nSleeperArray[] = { 290,903,1462,1671,1925,2369,2447 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_ItalianTown:				//意大利小镇
	{
		UINT nDefenderArray[] = { 140,256,1310,1258,1645,1611,1601,1645,2052,3630 };
		UINT nSleeperArray[] = { 290,903,1106,1914,2112,2369,2536 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	case Map_WarehouseAssault:			//仓库突击
	{
		UINT nDefenderArray[] = { 140,1222,1439,1735,1702,1310,1383,1374,2052,2266,2856,3093,2489 };
		UINT nSleeperArray[] = { 290,622,903,1395,2112,2369,3164,4410 };
		for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
			g_cDefenderStride.emplace_back(nDefenderArray[i]);
		for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
			g_cSleeperStride.emplace_back(nSleeperArray[i]);
		break;
	}
	}
}

void InitializeData()
{
	g_cGame.Attach(GetCurrentProcessId());
	auto nClientBaseAddress = g_cGame.modules().GetModule(std::wstring(L"client_panorama.dll"))->baseAddress;
	auto nEngineBaseAddress = g_cGame.modules().GetModule(std::wstring(L"engine.dll"))->baseAddress;

	int nMatrixOffset = 0x4cf9804;
	int nAngleOffset = 0x590D94;
	int nOwnBaseOffset = 0xCF5A4C;
	int nTargetOffset = 0x4D07DE4;
	
	g_nMetrixBaseAddress = static_cast<int>(nClientBaseAddress) + nMatrixOffset;
	g_nAngleBaseAddress = static_cast<int>(nEngineBaseAddress) + nAngleOffset;
	g_nOwnBaseAddress = static_cast<int>(nClientBaseAddress) + nOwnBaseOffset;
	g_nTargetBaseAddress = static_cast<int>(nClientBaseAddress) + nTargetOffset;
}

void InitializeFirearm()
{
	Firearm stFireData[] =
	{
	{2258,u8"重型武器 新星"},
	{3796,u8"重型武器 MX1014"},
	{1941,u8"重型武器 MAG-7"},
	{2217,u8"重型武器 截短散弹枪"},
	{4280,u8"重型武器 M249"},
	{4122,u8"重型武器 内格夫"},

	{3851,u8"微型冲锋枪 MP9"},
	{2729,u8"微型冲锋枪 MAC-10"},
	{4485,u8"微型冲锋枪 MP7"},
	{3152,u8"微型冲锋枪 UMP5"},
	{4480,u8"微型冲锋枪 P90"},
	{3398,u8"微型冲锋枪 PP-野牛"},

	{3633,u8"步枪 法玛斯"},
	{15252,u8"步枪 加利尔AR"},
	{3661,u8"步枪 Ak-47"},
	{2770,u8"步枪 M4A1"},
	{2899,u8"步枪 SSG-08"},
	{4375,u8"步枪 SG-553"},
	{1941,u8"步枪 AUG "},
	{3685,u8"步枪 AWP"},
	{3411,u8"步枪 G3SG1"},
	{4557,u8"步枪 SCAR-20"},

	{1871,u8"装备 宙斯 X275"},

	{1310,u8"手枪 P2000"},
	{2369,u8"手枪 格洛克18"},
	{2322,u8"手枪 双持贝瑞塔"},
	{1766,u8"手枪 P250"},
	{2847,u8"手枪 Tec-9"},
	{1426,u8"手枪 FN-57"},
	{2140,u8"手枪 沙漠之鹰"},

	{4329,u8"其它 C4炸弹"}
	};

	for (int i = 0; i < sizeof(stFireData) / sizeof(Firearm); i++)
	{
		g_cFirearm.emplace_back(stFireData[i]);
	}
}

void DrawBox(IDirect3DDevice9* pDirect3DDevice, D3DCOLOR dwColor, int x, int y, int w, int h)
{
	auto FillRgbColor = [&](int x, int y, int w, int h) -> void
	{
		x = (x <= 0) ? 1 : x;
		y = (y <= 0) ? 1 : y;
		w = (w <= 0) ? 1 : w;
		h = (h <= 0) ? 1 : h;

		D3DRECT stRect = { x,y,x + w,y + h };
		pDirect3DDevice->Clear(1, &stRect, D3DCLEAR_TARGET, dwColor, 0.0f, 0);
	};
	auto DrawBorder = [&](int x, int y, int w, int h, int nPixel = 1) -> void
	{
		FillRgbColor(x, y + h, w, nPixel);			//绘制下边
		FillRgbColor(x, y, nPixel, h);				//绘制左边
		FillRgbColor(x, y, w, nPixel);				//绘制上边
		FillRgbColor(x + w, y, nPixel, h);			//绘制右边
	};

	DrawBorder(x, y, w, h);
}

void DrawPlayerBox(IDirect3DDevice9* pDirect3DDevice)
{
	if (g_bDrawBox)
	{
		//获取游戏窗口的一半宽度和高度
		D3DVIEWPORT9 stView;
		if (FAILED(pDirect3DDevice->GetViewport(&stView)))return;
		DWORD dwHalfWidth = stView.Width / 2;
		DWORD dwHalfHeight = stView.Height / 2;

		int nOwnPosBaseAddress = 0;							//自己坐标基地址
		int nOwnPosOffset = 0x35A4;							//自己坐标偏移
		float pOwnPos[Coordinate_Number];					//自己坐标

		int nBoxColor = D3DCOLOR_XRGB(0, 0, 255);			//人物方框颜色
		int nPlayerCamp;									//人物阵营
		int nPlayerCmapOffset = 0xf4;						//人物阵营偏移

		int nPlayerAddress = 0;								//人物基址
		int nNextPlayerAddress = 0x10;						//下一个人物的地址偏移
		int nPlayerPosOffset = 0xa0;						//人物地址偏移

		int nPlayerBlood = 0;								//人物血量
		int nPlayerBloodOffset = 0x100;						//人物血量偏移

		float fPlayerPos[Coordinate_Number];				//人物地址
		AiMBotInfo stAiMBotInfo;							//最近的人物坐标

		float fOwnMatrix[4][4];								//矩阵

		//读取矩阵信息
		g_cGame.memory().Read(g_nMetrixBaseAddress, sizeof(fOwnMatrix), fOwnMatrix);
		//读取内存自己基址
		g_cGame.memory().Read(g_nOwnBaseAddress, sizeof(int), &nOwnPosBaseAddress);
		//根据内存基地址加上偏移得到我们当前的坐标
		g_cGame.memory().Read(nOwnPosBaseAddress + nOwnPosOffset, sizeof(pOwnPos), pOwnPos);

		for (int i = 0; i <= 20; i++)
		{
			//读取人物基址
			g_cGame.memory().Read(g_nTargetBaseAddress + i * nNextPlayerAddress, sizeof(int), &nPlayerAddress);
			if(nPlayerAddress == 0)
				continue;

			//读取人物坐标
			g_cGame.memory().Read(nPlayerAddress + nPlayerPosOffset, sizeof(fPlayerPos), fPlayerPos);
			//读取人物阵营
			g_cGame.memory().Read(nPlayerAddress + nPlayerCmapOffset, sizeof(nPlayerCamp), &nPlayerCamp);
			//读取人物血量
			g_cGame.memory().Read(nPlayerAddress + nPlayerBloodOffset, sizeof(nPlayerBlood), &nPlayerBlood);

			//阵营
			//if ((nPlayerCamp == 2 && g_bAiMBotSleepers) || (nPlayerCamp == 3 && g_bAiMBotDefenders))
			//{
			//	float fX = (pOwnPos[Pos_X] - fPlayerPos[Pos_X])*(pOwnPos[Pos_X] - fPlayerPos[Pos_X]);
			//	float fY = (pOwnPos[Pos_Y] - fPlayerPos[Pos_Y])*(pOwnPos[Pos_Y] - fPlayerPos[Pos_Y]);
			//	float fZ = (pOwnPos[Pos_Z] - fPlayerPos[Pos_Z])*(pOwnPos[Pos_Z] - fPlayerPos[Pos_Z]);
			//	stAiMBotInfo.fVector = sqrt(fX + fY + fZ);
			//	stAiMBotInfo.fPlayerPos[Pos_X] = fPlayerPos[Pos_X];
			//	stAiMBotInfo.fPlayerPos[Pos_Y] = fPlayerPos[Pos_Y];
			//	stAiMBotInfo.fPlayerPos[Pos_Z] = fPlayerPos[Pos_Z];
			//}

			//当人物死亡
			if (nPlayerBlood <= 0)
				continue;

			//朝向
			float fTarget =
				fOwnMatrix[2][0] * fPlayerPos[Pos_X]
				+ fOwnMatrix[2][1] * fPlayerPos[Pos_Y]
				+ fOwnMatrix[2][2] * fPlayerPos[Pos_Z]
				+ fOwnMatrix[2][3];

			//我们后面的敌人不需要方框透视
			if (fTarget < 0.01f)
				continue;

			fTarget = 1.0f / fTarget;

			float fBoxX = dwHalfWidth
				+ (fOwnMatrix[0][0] * fPlayerPos[Pos_X]
					+ fOwnMatrix[0][1] * fPlayerPos[Pos_Y]
					+ fOwnMatrix[0][2] * fPlayerPos[Pos_Z]
					+ fOwnMatrix[0][3])*fTarget*dwHalfWidth;

			float fBoxY_H = dwHalfHeight
				- (fOwnMatrix[1][0] * fPlayerPos[Pos_X]
					+ fOwnMatrix[1][1] * fPlayerPos[Pos_Y]
					+ fOwnMatrix[1][2] * (fPlayerPos[Pos_Z] + 75.0f)
					+ fOwnMatrix[1][3])*fTarget*dwHalfHeight;

			float fBoxY_W = dwHalfHeight
				- (fOwnMatrix[1][0] * fPlayerPos[Pos_X]
					+ fOwnMatrix[1][1] * fPlayerPos[Pos_Y]
					+ fOwnMatrix[1][2] * (fPlayerPos[Pos_Z] - 5.0f)
					+ fOwnMatrix[1][3])*fTarget*dwHalfHeight;

			//区分阵营，也就是不对队友进行自瞄操作
			if ((nPlayerCamp == 2 && g_bAiMBotSleepers) || (nPlayerCamp == 3 && g_bAiMBotDefenders))
			{
				float fX = (pOwnPos[Pos_X] - fPlayerPos[Pos_X])*(pOwnPos[Pos_X] - fPlayerPos[Pos_X]);
				float fY = (pOwnPos[Pos_Y] - fPlayerPos[Pos_Y])*(pOwnPos[Pos_Y] - fPlayerPos[Pos_Y]);
				float fZ = (pOwnPos[Pos_Z] - fPlayerPos[Pos_Z])*(pOwnPos[Pos_Z] - fPlayerPos[Pos_Z]);
				float fVector = sqrt(fX + fY + fZ);
				if (fVector < stAiMBotInfo.fVector)
				{
					stAiMBotInfo.fVector = fVector;
					stAiMBotInfo.fPlayerPos[Pos_X] = fPlayerPos[Pos_X];
					stAiMBotInfo.fPlayerPos[Pos_Y] = fPlayerPos[Pos_Y];
					stAiMBotInfo.fPlayerPos[Pos_Z] = fPlayerPos[Pos_Z];
				}
			}

			if (nPlayerCamp == 2)
				nBoxColor = D3DCOLOR_XRGB(255, 0, 0);//潜伏者
			if (nPlayerCamp == 3)
				nBoxColor = D3DCOLOR_XRGB(0, 255, 0);//保卫者

			DrawBox(pDirect3DDevice, nBoxColor,
				static_cast<int>(fBoxX - ((fBoxY_W - fBoxY_H) / 4.0f)),static_cast<int>(fBoxY_H), 
				static_cast<int>((fBoxY_W - fBoxY_H) / 2.0f), static_cast<int>(fBoxY_W - fBoxY_H));
		}

		//对最近的人物进行自瞄
		if (GetAsyncKeyState(VK_SHIFT))
			AiMBot(pOwnPos, stAiMBotInfo.fPlayerPos);
	}
}

void HideModule(void* pModule)
{
	void* pPEB = nullptr;

	//读取PEB指针
	_asm
	{
		push eax
		mov eax, fs:[0x30]
		mov pPEB, eax
		pop eax
	}

	//操作得到保存全部模块的双向链表头指针
	void* pLDR = *((void**)((unsigned char*)pPEB + 0xc));
	void* pCurrent = *((void**)((unsigned char*)pLDR + 0x0c));
	void* pNext = pCurrent;

	//对链表进行遍历，对指定模块进行断链隐藏
	do
	{
		void* pNextPoint = *((void**)((unsigned char*)pNext));
		void* pLastPoint = *((void**)((unsigned char*)pNext + 0x4));
		void* nBaseAddress = *((void**)((unsigned char*)pNext + 0x18));

		if (nBaseAddress == pModule)
		{
			*((void**)((unsigned char*)pLastPoint)) = pNextPoint;
			*((void**)((unsigned char*)pNextPoint + 0x4)) = pLastPoint;
			pCurrent = pNextPoint;
		}

		pNext = *((void**)pNext);
	} while (pCurrent != pNext);
}

void AiMBot(float* pOwnPos,float* pEnemyPos)
{
	int nAngleAddress = 0;					//偏斜角和俯仰角地址
	int nAngleOffsetX = 0x4d88;				//偏斜角和俯仰角偏移

	float pAngle[Angle_Number];				//偏斜角和俯仰角
	static float PI = 3.14f;				//

	if (g_bAiMBotDefenders == false && g_bAiMBotSleepers == false)
		return;

	//得到我们坐标和敌人坐标的差值
	float fDifferX = pOwnPos[Pos_X] - pEnemyPos[Pos_X];
	float fDifferY = pOwnPos[Pos_Y] - pEnemyPos[Pos_Y];
	float fDifferZ = pOwnPos[Pos_Z] - pEnemyPos[Pos_Z];

	//先求偏斜角度X
	pAngle[Angle_X] = atan(fDifferY / fDifferX);
	if (fDifferX >= 0.0f && fDifferY >= 0.0f)
		pAngle[Angle_X] = pAngle[Angle_X] / PI * 180 - 180;
	else if (fDifferX < 0.0f && fDifferY >= 0.0f)
		pAngle[Angle_X] = pAngle[Angle_X] / PI * 180;
	else if (fDifferX < 0.0f && fDifferY < 0.0)
		pAngle[Angle_X] = pAngle[Angle_X] / PI * 180;
	else if (fDifferX >= 0.0f && fDifferY < 0.0f)
		pAngle[Angle_X] = pAngle[Angle_X] / PI * 180 + 180;

	//再求仰俯角Y
	pAngle[Angle_Y] = atan(fDifferZ / sqrt(fDifferX * fDifferX + fDifferY * fDifferY)) / PI * 180 + 0.5f;

	//将角度写入
	g_cGame.memory().Read(g_nAngleBaseAddress, sizeof(int), &nAngleAddress);
	g_cGame.memory().Write(nAngleAddress + nAngleOffsetX, sizeof(pAngle), pAngle);
}
