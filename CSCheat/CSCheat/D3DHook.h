#pragma once
#include "InlineHook.h"
#include <map>

#include <time.h>
#include <d3d9.h>
#pragma comment(lib,"d3d9.lib")

namespace d3dhook
{
	enum D3dClass
	{
		Class_IDirect3D9,					//表示IDirect3D9类
		Class_IDirect3DDevice9		//表示IDirect3DDevice9类
	};

	enum Direct3D_Function				//IDirect3D9类的函数索引
	{
		f_RegisterSoftwareDevice = 3,
		f_GetAdapterCount,
		f_GetAdapterIdentifier,
		f_GetAdapterModeCount,
		f_EnumAdapterModes,
		f_GetAdapterDisplayMode,
		f_CheckDeviceType,
		f_CheckDeviceFormat,
		f_CheckDeviceMultiSampleType,
		f_CheckDepthStencilMatch,
		f_CheckDeviceFormatConversion,
		f_GetDeviceCaps,
		f_GetAdapterMonitor,
		f_CreateDevice
	};

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

	class D3DHook
	{
	private:
		IDirect3D9* m_pOwnDirect3D;												//我们创建的IDirect3D9
		IDirect3DDevice9* m_pOwnDirect3DDevice;							//我们创建的IDirect3DDevice9

		int* m_pDirect3DTable;														//IDirect3D9类的函数查找指针
		int* m_pDirect3DDeviceTable;											//IDirect3DDevice9类的函数查找指针

		IDirect3D9* m_pGameDirect3D;											//原本游戏里面的IDirect3D9
		IDirect3DDevice9* m_pGameDirect3DDevice;					//原本游戏里面的IDirect3DDevice9

		std::map<Direct3D_Function, InlineHook> m_cDirect3DAddress;							//保存Hook住的IDirect3D9函数
		std::map<Direct3DDevice_Function, InlineHook> m_cDirect3DDeviceAddress;	//保存Hook住的IDirect3DDevice9函数

		bool m_bInitialize;																//表示当前实例是否初始化完成

	public:
		void SetOwnDirect3DPoint(IDirect3D9* pDirect3D);			//设置我们的IDirect3D9指针
		void SetOwnDirect3DDevicePoint(IDirect3DDevice9* pDirect3DDevice);		//设置我们的IDirect3DDevice9指针
		inline void SetInitialize(bool bState) { m_bInitialize = bState; }		//设置初始化状态

	public:
		void SetGameDirect3DPoint(IDirect3D9* pDirect3D);		//设置游戏里面的IDirect3D9指针
		void SetGameDirect3DDevicePoint(IDirect3DDevice9* pDirect3DDevice);		//设置游戏里面的IDirect3DDevice9指针

	public:
		inline IDirect3D9* GetGameDirect3D() const { return m_pGameDirect3D; }		//获取游戏里面的IDirect3D9指针
		inline IDirect3DDevice9* GetGameDirect3DDevice() const { return m_pGameDirect3DDevice; }		//获取游戏里面的IDirect3DDevice9指针

	public:
		D3DHook();
		~D3DHook();

		///Note 初始化并且对函数进行Hook操作
		///eClass 指定要Hook哪里的D3D函数地址
		///nFunctionIndex 指定要Hook哪一个D3D函数
		///nNewAddress 我们自己的新函数地址
		///执行成功返回true，否则返回false
		bool InitializeAndModifyAddress(D3dClass eClass, int nFunctionIndex,int nNewAddress);

		///Note 对指定函数进行Hook操作
		///eClass 指定要Hook哪里的D3D函数地址
		///nFunctionIndex 指定要Hook哪一个D3D函数
		///执行成功返回true，否则返回false
		bool ModifyAddress(D3dClass eClass,int nFunctionIndex);

		///Note 还原D3D函数的地址
		///eClass 指定要还原哪里的D3D函数地址
		///nFunctionIndex 指定要还原哪一个D3D函数
		///执行成功返回true，否则返回false
		bool ReduceAddress(D3dClass eClass, int nFunctionIndex);

		///Note 还原所以的D3D函数的地址
		///执行成功返回true，否则返回false
		bool ReduceAllAddress();
	};

	///Note 类辅助函数，默认窗口过程
	LRESULT CALLBACK D3DHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	///Note 类辅助函数，初始化D3DHook类
	bool InitializeD3DClass(D3DHook* pThis);
}

