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

//�ڴ�ģʽ���Ҹ�����
//__VA_ARGS__ ��һ���ɱ�����ĺ꣬������֪������꣬����ɱ�����ĺ����µ�C99�淶�������ģ�Ŀǰ�ƺ�ֻ��gcc֧��
#define FIND_PATTERN(type, ...) \
reinterpret_cast<type>(findPattern(__VA_ARGS__))

//��Ե�ַת��Ϊ���Ե�ַ
template <typename T>
static constexpr auto relativeToAbsolute(int* address) noexcept
{
	return reinterpret_cast<T>(reinterpret_cast<char*>(address + 1) + *address);
}

//�ڴ�ģʽ����
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

//�����麯��
template<typename T, typename ...Args>
constexpr auto callVirtualMethod(void* classBase, int index, Args... args) noexcept
{
	return ((*reinterpret_cast<T(__thiscall***)(void*, Args...)>(classBase))[index])(classBase, args...);
}

//��ȡָ����ָ��
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

//�����Ϣ
struct PlayerInfo
{
	std::uint64_t version;
	union
	{
		std::uint64_t xuid;//���SteamID
		struct
		{
			std::uint32_t xuidLow;
			std::uint32_t xuidHigh;
		};
	};
	char name[128];//�������
	int userId;//���ID
	char guid[33];
	std::uint32_t friendsId;
	char friendsName[128];
	bool fakeplayer;//�Ƿ��ǵ������
	bool hltv;
	int customfiles[4];
	unsigned char filesdownloaded;
	int entityIndex;
};

//��������
class matrix3x4 
{
private:
	float mat[3][4];
public:
	constexpr auto operator[](int i) const noexcept { return mat[i]; }
};

//���������ṹ
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

//�û������нṹ
struct UserCmd 
{
	enum 
	{
		IN_ATTACK = 1 << 0,//����
		IN_JUMP = 1 << 1,//��Ծ
		IN_DUCK = 1 << 2,//����
		IN_FORWARD = 1 << 3,//ǰ��
		IN_BACK = 1 << 4,//����
		IN_USE = 1 << 5,
		IN_MOVELEFT = 1 << 9,//����
		IN_MOVERIGHT = 1 << 10,//����
		IN_ATTACK2 = 1 << 11,
		IN_SCORE = 1 << 16,
		IN_BULLRUSH = 1 << 22//��
	};
	int pad;
	int commandNumber;
	int tickCount;
	Vector viewangles;//��ͼ�ӽ�
	Vector aimdirection;//���鷽��
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

//ȫ�ֱ����ṹ
struct GlobalVars 
{
	const float realtime;//ʵʱʱ��
	const int framecount;//֡����
	const float absoluteFrameTime;
	const std::byte pad[4];
	float currenttime;//��ǰʱ��
	float frametime;//֡ʱ��
	const int maxClients;//�ͻ�������
	const int tickCount;
	const float intervalPerTick;
};

//������
class Engine
{
public:
	//����������ȡ�����Ϣ
	constexpr auto getPlayerInfo(int entityIndex, const PlayerInfo& playerInfo) noexcept
	{
		return callVirtualMethod<bool, int, const PlayerInfo&>(this, 8, entityIndex, playerInfo);
	}

	//�������ID��ȡ�������
	constexpr auto getPlayerForUserID(int userId) noexcept
	{
		return callVirtualMethod<int, int>(this, 9, userId);
	}

	//��ȡ�����������
	constexpr auto getLocalPlayer() noexcept
	{
		return callVirtualMethod<int>(this, 12);
	}

	//��ȡ��ͼ�Ƕ�
	constexpr auto getViewAngles(Vector& angles) noexcept
	{
		callVirtualMethod<void, Vector&>(this, 18, angles);
	}

	//������ͼ�Ƕ�
	constexpr auto setViewAngles(const Vector& angles) noexcept
	{
		callVirtualMethod<void, const Vector&>(this, 19, angles);
	}

	//��ȡ��ǰ��ҵ�����
	constexpr auto getMaxClients() noexcept
	{
		return callVirtualMethod<int>(this, 20);
	}

	//�ж��Ƿ��ڶԾ���
	constexpr auto isInGame() noexcept
	{
		return callVirtualMethod<bool>(this, 26);
	}

	//�ж���Ϸ�Ƿ�����
	constexpr auto isConnected() noexcept
	{
		return callVirtualMethod<bool>(this, 27);
	}

	using Matrix = float[4][4];

	//��ȡ�������
	constexpr auto worldToScreenMatrix() noexcept
	{
		return callVirtualMethod<const Matrix&>(this, 37);
	}

	constexpr auto getBSPTreeQuery() noexcept
	{
		return callVirtualMethod<void*>(this, 43);
	}

	//��ȡ�ȼ�����
	constexpr auto getLevelName() noexcept
	{
		return callVirtualMethod<const char*>(this, 53);
	}

	constexpr auto clientCmdUnrestricted(const char* cmd) noexcept
	{
		callVirtualMethod<void, const char*, bool>(this, 114, cmd, false);
	}
};

//�ͻ�����
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

//������������
typedef struct super_data
{
	//������
	Engine* engine;

	//�ͻ�����
	Client* client;

	//ȫ�ֱ����ṹ
	GlobalVars* globalVars;

	//�ٱ�����
	std::add_pointer_t<bool __stdcall(const char*, const char*)> submitReport;

	//���������Ǻ���
	std::add_pointer_t<void __fastcall(const char*, const char*)> setClanTag;

	//�ٱ�ģʽ
	int report_mode;

	//�ٱ�ʱ����
	int report_time;

	//�ٱ�����
	bool report_curse;

	//�ٱ�������Ϸ
	bool report_grief;

	//�ٱ�͸��
	bool report_wallhack;

	//�ٱ�����
	bool report_aim;

	//�ٱ�����
	bool report_speed;

	//����б�
	std::vector<std::string> inline_players;

	//������ID
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

//��ʼ�ٱ�
void repoter_players(super_data& super, bool clear = false)
{
	//û���ٱ�
	if (!super.report_mode) return;

	//��һ�ξٱ���ʱ��
	static float last_time = 0.0f;
	if (last_time + super.report_time > super.globalVars->realtime) return;

	//�ٱ������б�
	static std::vector<uint64_t> report_list;
	if (clear)
	{
		report_list.clear();
		return;
	};

	//��ȡ������ָ��
	Engine* engine = super.engine;

	//��ȡ�Լ���Ϣ
	PlayerInfo local_player;
	if (!engine->getPlayerInfo(engine->getLocalPlayer(), local_player)) return;

	//�ٱ���Ϣ
	std::string report_data;
	if (super.report_curse) report_data += "textabuse,";
	if (super.report_grief) report_data += "grief,";
	if (super.report_wallhack) report_data += "wallhack,";
	if (super.report_aim) report_data += "aimbot,";
	if (super.report_speed) report_data += "speedhack,";

	//û�оٱ���Ϣ
	if (report_data.empty()) return;

	//��������б�
	super.inline_players.clear();

	//��ÿһ����ҽ��в���
	for (int i = 1; i <= engine->getMaxClients(); i++)
	{
		//��ȡ�����Ϣ
		PlayerInfo player_info;
		if (!engine->getPlayerInfo(i, player_info)) continue;
			
		//����Ǽ���һ������Լ�����SteamID������
		if (player_info.fakeplayer || local_player.userId == player_info.userId) continue;

		//��ʼ�ٱ�
		if (super.report_mode == 2)
		{
			//�б���
			std::string temp = std::to_string(player_info.userId);
			temp += "\t";
			temp += player_info.name;
			super.inline_players.push_back(std::move(temp));

			if (super.target_playerid == player_info.userId)//ID��ͬ
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

//����������
void change_clantag(super_data& super, std::string clantag)
{
	//���ǰ��ͺ��涼���ǿո��
	if (!isblank(clantag.front() && !isblank(clantag.back()))) clantag.push_back(' ');

	//��������
	super.setClanTag(clantag.c_str(), clantag.c_str());
}

