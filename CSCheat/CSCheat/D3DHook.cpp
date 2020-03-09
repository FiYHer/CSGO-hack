#include "D3DHook.h"

LRESULT CALLBACK d3dhook::D3DHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

bool d3dhook::InitializeD3DClass(D3DHook* pThis)
{
	auto RunError = [](const char* szNote) -> void
	{
		throw std::runtime_error(szNote);
	};

	try
	{
		CHAR szWindowClass[MAX_PATH];
		wsprintfA(szWindowClass, "D3DWindow%d", (int)time(NULL));
		WNDCLASSEXA stWndCls;
		memset(&stWndCls, 0, sizeof(WNDCLASSEXA));
		stWndCls.cbClsExtra = 0;
		stWndCls.cbWndExtra = 0;
		stWndCls.cbSize = sizeof(WNDCLASSEXA);
		stWndCls.hbrBackground = (HBRUSH)COLOR_HIGHLIGHT;
		stWndCls.hCursor = NULL;
		stWndCls.hIcon = NULL;
		stWndCls.hIconSm = NULL;
		stWndCls.hInstance = GetModuleHandleA(NULL);
		stWndCls.lpfnWndProc = D3DHookProc;
		stWndCls.lpszClassName = szWindowClass;
		stWndCls.lpszMenuName = NULL;
		stWndCls.style = CS_HREDRAW | CS_VREDRAW;
		if (RegisterClassExA(&stWndCls) == NULL) RunError("RegisterClassExA fail");

		HWND hWnd = CreateWindowExA(NULL, szWindowClass, NULL, WS_OVERLAPPEDWINDOW, 100, 100, 100, 100, NULL, NULL, stWndCls.hInstance, NULL);
		if (hWnd == NULL) RunError("CreateWindowExA fail");

		IDirect3D9* pDirect3D = Direct3DCreate9(D3D_SDK_VERSION);
		if (pDirect3D == NULL) RunError("Direct3DCreate9 fail");

		IDirect3DDevice9* pDirect3DDevice;
		D3DPRESENT_PARAMETERS stPresent;
		memset(&stPresent, 0,sizeof(D3DPRESENT_PARAMETERS));
		stPresent.Windowed = TRUE;
		stPresent.SwapEffect = D3DSWAPEFFECT_DISCARD;
		stPresent.BackBufferFormat = D3DFMT_UNKNOWN;
		stPresent.EnableAutoDepthStencil = TRUE;
		stPresent.AutoDepthStencilFormat = D3DFMT_D16;
		if (FAILED(pDirect3D->CreateDevice(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			hWnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			&stPresent,
			&pDirect3DDevice)))
			RunError("CreateDevice fail");

		pThis->SetOwnDirect3DPoint(pDirect3D);
		pThis->SetOwnDirect3DDevicePoint(pDirect3DDevice);
		pThis->SetInitialize(true);
	}
	catch (const std::exception& e)
	{
		std::cout << GetLastError() <<e.what() << std::endl;
		return false;
	}
	return true;
}

void d3dhook::D3DHook::SetOwnDirect3DPoint(IDirect3D9* pDirect3D)
{
	m_pDirect3DTable = (int*)*(int*)pDirect3D;
	m_pOwnDirect3D = pDirect3D;
}

void d3dhook::D3DHook::SetOwnDirect3DDevicePoint(IDirect3DDevice9* pDirect3DDevice)
{
	m_pDirect3DDeviceTable = (int*)*(int*)pDirect3DDevice;
	m_pOwnDirect3DDevice = pDirect3DDevice;
}

void d3dhook::D3DHook::SetGameDirect3DPoint(IDirect3D9* pDirect3D)
{
	if (pDirect3D != m_pOwnDirect3D && m_pGameDirect3D == nullptr)
		m_pGameDirect3D = pDirect3D;
}

void d3dhook::D3DHook::SetGameDirect3DDevicePoint(IDirect3DDevice9* pDirect3DDevice)
{
	if (pDirect3DDevice != m_pOwnDirect3DDevice && m_pGameDirect3DDevice == nullptr)
		m_pGameDirect3DDevice = pDirect3DDevice;
}

d3dhook::D3DHook::D3DHook():m_bInitialize(false)
{
	m_pOwnDirect3D = nullptr;
	m_pOwnDirect3DDevice = nullptr;

	m_pDirect3DTable = nullptr;
	m_pDirect3DDeviceTable = nullptr;

	m_pGameDirect3D = nullptr;
	m_pGameDirect3DDevice = nullptr;
}

d3dhook::D3DHook::~D3DHook()
{
	ReduceAllAddress();

	if (m_pOwnDirect3D) m_pOwnDirect3D->Release();
	if (m_pOwnDirect3DDevice) m_pOwnDirect3DDevice->Release();

	m_pOwnDirect3D = nullptr;
	m_pOwnDirect3DDevice = nullptr;
	m_pDirect3DTable = nullptr;
	m_pDirect3DDeviceTable = nullptr;
}

bool d3dhook::D3DHook::InitializeAndModifyAddress(D3dClass eClass, int nFunctionIndex, int nNewAddress)
{
	if (m_bInitialize == false) return false;

	if (eClass == D3dClass::Class_IDirect3D9)
	{
		int nOldAddress = m_pDirect3DTable[nFunctionIndex];	//获取指定索引的函数地址
		m_cDirect3DAddress[static_cast<Direct3D_Function>(nFunctionIndex)];
		if (m_cDirect3DAddress[static_cast<Direct3D_Function>(nFunctionIndex)].Initialize(nOldAddress, nNewAddress) == false)
			return false;
		if (m_cDirect3DAddress[static_cast<Direct3D_Function>(nFunctionIndex)].ModifyAddress() == false)
			return false;
		return true;
	}

	if (eClass == D3dClass::Class_IDirect3DDevice9)
	{
		int nOldAddress = m_pDirect3DDeviceTable[nFunctionIndex];			//获取指定索引的函数地址
		m_cDirect3DDeviceAddress[static_cast<Direct3DDevice_Function>(nFunctionIndex)];
		if (m_cDirect3DDeviceAddress[static_cast<Direct3DDevice_Function>(nFunctionIndex)].Initialize(nOldAddress, nNewAddress) == false)
			return false;
		if (m_cDirect3DDeviceAddress[static_cast<Direct3DDevice_Function>(nFunctionIndex)].ModifyAddress() == false)
			return false;
		return true;
	}

	return false;
}

bool d3dhook::D3DHook::ModifyAddress(D3dClass eClass, int nFunctionIndex)
{
	if (m_bInitialize == false) return false;
	
	if (eClass == D3dClass::Class_IDirect3D9)
		return m_cDirect3DAddress[static_cast<Direct3D_Function>(nFunctionIndex)].ModifyAddress();

	if (eClass == D3dClass::Class_IDirect3DDevice9)
		return m_cDirect3DDeviceAddress[static_cast<Direct3DDevice_Function>(nFunctionIndex)].ModifyAddress();

	return false;
}

bool d3dhook::D3DHook::ReduceAddress(D3dClass eClass, int nFunctionIndex)
{
	if (m_bInitialize == false) return false;

	if (eClass == D3dClass::Class_IDirect3D9)
		return m_cDirect3DAddress[static_cast<Direct3D_Function>(nFunctionIndex)].ReduceAddress();

	if (eClass == D3dClass::Class_IDirect3DDevice9)
		return m_cDirect3DDeviceAddress[static_cast<Direct3DDevice_Function>(nFunctionIndex)].ReduceAddress();

	return false;
}

bool d3dhook::D3DHook::ReduceAllAddress()
{
	if (m_bInitialize == false) return false;

	for (std::map<Direct3D_Function, InlineHook>::iterator it = m_cDirect3DAddress.begin();
		it != m_cDirect3DAddress.end(); it++)
	{
		it->second.ReduceAddress();
	}

	for (std::map<Direct3DDevice_Function, InlineHook>::iterator it = m_cDirect3DDeviceAddress.begin();
		it != m_cDirect3DDeviceAddress.end(); it++)
	{
		it->second.ReduceAddress();
	}

	m_cDirect3DAddress.clear();
	m_cDirect3DDeviceAddress.clear();
	return true;
}
