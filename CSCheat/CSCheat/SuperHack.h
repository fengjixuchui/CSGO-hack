#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <sstream>
#include <type_traits>
#include <iostream>
#include <string>
#include <vector>

#define FIND_PATTERN(type, ...) \
reinterpret_cast<type>(findPattern(__VA_ARGS__))

//��Ե�ַת��
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

	if (MODULEINFO moduleInfo; GetModuleInformation(GetCurrentProcess(), GetModuleHandleW(module), &moduleInfo, sizeof(moduleInfo))) {
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

		if (!*second)
			return reinterpret_cast<std::uintptr_t>(const_cast<char*>(start) + offset);
	}
	MessageBoxA(NULL, ("Failed to find pattern #" + std::to_string(id) + '!').c_str(), "Osiris", MB_OK | MB_ICONWARNING);
	return 0;
}

//�����麯��
template<typename T, typename ...Args>
constexpr auto callVirtualMethod(void* classBase, int index, Args... args) noexcept
{
	return ((*reinterpret_cast<T(__thiscall***)(void*, Args...)>(classBase))[index])(classBase, args...);
}

//��ȡָ����
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
		std::uint64_t xuid;
		struct
		{
			std::uint32_t xuidLow;
			std::uint32_t xuidHigh;
		};
	};
	char name[128];
	int userId;
	char guid[33];
	std::uint32_t friendsId;
	char friendsName[128];
	bool fakeplayer;
	bool hltv;
	int customfiles[4];
	unsigned char filesdownloaded;
	int entityIndex;
};

//������
class Engine
{
public:
	constexpr auto getPlayerInfo(int entityIndex, const PlayerInfo& playerInfo) noexcept
	{
		return callVirtualMethod<bool, int, const PlayerInfo&>(this, 8, entityIndex, playerInfo);
	}

	constexpr auto getPlayerForUserID(int userId) noexcept
	{
		return callVirtualMethod<int, int>(this, 9, userId);
	}

	constexpr auto getLocalPlayer() noexcept
	{
		return callVirtualMethod<int>(this, 12);
	}

	constexpr auto getMaxClients() noexcept
	{
		return callVirtualMethod<int>(this, 20);
	}

	constexpr auto isInGame() noexcept
	{
		return callVirtualMethod<bool>(this, 26);
	}

	constexpr auto isConnected() noexcept
	{
		return callVirtualMethod<bool>(this, 27);
	}

	using Matrix = float[4][4];

	constexpr auto worldToScreenMatrix() noexcept
	{
		return callVirtualMethod<const Matrix&>(this, 37);
	}

	constexpr auto getBSPTreeQuery() noexcept
	{
		return callVirtualMethod<void*>(this, 43);
	}

	constexpr auto getLevelName() noexcept
	{
		return callVirtualMethod<const char*>(this, 53);
	}

	constexpr auto clientCmdUnrestricted(const char* cmd) noexcept
	{
		callVirtualMethod<void, const char*, bool>(this, 114, cmd, false);
	}
};

//������������
typedef struct super_data
{
	//������
	Engine* engine;

	//�ٱ�����
	std::add_pointer_t<bool __stdcall(const char*, const char*)> submitReport;

	//�ٱ�ģʽ
	int report_mode;

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

	super_data()
	{
		engine = find<Engine>(L"engine", "VEngineClient014");
		submitReport = FIND_PATTERN(decltype(submitReport), L"client_panorama", "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x28\x8B\x4D\x08");
	}
}super_data;

//��ʼ�ٱ�
void repoter_players(super_data& super)
{
	//û���ٱ�
	if (!super.report_mode) return;

	//�ٱ�ʱ����
	static int report_time = 100;
	if (--report_time > 0) return;
	else report_time = 100;

	//��ȡ������ָ��
	Engine* engine = super.engine;

	//��ȡ�Լ���Ϣ
	PlayerInfo local_player;
	if (!engine->getPlayerInfo(engine->getLocalPlayer(), local_player)) return;

	//��������б�
	super.inline_players.clear();

	//�ٱ���Ϣ
	std::string report_data;
	if (super.report_curse) report_data += "textabuse,";
	if (super.report_grief) report_data += "grief,";
	if (super.report_wallhack) report_data += "wallhack,";
	if (super.report_aim) report_data += "aimbot,";
	if (super.report_speed) report_data += "speedhack,";

	//��ÿһ����ҽ��в���
	for (int i = 1; i <= engine->getMaxClients(); i++)
	{
		//��ȡ�����Ϣ
		PlayerInfo player_info;
		if (!engine->getPlayerInfo(i, player_info)) continue;

		//����Ǽ���һ������Լ�
		if (player_info.fakeplayer || local_player.userId == player_info.userId) continue;

		//�б���
		std::string temp = std::to_string(player_info.userId);
		temp += "\t";
		temp += player_info.name;
		super.inline_players.push_back(std::move(temp));
		
		//��ʼ�ٱ�
		if (report_data.size())
		{
			if (super.report_mode == 2 && super.target_playerid == player_info.userId)
			{
				super.submitReport(std::to_string(player_info.xuid).c_str(), report_data.c_str());
				break;
			}
			if (super.report_mode == 1)
				super.submitReport(std::to_string(player_info.xuid).c_str(), report_data.c_str());
		}
	}	
}

