#pragma once
#ifndef _CRT_SECURE_NO_WARNINGS
#define  _CRT_SECURE_NO_WARNINGS
#endif //_CRT_SECURE_NO_WARNINGS

#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_dx9.h"
#include "Imgui/imgui_impl_win32.h"

#include "D3DHook.h"
#include "SuperHack.h"

//游戏数据保存结构
typedef struct game_data
{
	d3dhook::D3DHook d3d_hook;			//hook类
	DWORD pid;										//游戏进程ID
	HANDLE game_proc;							//游戏进程句柄
	HWND game_hwnd;							//游戏窗口句柄
	WNDPROC old_proc;							//游戏窗口原始过程
	bool show_meun;								//显示游戏菜单
	int self_camp;										//自己阵营
	int game_width;									//游戏窗口宽度
	int game_height;									//游戏窗口高度

	int matrix_address;								//矩阵地址
	int self_address;									//自身地址
	int enemy_address;								//敌人地址
	int angle_address;								//角度地址

	bool show_enemy;								//显示敌人
	bool show_friend;								//显示队友

	bool open_mirror;								//开镜自瞄
	bool player_jump;								//跳跃自瞄
	bool quiet_step;									//静步自瞄
	bool player_squat;								//下蹲自瞄

	int mirror_ms;										//开镜自瞄开枪间隔
	int mode_type;									//模式类型
	int distance_type;								//距离类型

	float aim_offset;									//自瞄微调

	bool ignore_flash;								//忽略闪光
	bool show_armor;								//显示护甲值
	bool show_money;								//显示金钱值
	bool show_blood;								//显示血量值

	float aim_data[3];								//自瞄位置
	bool start_aim;										//自瞄标识，防止杀死敌人后立马自瞄下一个

	game_data()
	{
		show_meun = true;
		aim_offset = 1.5f;
		mirror_ms = 25;
	}
}game_data;

//初始化线程函数
void __cdecl  _beginthread_proc(void*);

//检查是否游戏进程
bool check_csgo(const char* str = "Counter-Strike: Global Offensive");

//创建调试控制台
void create_debug();

//错误输出接口
void show_err(const char* str);

//D3D9新EndScene
HRESULT _stdcall my_endscene(IDirect3DDevice9* pDirect3DDevice);

//D3D9新Reset
HRESULT _stdcall my_reset(IDirect3DDevice9* pDirect3DDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);

//游戏窗口新过程函数
HRESULT _stdcall my_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//初始化Imgui界面
void init_imgui();

//初始化游戏数据
void init_gamedatas();

//绘制菜单
void draw_meun();

//作弊管理
void hack_manager();

//绘制直线
void draw_line(int x, int y, int w, int h, int color);

//绘制方框
void draw_box(int x, int y, int w, int h, int color);

//绘制骨骼方框
void draw_espbox(int x, int y, int w, int h, int color);

//绘制血量
void draw_blood(int x, int y, int h, int blood);

//绘制护甲
void draw_armor(int x, int y, int h, int armor);

//绘制金钱
void draw_meney(int x, int y, int w, int meney);

//开始自瞄
void aim_bot(float* self_data, float* enemy_data);



