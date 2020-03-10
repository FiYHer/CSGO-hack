#include "CSCheat.h"

#include <process.h>
using namespace std;
#pragma warning(disable:4244)

game_data g_data;			//游戏全局数据
super_data g_super;		//超级作弊数据

BOOL WINAPI DllMain(_In_ void* _DllHandle, _In_ unsigned long _Reason, _In_opt_ void* _Reserved)
{
	if (_Reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls((HMODULE)_DllHandle);
		_beginthread(_beginthread_proc, 0, NULL);
	}
	return TRUE;
}

void __cdecl _beginthread_proc(void*)
{
	create_debug();

	if (!check_csgo())
	{
		show_err("非CSGO游戏进程");
		return;
	}

	if (!d3dhook::InitializeD3DClass(&g_data.d3d_hook))
	{
		show_err("D3D初始化失败");
		return;
	}

	if (!g_data.d3d_hook.InitializeAndModifyAddress(d3dhook::D3dClass::Class_IDirect3DDevice9,
		d3dhook::Direct3DDevice_Function::F_EndScene,
		reinterpret_cast<int>(my_endscene)))
	{
		show_err("inline hook EndScene 失败");
		return;
	}

	if (!g_data.d3d_hook.InitializeAndModifyAddress(d3dhook::D3dClass::Class_IDirect3DDevice9,
		d3dhook::Direct3DDevice_Function::F_Reset,
		reinterpret_cast<int>(my_reset)))
	{
		show_err("inline hook Reset 失败");
		return;
	}

}

bool check_csgo(const char* str /*= "Counter-Strike: Global Offensive"*/)
{
	g_data.game_hwnd = FindWindowA(NULL, str);
	GetWindowThreadProcessId(g_data.game_hwnd, &g_data.pid);
	return GetCurrentProcessId() == g_data.pid;
}

void create_debug()
{
#ifdef _DEBUG
	AllocConsole();
	SetConsoleTitleA("CSGO游戏调试台");
	freopen("con", "w", stdout);
	system("mode con cols=50 lines=50");
#endif 
}

void show_err(const char* str)
{
#ifdef _DEBUG
	cout << str << endl;
#else
	MessageBoxA(NULL, str, NULL, NULL);
#endif 
}

HRESULT _stdcall my_endscene(IDirect3DDevice9* pDirect3DDevice)
{
	static bool state = true;
	if (state)
	{
		state = false;
		g_data.d3d_hook.SetGameDirect3DDevicePoint(pDirect3DDevice);
		init_imgui();
		init_gamedatas();
		g_data.old_proc = (WNDPROC)SetWindowLongA(g_data.game_hwnd, GWL_WNDPROC, (LONG)my_proc);
	}
	g_data.d3d_hook.ReduceAddress(d3dhook::D3dClass::Class_IDirect3DDevice9, d3dhook::Direct3DDevice_Function::F_EndScene);

	draw_meun();
	hack_manager();
	repoter_players(g_super);

	HRESULT hRet = pDirect3DDevice->EndScene();
	g_data.d3d_hook.ModifyAddress(d3dhook::D3dClass::Class_IDirect3DDevice9, d3dhook::Direct3DDevice_Function::F_EndScene);
	return hRet;
}

HRESULT _stdcall my_reset(IDirect3DDevice9* pDirect3DDevice,
	D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	static bool state = true;
	if (state)
	{
		state = false;
		g_data.d3d_hook.SetGameDirect3DDevicePoint(pDirect3DDevice);
	}
	g_data.d3d_hook.ReduceAddress(d3dhook::D3dClass::Class_IDirect3DDevice9, d3dhook::Direct3DDevice_Function::F_Reset);
	
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hRet = pDirect3DDevice->Reset(pPresentationParameters);
	ImGui_ImplDX9_CreateDeviceObjects();

	g_data.d3d_hook.ModifyAddress(d3dhook::D3dClass::Class_IDirect3DDevice9, d3dhook::Direct3DDevice_Function::F_Reset);
	return hRet;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
HRESULT _stdcall my_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) return TRUE;

	if (uMsg == WM_KEYDOWN && wParam == VK_INSERT) g_data.show_meun = !g_data.show_meun;

	return CallWindowProcW(g_data.old_proc, hWnd, uMsg, wParam, lParam);
}

void init_imgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\msyh.ttc", 15.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
	ImGui::StyleColorsLight();

	ImGui_ImplWin32_Init(g_data.game_hwnd);
	ImGui_ImplDX9_Init(g_data.d3d_hook.GetGameDirect3DDevice());
}

void init_gamedatas()
{
	g_data.game_proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, g_data.pid);
	HMODULE client_panorama = GetModuleHandleA("client_panorama");
	HMODULE engine = GetModuleHandleA("engine");

#ifdef _DEBUG
	cout << "game process -> " << g_data.game_proc << endl;
	cout << "client_panorama address -> " << client_panorama << endl;
	cout << "engine address -> " << engine << endl;
#endif

	const int matrix_offset = 0x4D36454;
	const int self_offset = 0xD30B84;
	const int enemy_offset = 0x4D44A04;
	const int angle_offset = 0x589D9C;

	g_data.matrix_address = (int)client_panorama + matrix_offset;
	g_data.self_address = (int)client_panorama + self_offset;
	g_data.enemy_address = (int)client_panorama + enemy_offset;
	g_data.angle_address = (int)engine + angle_offset;
}

void draw_meun()
{
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (g_data.show_meun)
	{
		ImGui::Begin(u8"CSGO游戏辅助", &g_data.show_meun);
		ImGui::Text(u8"[Ins]		显示/关闭");
		ImGui::Separator();

		ImGui::Checkbox(u8"敌人方框", &g_data.show_enemy);
		ImGui::SameLine();
		ImGui::Checkbox(u8"队友方框", &g_data.show_friend);
		ImGui::Separator();

		ImGui::Checkbox(u8"开镜自瞄", &g_data.open_mirror);
		ImGui::SameLine();
		ImGui::Checkbox(u8"跳跃自瞄", &g_data.player_jump);
		ImGui::SameLine();
		ImGui::Checkbox(u8"静步自瞄", &g_data.quiet_step);
		ImGui::SameLine();
		ImGui::Checkbox(u8"下蹲自瞄", &g_data.player_squat);
		ImGui::Separator();

		ImGui::SliderInt(u8"开镜自瞄时间间隔", &g_data.mirror_ms, 5, 100);
		ImGui::Separator();

		ImGui::RadioButton(u8"位置模式", &g_data.mode_type, 0);
		ImGui::SameLine();
		ImGui::RadioButton(u8"骨骼模式", &g_data.mode_type, 1);
		ImGui::Separator();

		if (ImGui::RadioButton(u8"准星距离模式", &g_data.distance_type, 0)) g_data.aim_offset = 1.5f;
		ImGui::SameLine();
		if (ImGui::RadioButton(u8"人物距离模式", &g_data.distance_type, 1)) g_data.aim_offset = 0.0f;
		ImGui::Separator();

		ImGui::SliderFloat(u8"自瞄微调", &g_data.aim_offset, -30.f, 30.0f);
		ImGui::Separator();

		ImGui::Checkbox(u8"无视闪光", &g_data.ignore_flash);
		ImGui::SameLine();
		ImGui::Checkbox(u8"显示护甲值", &g_data.show_armor);
		ImGui::SameLine();
		ImGui::Checkbox(u8"显示金钱值", &g_data.show_money);
		ImGui::SameLine();
		ImGui::Checkbox(u8"显示血量值", &g_data.show_blood);
		ImGui::Separator();

		ImGui::RadioButton(u8"关闭举报", &g_super.report_mode, 0);
		ImGui::SameLine();
		ImGui::RadioButton(u8"全部举报", &g_super.report_mode, 1);
		ImGui::SameLine();
		ImGui::RadioButton(u8"针对举报", &g_super.report_mode, 2);
		ImGui::Separator();

		ImGui::Checkbox(u8"举报骂人", &g_super.report_curse);
		ImGui::SameLine();
		ImGui::Checkbox(u8"举报消极游戏", &g_super.report_grief);
		ImGui::SameLine();
		ImGui::Checkbox(u8"举报透视", &g_super.report_wallhack);
		ImGui::SameLine();
		ImGui::Checkbox(u8"举报自瞄", &g_super.report_aim);
		ImGui::SameLine();
		ImGui::Checkbox(u8"举报变速", &g_super.report_speed);

		if (g_super.report_mode == 2)
		{
			ImGui::Begin(u8"房间成员信息");
			static int id = 0;
			ImGui::InputInt(u8"UserID", &id);
			g_super.target_playerid = id;
			for (auto& it : g_super.inline_players) ImGui::BulletText(it.c_str());
			ImGui::End();
		}

		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void hack_manager()
{
	bool state = g_data.show_enemy || g_data.show_friend
		|| g_data.open_mirror || g_data.player_jump
		|| g_data.quiet_step || g_data.player_squat
		|| g_data.ignore_flash || g_data.show_armor
		|| g_data.show_money || g_data.show_blood;
	if (!state) return;

	D3DVIEWPORT9 view_port;
	if (FAILED(g_data.d3d_hook.GetGameDirect3DDevice()->GetViewport(&view_port))) return;
	g_data.game_width = view_port.Width;
	g_data.game_height = view_port.Height;
	int height = view_port.Height / 2;
	int width = view_port.Width / 2;

	//读取自身矩阵
	float matrix_data[4][4];
	SIZE_T read_size;
	ReadProcessMemory(g_data.game_proc, (LPCVOID)g_data.matrix_address, matrix_data, sizeof(matrix_data), &read_size);
	if (!read_size) return;

	//读取自身位置
	float self_data[3];
	int self_base = 0, self_offset = 0x35A8;
	ReadProcessMemory(g_data.game_proc, (LPCVOID)g_data.self_address, &self_base, sizeof(self_base), &read_size);
	if (!read_size) return;
	ReadProcessMemory(g_data.game_proc, (LPCVOID)(self_base + self_offset), self_data, sizeof(self_data), &read_size);
	if (!read_size) return;

	int player_base = 0, next_offset = 0x10;
	int blood_data = 0, blood_offset = 0x100;
	float pos_data[3]; int pos_offset = 0xA0;
	int camp_data = 0, camp_offset = 0xF4;
	float flash_data = 2.0f; int flash_offset = 0xA40C;
	int armor_data = 0, armor_offset = 0xB368;
	int meney_data = 0, meney_offset = 0xB358;
	bool mirror_data = false; int mirror_offset = 0x3914;
	int esp_base = 0, esp_offset = 0x26A8;
	float esp_data[3];
	float enemy_data[3]; int aim_min = INT_MAX;
	for (int i = 0; i <= 50; i++)
	{
		//读取人物基址
		ReadProcessMemory(g_data.game_proc, (LPCVOID)(g_data.enemy_address + i * next_offset), &player_base, sizeof(player_base), &read_size);
		if (!read_size) return;
		if (!player_base) continue;

		//读取人物血量
		ReadProcessMemory(g_data.game_proc, (LPCVOID)(player_base + blood_offset), &blood_data,sizeof(blood_data),&read_size);
		if (!read_size) return;
		if (!blood_data) continue;

		//读取人物位置
		ReadProcessMemory(g_data.game_proc, (LPCVOID)(player_base + pos_offset), pos_data, sizeof(pos_data), &read_size);
		if (!read_size) return;

		//读取人物阵营
		ReadProcessMemory(g_data.game_proc, (LPCVOID)(player_base + camp_offset), &camp_data, sizeof(camp_data), &read_size);
		if (!read_size) return;

		//读取人物护甲
		ReadProcessMemory(g_data.game_proc, (LPCVOID)(player_base + armor_offset), &armor_data, sizeof(armor_data), &read_size);
		if (!read_size) return;

		//读取人物金钱
		ReadProcessMemory(g_data.game_proc, (LPCVOID)(player_base + meney_offset), &meney_data, sizeof(meney_data), &read_size);
		if (!read_size) return;

		//判断是不是自己
		int absolute_x = abs(self_data[0] - pos_data[0]);
		int absolute_y = abs(self_data[1] - pos_data[1]);
		int absolute_z = abs(self_data[2] - pos_data[2]);
		if (absolute_x < 5 && absolute_y < 5 && absolute_z < 5)
		{
			if (g_data.ignore_flash)
			{
				WriteProcessMemory(g_data.game_proc, (LPVOID)(player_base + flash_offset), &flash_data, sizeof(flash_data), &read_size);
				if (!read_size) return;
			}
			if (g_data.open_mirror)
			{
				ReadProcessMemory(g_data.game_proc, (LPCVOID)(player_base + mirror_offset), &mirror_data, sizeof(mirror_data), &read_size);
				if (!read_size) return;
			}
			g_data.self_camp = camp_data;
			continue;
		}

		//人物朝向
		float to_target = matrix_data[2][0] * pos_data[0]
			+ matrix_data[2][1] * pos_data[1]
			+ matrix_data[2][2] * pos_data[2]
			+ matrix_data[2][3];

		//后面的人物不做处理加快速度
		if(to_target < 0.01f) continue;
		to_target = 1.0f / to_target;

		int to_width = width + (matrix_data[0][0] * pos_data[0]
			+ matrix_data[0][1] * pos_data[1]
			+ matrix_data[0][2] * pos_data[2]
			+ matrix_data[0][3]) * to_target * width;

		int to_height_h = height - (matrix_data[1][0] * pos_data[0]
			+ matrix_data[1][1] * pos_data[1]
			+ matrix_data[1][2] * (pos_data[2] + 75.0f)
			+ matrix_data[1][3]) * to_target * height;

		int to_height_w = height - (matrix_data[1][0] * pos_data[0]
			+ matrix_data[1][1] * pos_data[1]
			+ matrix_data[1][2] * (pos_data[2] - 5.0f)
			+ matrix_data[1][3]) * to_target * height;

		int color = D3DCOLOR_XRGB(255, 0, 0);
		if (g_data.self_camp == camp_data) color = D3DCOLOR_XRGB(0, 255, 0);

		int x = to_width - (to_height_w - to_height_h) / 4;
		int y = to_height_h;
		int w = (to_height_w - to_height_h) / 2;
		int h = to_height_w - to_height_h;

		//绘制方框
		if (!g_data.mode_type
			&& ((g_data.show_friend && g_data.self_camp == camp_data)
				|| (g_data.show_enemy  && g_data.self_camp != camp_data)))
			draw_box(x, y, w, h, color);

		//绘制骨骼方框
		if (g_data.mode_type
			&& ((g_data.show_friend && g_data.self_camp == camp_data)
				|| (g_data.show_enemy  && g_data.self_camp != camp_data)))
			draw_espbox(x, y, w, h, color);

		//绘制血量
		if (g_data.show_blood) draw_blood(x, y, h, blood_data);

		//绘制护甲
		if (g_data.show_armor) draw_armor(x + w, y, h, armor_data);

		//绘制金钱
		if (g_data.show_money) draw_meney(x, y, w, meney_data);

		//接下来进行进行自瞄,不对队友进行操作
		if (g_data.self_camp == camp_data) continue;

		//开启了任一自瞄
		if (g_data.open_mirror || g_data.player_jump
			|| g_data.quiet_step || g_data.player_squat)
		{
			if (g_data.mode_type)//骨骼模式
			{
				ReadProcessMemory(g_data.game_proc, (LPCVOID)(player_base + esp_offset), &esp_base, sizeof(esp_base), &read_size);
				if (!read_size) return;
				ReadProcessMemory(g_data.game_proc, (LPCVOID)(esp_base + 99 * sizeof(float)), &esp_data[0], sizeof(float), &read_size);
				if (!read_size) return;
				ReadProcessMemory(g_data.game_proc, (LPCVOID)(esp_base + 103 * sizeof(float)), &esp_data[1], sizeof(float), &read_size);
				if (!read_size) return;
				ReadProcessMemory(g_data.game_proc, (LPCVOID)(esp_base + 107 * sizeof(float)), &esp_data[2], sizeof(float), &read_size);
				if (!read_size) return;
			}

			float temp_pos[3];
			temp_pos[0] = pos_data[0];
			temp_pos[1] = pos_data[1];
			temp_pos[2] = pos_data[2];
			if (g_data.mode_type)//骨骼自瞄
			{
				temp_pos[0] = esp_data[0];
				temp_pos[1] = esp_data[1];
				temp_pos[2] = esp_data[2];
			}

			if (g_data.distance_type)//人物距离模式
			{
				int value = sqrt((self_data[0] - temp_pos[0]) * (self_data[0] - temp_pos[0])
					+ (self_data[1] - temp_pos[1]) * (self_data[1] - temp_pos[1])
					+ (self_data[2] - temp_pos[2]) * (self_data[2] - temp_pos[2]));
				if (value < aim_min)
				{
					aim_min = value;
					enemy_data[0] = temp_pos[0];
					enemy_data[1] = temp_pos[1];
					enemy_data[2] = temp_pos[2];
				}
			}
			else//准星距离模式
			{
				int value = abs(width - to_width) + abs(height - to_height_h);
				if (value < aim_min)
				{
					aim_min = value;
					enemy_data[0] = temp_pos[0];
					enemy_data[1] = temp_pos[1];
					enemy_data[2] = temp_pos[2];
				}
			}
		}
	}

	if (g_data.open_mirror)
		if (mirror_data)
		{
			static int per_times = g_data.mirror_ms;
			aim_bot(self_data, enemy_data);
			if (!--per_times)
			{
				per_times = g_data.mirror_ms;
				mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
				mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
			}
		}
		else g_data.start_aim = false;

	if (g_data.player_jump)
		if (GetAsyncKeyState(VK_SPACE) & 0x8000) aim_bot(self_data, enemy_data);
		else g_data.start_aim = false;

	if (g_data.quiet_step)
		if (GetAsyncKeyState(VK_SHIFT) & 0x8000) aim_bot(self_data, enemy_data);
		else g_data.start_aim = false;

	if (g_data.player_squat)
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000) aim_bot(self_data, enemy_data);
		else g_data.start_aim = false;
}

void draw_line(int x, int y, int w, int h, int color)
{
	D3DRECT rect{ x + 0 ,y + 0,x + w,y + h };
	g_data.d3d_hook.GetGameDirect3DDevice()->Clear(1, &rect, D3DCLEAR_TARGET, color, 0.0f, 0);
}

void draw_box(int x, int y, int w, int h, int color)
{
	draw_line(x, y + h, w, 2, color);		//下边
	draw_line(x, y, 2, h, color);				//左边
	draw_line(x, y, w, 2, color);				//上边
	draw_line(x + w, y, 2, h, color);		//右边
}

void draw_espbox(int x, int y, int w, int h, int color)
{
	draw_line(x, y, w, 2, color);				//上边
	draw_line(x, y + h, w, 2, color);		//下边
	draw_line(x, y, 2, h, color);				//左边
	draw_line(x + w, y, 2, h, color);		//右边
}

void draw_blood(int x, int y, int h, int blood)
{
	int value = (float)blood / 100.0f * (float)h;
	draw_line(x - 10, y, 3, value, D3DCOLOR_XRGB(0, 0, 255));
}

void draw_armor(int x, int y, int h, int armor)
{
	int value = (float)armor / 100.0f * (float)h;
	draw_line(x + 10, y, 3, value, D3DCOLOR_XRGB(0, 255, 255));
}

void draw_meney(int x, int y, int w, int meney)
{
	int value = (float)meney / 16000.0f * (float)w;
	draw_line(x, y - 10, value, 3, D3DCOLOR_XRGB(255, 0, 255));
}

void aim_bot(float* self_data, float* enemy_data)
{
	//刚开始自瞄
	if (!g_data.start_aim)
	{
		g_data.aim_data[0] = enemy_data[0];
		g_data.aim_data[1] = enemy_data[1];
		g_data.aim_data[2] = enemy_data[2];
		g_data.start_aim = true;
	}
	else//防止杀死一个敌人后立即自瞄下一个敌人
	{
		int value = 80;
		if (abs(g_data.aim_data[0] - enemy_data[0]) > value
			|| abs(g_data.aim_data[1] - enemy_data[1]) > value
			|| abs(g_data.aim_data[2] - enemy_data[2]) > value) return;
	}

	float x = self_data[0] - enemy_data[0];
	float y = self_data[1] - enemy_data[1];
	float z = self_data[2] - enemy_data[2];
	if (g_data.mode_type) z += 60.0f + g_data.aim_offset;	//骨骼模式

	float angle[2];
	const float pi = 3.1415f;

	//x
	angle[1] = atan(y / x);
	if (x >= 0.0f && y >= 0.0f) angle[1] = angle[1] / pi * 180.0f - 180.0f;
	else if (x < 0.0f && y >= 0.0f) angle[1] = angle[1] / pi * 180.0f;
	else if (x < 0.0f && y < 0.0f) angle[1] = angle[1] / pi * 180.0f;
	else if (x >= 0.0f && y < 0.0f) angle[1] = angle[1] / pi * 180.f + 180.0f;

	//y
	angle[0] = atan(z / sqrt(x * x + y * y)) / pi * 180.f;
	if (!g_data.mode_type) angle[0] += g_data.aim_offset;

	int angle_base = 0, angle_offset = 0x4D88;
	SIZE_T read_size;
	ReadProcessMemory(g_data.game_proc, (LPCVOID)g_data.angle_address, &angle_base, sizeof(angle_base), &read_size);
	if (!read_size) return;
	WriteProcessMemory(g_data.game_proc, (LPVOID)(angle_base + angle_offset), angle, sizeof(angle), &read_size);
	if (!read_size) return;

}
