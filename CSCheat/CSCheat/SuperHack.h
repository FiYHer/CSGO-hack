#pragma once
#include <Windows.h>
#include <process.h>
#include <Psapi.h>
#include <sstream>
#include <type_traits>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iterator>
#include <algorithm>
using namespace std;

//内存模式查找辅助宏
//__VA_ARGS__ 是一个可变参数的宏，很少人知道这个宏，这个可变参数的宏是新的C99规范中新增的，目前似乎只有gcc支持
#define FIND_PATTERN(type, ...) \
reinterpret_cast<type>(findPattern(__VA_ARGS__))

//相对地址转化为绝对地址
template <typename T>
static constexpr auto relativeToAbsolute(int* address) noexcept
{
	return reinterpret_cast<T>(reinterpret_cast<char*>(address + 1) + *address);
}

//内存模式查找
std::uintptr_t findPattern(const wchar_t* module, const char* pattern, size_t offset = 0)
{
	static auto id = 0;
	++id;

	if (MODULEINFO moduleInfo; 
		GetModuleInformation(GetCurrentProcess(), GetModuleHandleW(module), &moduleInfo, sizeof(moduleInfo))) 
	{
		auto start = static_cast<const char*>(moduleInfo.lpBaseOfDll);
		const auto end = start + moduleInfo.SizeOfImage;

		auto first = start;
		auto second = pattern;

		while (first < end && *second) {
			if (*first == *second || *second == '?') {
				++first;
				++second;
			}
			else {
				first = ++start;
				second = pattern;
			}
		}

		if (!*second) return reinterpret_cast<std::uintptr_t>(const_cast<char*>(start) + offset);
	}
	MessageBoxA(NULL, ("Failed to find pattern #" + std::to_string(id) + '!').c_str(), nullptr, MB_OK | MB_ICONWARNING);
	std::exit(EXIT_FAILURE);
	return 0;
}

//调用虚函数
template<typename T, typename ...Args>
constexpr auto callVirtualMethod(void* classBase, int index, Args... args) noexcept
{
	return ((*reinterpret_cast<T(__thiscall***)(void*, Args...)>(classBase))[index])(classBase, args...);
}

//获取指定类指针
template <typename T>
static auto find(const wchar_t* module, const char* name) noexcept
{
	if (HMODULE moduleHandle = GetModuleHandleW(module))
		if (const auto createInterface = reinterpret_cast<std::add_pointer_t<T* (const char* name, int* returnCode)>>(GetProcAddress(moduleHandle, "CreateInterface")))
			if (T* foundInterface = createInterface(name, nullptr))
				return foundInterface;

	MessageBoxA(nullptr, (std::ostringstream{ } << "Failed to find " << name << " interface!").str().c_str(), "Osiris", MB_OK | MB_ICONERROR);
	std::exit(EXIT_FAILURE);
}

//玩家信息
struct PlayerInfo
{
	std::uint64_t version;
	union
	{
		std::uint64_t xuid;//玩家SteamID
		struct
		{
			std::uint32_t xuidLow;
			std::uint32_t xuidHigh;
		};
	};
	char name[128];//玩家名称
	int userId;//玩家ID
	char guid[33];
	std::uint32_t friendsId;
	char friendsName[128];
	bool fakeplayer;//是否是电脑玩家
	bool hltv;
	int customfiles[4];
	unsigned char filesdownloaded;
	int entityIndex;
};

//矩阵辅助类
class matrix3x4 
{
private:
	float mat[3][4];
public:
	constexpr auto operator[](int i) const noexcept { return mat[i]; }
};

//向量辅助结构
struct Vector 
{
	constexpr float operator[](int i)
	{
		switch (i)
		{
		case 0: return x;
		case 1: return y;
		case 2: return z;
		}
		return 0.0f;
	}

	constexpr operator bool() const noexcept
	{
		return x || y || z;
	}

	constexpr Vector& operator=(float array[3]) noexcept
	{
		x = array[0];
		y = array[1];
		z = array[2];
		return *this;
	}

	constexpr Vector& operator+=(const Vector& v) noexcept
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	constexpr Vector& operator-=(const Vector& v) noexcept
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	constexpr auto operator-(const Vector& v) const noexcept
	{
		return Vector{ x - v.x, y - v.y, z - v.z };
	}

	constexpr auto operator+(const Vector& v) const noexcept
	{
		return Vector{ x + v.x, y + v.y, z + v.z };
	}

	constexpr Vector& operator/=(float div) noexcept
	{
		x /= div;
		y /= div;
		z /= div;
		return *this;
	}

	constexpr auto operator*(float mul) const noexcept
	{
		return Vector{ x * mul, y * mul, z * mul };
	}

	constexpr void normalize() noexcept
	{
		x = std::isfinite(x) ? std::remainder(x, 360.0f) : 0.0f;
		y = std::isfinite(y) ? std::remainder(y, 360.0f) : 0.0f;
		z = 0.0f;
	}

	auto length() const noexcept
	{
		return std::sqrt(x * x + y * y + z * z);
	}

	auto length2D() const noexcept
	{
		return std::sqrt(x * x + y * y);
	}

	constexpr auto squareLength() const noexcept
	{
		return x * x + y * y + z * z;
	}

	constexpr auto dotProduct(const Vector& v) const noexcept
	{
		return x * v.x + y * v.y + z * v.z;
	}

	constexpr auto transform(const matrix3x4& mat) const noexcept
	{
		return Vector{ dotProduct({ mat[0][0], mat[0][1], mat[0][2] }) + mat[0][3],
					   dotProduct({ mat[1][0], mat[1][1], mat[1][2] }) + mat[1][3],
					   dotProduct({ mat[2][0], mat[2][1], mat[2][2] }) + mat[2][3] };
	}

	float x, y, z;
};

//用户命令行结构
struct UserCmd 
{
	enum 
	{
		IN_ATTACK = 1 << 0,//攻击
		IN_JUMP = 1 << 1,//跳跃
		IN_DUCK = 1 << 2,//闪避
		IN_FORWARD = 1 << 3,//前进
		IN_BACK = 1 << 4,//后退
		IN_USE = 1 << 5,
		IN_MOVELEFT = 1 << 9,//左移
		IN_MOVERIGHT = 1 << 10,//右移
		IN_ATTACK2 = 1 << 11,
		IN_SCORE = 1 << 16,
		IN_BULLRUSH = 1 << 22//狂奔
	};
	int pad;
	int commandNumber;
	int tickCount;
	Vector viewangles;//视图视角
	Vector aimdirection;//自瞄方向
	float forwardmove;
	float sidemove;
	float upmove;
	int buttons;
	char impulse;
	int weaponselect;
	int weaponsubtype;
	int randomSeed;
	short mousedx;
	short mousedy;
	bool hasbeenpredicted;
};

//全局变量结构
struct GlobalVars 
{
	const float realtime;//实时时间
	const int framecount;//帧数量
	const float absoluteFrameTime;
	const std::byte pad[4];
	float currenttime;//当前时间
	float frametime;//帧时间
	const int maxClients;//客户端数量
	const int tickCount;
	const float intervalPerTick;
};

//引擎类
class Engine
{
public:
	//根据索引获取玩家信息
	constexpr auto getPlayerInfo(int entityIndex, const PlayerInfo& playerInfo) noexcept
	{
		return callVirtualMethod<bool, int, const PlayerInfo&>(this, 8, entityIndex, playerInfo);
	}

	//根据玩家ID获取玩家索引
	constexpr auto getPlayerForUserID(int userId) noexcept
	{
		return callVirtualMethod<int, int>(this, 9, userId);
	}

	//获取本地玩家索引
	constexpr auto getLocalPlayer() noexcept
	{
		return callVirtualMethod<int>(this, 12);
	}

	//获取视图角度
	constexpr auto getViewAngles(Vector& angles) noexcept
	{
		callVirtualMethod<void, Vector&>(this, 18, angles);
	}

	//设置视图角度
	constexpr auto setViewAngles(const Vector& angles) noexcept
	{
		callVirtualMethod<void, const Vector&>(this, 19, angles);
	}

	//获取当前玩家的数量
	constexpr auto getMaxClients() noexcept
	{
		return callVirtualMethod<int>(this, 20);
	}

	//判断是否在对局中
	constexpr auto isInGame() noexcept
	{
		return callVirtualMethod<bool>(this, 26);
	}

	//判断游戏是否连接
	constexpr auto isConnected() noexcept
	{
		return callVirtualMethod<bool>(this, 27);
	}

	using Matrix = float[4][4];

	//获取自身矩阵
	constexpr auto worldToScreenMatrix() noexcept
	{
		return callVirtualMethod<const Matrix&>(this, 37);
	}

	constexpr auto getBSPTreeQuery() noexcept
	{
		return callVirtualMethod<void*>(this, 43);
	}

	//获取等级名称
	constexpr auto getLevelName() noexcept
	{
		return callVirtualMethod<const char*>(this, 53);
	}

	constexpr auto clientCmdUnrestricted(const char* cmd) noexcept
	{
		callVirtualMethod<void, const char*, bool>(this, 114, cmd, false);
	}
};

//客户端类
class Client 
{
public:
	constexpr auto getAllClasses() noexcept
	{
		//return callVirtualMethod<ClientClass*>(this, 8);
		return;
	}

	constexpr bool dispatchUserMessage(int messageType, int arg, int arg1, void* data) noexcept
	{
		return callVirtualMethod<bool, int, int, int, void*>(this, 38, messageType, arg, arg1, data);
	}
};

//超级作弊数据
typedef struct super_data
{
	//引擎类
	Engine* engine;

	//客户端类
	Client* client;

	//全局变量结构
	GlobalVars* globalVars;

	//举报函数
	std::add_pointer_t<bool __stdcall(const char*, const char*)> submitReport;

	//设置氏族标记函数
	std::add_pointer_t<void __fastcall(const char*, const char*)> setClanTag;

	//举报模式
	int report_mode;

	//举报时间间隔
	int report_time;

	//举报骂人
	bool report_curse;

	//举报消极游戏
	bool report_grief;

	//举报透视
	bool report_wallhack;

	//举报自瞄
	bool report_aim;

	//举报变速
	bool report_speed;

	//玩家列表
	std::vector<std::string> inline_players;

	//针对玩家ID
	int target_playerid;

	super_data() :report_wallhack(true), report_aim(true), report_time(5)
	{
		engine = find<Engine>(L"engine", "VEngineClient014");
		client = find<Client>(L"client_panorama", "VClient018");
		globalVars = **reinterpret_cast<GlobalVars***>((*reinterpret_cast<uintptr_t**>(client))[11] + 10);
		submitReport = FIND_PATTERN(decltype(submitReport), L"client_panorama", "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x28\x8B\x4D\x08");
		setClanTag = FIND_PATTERN(decltype(setClanTag), L"engine", "\x53\x56\x57\x8B\xDA\x8B\xF9\xFF\x15");

#ifdef _DEBUG
		std::cout << "VEngineClient014 address " << engine << std::endl;
		std::cout << "submitReport address " << submitReport << std::endl;
		std::cout << "setClanTag address " << setClanTag << std::endl;
#endif
	}
}super_data;

//开始举报
void repoter_players(super_data& super, bool clear = false)
{
	//没开举报
	if (!super.report_mode) return;

	//上一次举报得时间
	static float last_time = 0.0f;
	if (last_time + super.report_time > super.globalVars->realtime) return;

	//举报过的列表
	static std::vector<uint64_t> report_list;
	if (clear)
	{
		report_list.clear();
		return;
	};

	//获取引擎类指针
	Engine* engine = super.engine;

	//获取自己信息
	PlayerInfo local_player;
	if (!engine->getPlayerInfo(engine->getLocalPlayer(), local_player)) return;

	//举报信息
	std::string report_data;
	if (super.report_curse) report_data += "textabuse,";
	if (super.report_grief) report_data += "grief,";
	if (super.report_wallhack) report_data += "wallhack,";
	if (super.report_aim) report_data += "aimbot,";
	if (super.report_speed) report_data += "speedhack,";

	//没有举报信息
	if (report_data.empty()) return;

	//清空在线列表
	super.inline_players.clear();

	//对每一个玩家进行操作
	for (int i = 1; i <= engine->getMaxClients(); i++)
	{
		//获取玩家信息
		PlayerInfo player_info;
		if (!engine->getPlayerInfo(i, player_info)) continue;
			
		//如果是假玩家或者是自己或者SteamID不存在
		if (player_info.fakeplayer || local_player.userId == player_info.userId) continue;

		//开始举报
		if (super.report_mode == 2)
		{
			//列表保存
			std::string temp = std::to_string(player_info.userId);
			temp += "\t";
			temp += player_info.name;
			super.inline_players.push_back(std::move(temp));

			if (super.target_playerid == player_info.userId)//ID相同
			{
				super.submitReport(std::to_string(player_info.xuid).c_str(), report_data.c_str());
				last_time = super.globalVars->realtime;
				break;
			}
		}
		else
		{
			if(std::find(report_list.cbegin(), report_list.cend(), player_info.xuid) != report_list.cend()) continue;
			super.submitReport(std::to_string(player_info.xuid).c_str(), report_data.c_str());
			last_time = super.globalVars->realtime;
			report_list.push_back(player_info.xuid);
			break;
		}
	}
}

//更改氏族标记
void change_clantag(super_data& super, std::string clantag)
{
	//如果前面和后面都不是空格符
	if (!isblank(clantag.front() && !isblank(clantag.back()))) clantag.push_back(' ');

	//设置氏族
	super.setClanTag(clantag.c_str(), clantag.c_str());
}

