#pragma once
#ifndef _CRT_SECURE_NO_WARNINGS
#define  _CRT_SECURE_NO_WARNINGS
#endif //_CRT_SECURE_NO_WARNINGS

#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_dx9.h"
#include "Imgui/imgui_impl_win32.h"

#include "D3DHook.h"
#include "SuperHack.h"
#include <filesystem>

//��Ϸ���ݱ���ṹ
typedef struct game_data
{
	void* dll_module;								//��ǰdll��ַ
	d3dhook::D3DHook d3d_hook;			//hook��
	DWORD pid;										//��Ϸ����ID
	HANDLE game_proc;							//��Ϸ���̾��
	HWND game_hwnd;							//��Ϸ���ھ��
	WNDPROC old_proc;							//��Ϸ����ԭʼ����
	bool show_meun;								//��ʾ��Ϸ�˵�
	int self_camp;										//�Լ���Ӫ
	int game_width;									//��Ϸ���ڿ��
	int game_height;									//��Ϸ���ڸ߶�

	int matrix_address;								//�����ַ
	int self_address;									//�����ַ
	int enemy_address;								//���˵�ַ
	int angle_address;								//�Ƕȵ�ַ

	bool show_enemy;								//��ʾ����
	bool show_friend;								//��ʾ����

	bool open_mirror;								//��������
	bool right_down;									//�Ҽ�����
	bool quiet_step;									//��������
	bool player_squat;								//�¶�����

	int tolerate_angle;								//������̽Ƕ�
	int mirror_ms;										//�������鿪ǹ���
	int mode_type;									//ģʽ����
	int distance_type;								//��������

	float aim_offset;									//����΢��

	bool ignore_flash;								//��������
	bool show_armor;								//��ʾ����ֵ
	bool show_money;								//��ʾ��Ǯֵ
	bool show_blood;								//��ʾѪ��ֵ

	float aim_data[3];								//����λ��
	bool start_aim;										//�����ʶ����ֹɱ�����˺�����������һ��

	game_data()
	{
		show_meun = true;
		aim_offset = 1.5f;
		mode_type = 1;
		mirror_ms = 25;
		tolerate_angle = 45;
	}
}game_data;

//��������ģ��
void hide_self(void* module);

//��ʼ���̺߳���
void __cdecl  _beginthread_proc(void*);

//����Ƿ���Ϸ����
bool check_csgo(const char* str = "Counter-Strike: Global Offensive");

//�������Կ���̨
void create_debug();

//��������ӿ�
void show_err(const char* str);

//D3D9��EndScene
HRESULT _stdcall my_endscene(IDirect3DDevice9* pDirect3DDevice);

//D3D9��Reset
HRESULT _stdcall my_reset(IDirect3DDevice9* pDirect3DDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);

//��Ϸ�����¹��̺���
HRESULT _stdcall my_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//��ʼ��Imgui����
void init_imgui();

//��ʼ����Ϸ����
void init_gamedatas();

//���Ʋ˵�
void draw_meun();

//���׹���
void hack_manager();

//����ֱ��
void draw_line(int x, int y, int w, int h, int color);

//���Ʒ���
void draw_box(int x, int y, int w, int h, int color);

//���ƹ�������
void draw_espbox(int x, int y, int w, int h, int color);

//����Ѫ��
void draw_blood(int x, int y, int h, int blood);

//���ƻ���
void draw_armor(int x, int y, int h, int armor);

//���ƽ�Ǯ
void draw_meney(int x, int y, int w, int meney);

//��ʼ����
void aim_bot(float* self_data, float* enemy_data);

//������﷽������
void clear_boxs();


