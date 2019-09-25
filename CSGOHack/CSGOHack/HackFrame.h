#pragma once

#include "BlackBone/BlackBone/Process/Process.h"
#include "BlackBone/BlackBone/Patterns/PatternSearch.h"
#include "BlackBone/BlackBone/Process/RPC/RemoteFunction.hpp"
#include "BlackBone/BlackBone/Syscalls/Syscall.h"
#if _DEBUG
#pragma comment(lib,"Blackbone/BlackboneDebug.lib")
#else
#pragma comment(lib,"Blackbone/BlackboneRelease.lib")
#endif

#include <iostream>
#include <list>

#include <Windows.h>
#include <process.h>

#include <d3d9.h>
#pragma comment(lib,"d3d9.lib")

#include "Detours/detours.h"
#if _DEBUG
#pragma comment(lib,"Detours/DetoursDebug.lib")
#else
#pragma comment(lib,"Detours/DetoursRelease.lib")
#endif

#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_dx9.h"
#include "Imgui/imgui_impl_win32.h"

#define Load_Fonts 1

#define Coordinate_Number 3
#define Pos_X 0
#define Pos_Y 1
#define Pos_Z 2

#define Angle_Number 2 
#define Angle_X 1
#define Angle_Y 0

typedef struct _AiMBotInfo
{
	float fVector;
	float fPlayerPos[Coordinate_Number];
	_AiMBotInfo()
	{
		fVector = 10000.0f;
		fPlayerPos[Pos_X] = fPlayerPos[Pos_Y] = fPlayerPos[Pos_Z] = 0.0f;
	}
}AiMBotInfo;

enum Direct3DDevice_Function		//IDirect3DDevice9类的函数索引
{
	F_TestCooperativeLevel = 3,
	F_GetAvailableTextureMem,
	F_EvictManagedResources,
	F_GetDirect3D,
	F_GetDeviceCaps,
	F_GetDisplayMode,
	F_GetCreationParameters,
	F_SetCursorProperties,
	F_SetCursorPosition,
	F_ShowCursor,
	F_CreateAdditionalSwapChain,
	F_GetSwapChain,
	F_GetNumberOfSwapChains,
	F_Reset,
	F_Present,
	F_GetBackBuffer,
	F_GetRasterStatus,
	F_SetDialogBoxMode,
	F_SetGammaRamp,
	F_GetGammaRamp,
	F_CreateTexture,
	F_CreateVolumeTexture,
	F_CreateCubeTexture,
	F_CreateVertexBuffer,
	F_CreateIndexBuffer,
	F_CreateRenderTarget,
	F_CreateDepthStencilSurface,
	F_UpdateSurface,
	F_UpdateTexture,
	F_GetRenderTargetData,
	F_GetFrontBufferData,
	F_StretchRect,
	F_ColorFill,
	F_CreateOffscreenPlainSurface,
	F_SetRenderTarget,
	F_GetRenderTarget,
	F_SetDepthStencilSurface,
	F_GetDepthStencilSurface,
	F_BeginScene,
	F_EndScene,
	F_Clear,
	F_SetTransform,
	F_GetTransform,
	F_MultiplyTransform,
	F_SetViewport,
	F_GetViewport,
	F_SetMaterial,
	F_GetMaterial,
	F_SetLight,
	F_GetLight,
	F_LightEnable,
	F_GetLightEnable,
	F_SetClipPlane,
	F_GetClipPlane,
	F_SetRenderState,
	F_GetRenderState,
	F_CreateStateBlock,
	F_BeginStateBlock,
	F_EndStateBlock,
	F_SetClipStatus,
	F_GetClipStatus,
	F_GetTexture,
	F_SetTexture,
	F_GetTextureStageState,
	F_SetTextureStageState,
	F_GetSamplerState,
	F_SetSamplerState,
	F_ValidateDevice,
	F_SetPaletteEntries,
	F_GetPaletteEntries,
	F_SetCurrentTexturePalette,
	F_GetCurrentTexturePalette,
	F_SetScissorRect,
	F_GetScissorRect,
	F_SetSoftwareVertexProcessing,
	F_GetSoftwareVertexProcessing,
	F_SetNPatchMode,
	F_GetNPatchMode,
	F_DrawPrimitive,
	F_DrawIndexedPrimitive,
	F_DrawPrimitiveUP,
	F_DrawIndexedPrimitiveUP,
	F_ProcessVertices,
	F_CreateVertexDeclaration,
	F_SetVertexDeclaration,
	F_GetVertexDeclaration,
	F_SetFVF,
	F_GetFVF,
	F_CreateVertexShader,
	F_SetVertexShader,
	F_GetVertexShader,
	F_SetVertexShaderConstantF,
	F_GetVertexShaderConstantF,
	F_SetVertexShaderConstantI,
	F_GetVertexShaderConstantI,
	F_SetVertexShaderConstantB,
	F_GetVertexShaderConstantB,
	F_SetStreamSource,
	F_GetStreamSource,
	F_SetStreamSourceFreq,
	F_GetStreamSourceFreq,
	F_SetIndices,
	F_GetIndices,
	F_CreatePixelShader,
	F_SetPixelShader,
	F_GetPixelShader,
	F_SetPixelShaderConstantF,
	F_GetPixelShaderConstantF,
	F_SetPixelShaderConstantI,
	F_GetPixelShaderConstantI,
	F_SetPixelShaderConstantB,
	F_GetPixelShaderConstantB,
	F_DrawRectPatch,
	F_DrawTriPatch,
	F_DeletePatch,
	F_CreateQuery,
};

//创建一个调试控制台
void CreateDebugConsole();

//初始化D3D9的Hook
bool InitializeD3DHook(_In_ HWND hGameWindow);

//释放掉资源
void ReleaseAll();

//D3D9函数类型
typedef HRESULT(_stdcall *Type_Reset)(IDirect3DDevice9* pDirect3DDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
typedef HRESULT(_stdcall *Type_Present)(IDirect3DDevice9* pDirect3DDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
typedef HRESULT(_stdcall *Type_EndScene)(IDirect3DDevice9* pDirect3DDevice);
typedef HRESULT(_stdcall *Type_DrawIndexedPrimitive)(IDirect3DDevice9* pDirect3DDevice, D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount);

//我们的D3D9替换函数
HRESULT _stdcall MyReset(IDirect3DDevice9* pDirect3DDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
HRESULT _stdcall MyPresent(IDirect3DDevice9* pDirect3DDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
HRESULT _stdcall MyEndScene(IDirect3DDevice9* pDirect3DDevice);
HRESULT _stdcall MyDrawIndexedPrimitive(IDirect3DDevice9* pDirect3DDevice, D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount);

//初始化Imgui
bool InitializeImgui();

//绘制Imgui菜单
void DrawImgui();

//我们的窗口过程
LRESULT CALLBACK MyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//初始化游戏人物Stride
void InitializeStride();

//初始化数据基址
void InitializeData();

//绘制方框
void DrawBox(IDirect3DDevice9* pDirect3DDevice, D3DCOLOR dwColor, int x, int y, int w, int h, int nBlood = 0);

//绘制人物方框
void DrawPlayerBox(IDirect3DDevice9* pDirect3DDevice);

//断链操作
void HideModule(void* pModule);

//游戏人物自瞄
void AiMBot(float* pOwnPos,float* pEnemyPos);