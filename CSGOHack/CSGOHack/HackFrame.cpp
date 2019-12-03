#include "HackFrame.h"

IDirect3D9* g_pOwnDirect3D = nullptr;							//�Լ���IDirect3D9
IDirect3DDevice9* g_pOwnDirect3DDevice = nullptr;				//�Լ���IDirect3DDevice9

IDirect3D9* g_pGameDirect3D = nullptr;							//��Ϸ�����IDirect3D9
IDirect3DDevice9* g_pGameDirect3DDevice = nullptr;				//��Ϸ�����IDirect3DDevice9

Type_Reset g_fReset = nullptr;									//Reset
Type_Present g_fPresent = nullptr;						 		//Present
Type_EndScene g_fEndScene = nullptr;							//EndScene
Type_DrawIndexedPrimitive g_fDrawIndexedPrimitive = nullptr;	//DrawIndexedPrimitive

HWND g_hGameWindow = NULL;										//��Ϸ���ھ��
WNDPROC g_fOriginProc = nullptr;								//ԭ���ں�����ַ

bool g_bCallOne = true;											//ֻ����һ��
bool g_bShowImgui = true;										//��ʾimgui�˵�
bool g_bShowDefenders = false;									//��ʾ������
bool g_bShowSleeper = false;									//��ʾǱ����
bool g_bDrawBox = false;										//��ʾ���﷽��
bool g_bAiMBot = false;											//��������
bool g_bDistanceAimBot = false;									//��������
bool g_bEspAimBot = false;										//��������

bool g_bInitAimbot = false;										//��ʼ������
float g_fAimBotPos[Coordinate_Number];							//����λ��

float g_fAiMBotSet = 1.0f;										//����΢��

int g_nOwnCamp = 0;												//�Լ���Ӫ

std::list<UINT> g_cDefenderStride;								//�����߱�ʶ
std::list<UINT> g_cSleeperStride;								//Ǳ���߱�ʶ								

int g_nMetrixBaseAddress = 0;									//�����ַ
int g_nAngleBaseAddress = 0;									//ƫ���ǣ������ǻ�ַ
int g_nOwnBaseAddress = 0;										//�Լ���ַ
int g_nTargetBaseAddress = 0;									//���˻�ַ
//int g_nTargetEsp = 0;											//���˹���[�ǵ��˻�ַ]

blackbone::Process g_cGame;										//��Ϸ������Ϣ

void CreateDebugConsole()
{
#ifdef _DEBUG
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
		//InitializeStride();
	}

	DrawPlayerBox(pDirect3DDevice);
	DrawImgui();

	return g_fEndScene(pDirect3DDevice);
}

HRESULT _stdcall MyDrawIndexedPrimitive(IDirect3DDevice9 * pDirect3DDevice, D3DPRIMITIVETYPE stType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
	//bool bTargetModel = false;
	//auto CheckDefender = [&]()->void
	//{
	//	if (bTargetModel == false && g_bShowDefenders)
	//	{
	//		for (auto it : g_cDefenderStride)
	//		{
	//			if (it == NumVertices)
	//			{
	//				bTargetModel = true;
	//				break;
	//			}
	//		}
	//	}
	//};
	//auto CheckSleeper = [&]()->void
	//{
	//	if (bTargetModel == false && g_bShowSleeper)
	//	{
	//		for (auto it : g_cSleeperStride)
	//		{
	//			if (it == NumVertices)
	//			{
	//				bTargetModel = true;
	//				break;
	//			}
	//		}
	//	}
	//};

	//CheckDefender();
	//CheckSleeper();

	//IDirect3DBaseTexture9* pOldTexture = nullptr;		//ԭ��������
	//IDirect3DTexture9* pNewTexture = nullptr;			//��ɫ������
	//if (bTargetModel)
	//{
	//	pDirect3DDevice->SetRenderState(D3DRS_ZENABLE, false);//����͸��
	//	pDirect3DDevice->GetTexture(0, &pOldTexture);//����ԭ������
	//	pDirect3DDevice->SetTexture(0, pNewTexture);//����Ϊ��ɫ����
	//	g_fDrawIndexedPrimitive(pDirect3DDevice, stType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	//	pDirect3DDevice->SetRenderState(D3DRS_ZENABLE, true);//�ر�͸��
	//	if(pOldTexture)
	//		pDirect3DDevice->SetTexture(0, pOldTexture);
	//}
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
		ImGui::Begin(u8"CSGO͸�����鸨��ģ��", &g_bShowImgui);

		ImGui::Text(u8"����˵��:Insert   �˵���ʾ����");
		ImGui::Text(u8"����˵��:Shift    �����������");
		//ImGui::Checkbox(u8"͸�ӱ�����", &g_bShowDefenders);
		//ImGui::Checkbox(u8"͸��Ǳ����", &g_bShowSleeper);
		ImGui::Checkbox(u8"��������", &g_bAiMBot);
		ImGui::Checkbox(u8"��������ģʽ", &g_bDistanceAimBot);
		ImGui::SliderFloat(u8"����΢��", &g_fAiMBotSet, -20.0f, 20.0f);
		if (ImGui::Checkbox(u8"��������", &g_bEspAimBot))
		{
			g_bAiMBot = false;
			g_bDistanceAimBot = false;
			g_fAiMBotSet = 2.0;
		}
		ImGui::Checkbox(u8"���﷽��", &g_bDrawBox);
		if (g_nOwnCamp == 2) ImGui::Text(u8"�Լ���Ӫ:  Ǳ����");
		if (g_nOwnCamp == 3) ImGui::Text(u8"�Լ���Ӫ:  ������");
			
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
		if (wParam == VK_INSERT)//�Ƿ���ʾ�˵�
			g_bShowImgui = !g_bShowImgui;
	}

	if (g_fOriginProc)
		return CallWindowProcA(g_fOriginProc, hWnd, uMsg, wParam, lParam);

	return true;
}

void InitializeStride()
{
	//UINT nDefenderArray[] = { 64,116,136,140,256,934,991,1222,1258,1310,1374,1383,1410,1432,1439,1601,1611,1645,1702,1735,1967,2052,
	//2112,2151,2157,2254,2266,2482,2489,2539,2856,2924,2963,3093,3152,3137,3245,3630,4084,4320,4422,4510,4533,4666,5299,7163 };
	//UINT nSleeperArray[] = { 200,252,290,622,646,692,903,924,927,1003,1006,1053,1106,1197,1183,1214,1215,1273,1314,1345,1380,1395,
	//1462,1525,1671,1624,1914,1925,1996,2056,2112,2369,2447,2536,2663,3164,3225,3368,3692,3742,3836,3932,4159,4410,5572,7236};
	//for (int i = 0; i < sizeof(nDefenderArray) / sizeof(UINT); i++)
	//	g_cDefenderStride.emplace_back(nDefenderArray[i]);
	//for (int i = 0; i < sizeof(nSleeperArray) / sizeof(UINT); i++)
	//	g_cSleeperStride.emplace_back(nSleeperArray[i]);
}

void InitializeData()
{
	g_cGame.Attach(GetCurrentProcessId());
	auto nClientBaseAddress = g_cGame.modules().GetModule(std::wstring(L"client_panorama.dll"))->baseAddress;
	auto nEngineBaseAddress = g_cGame.modules().GetModule(std::wstring(L"engine.dll"))->baseAddress;

	int nMatrixOffset = 0x4CFEAD4;
	int nAngleOffset = 0x590D8C;
	int nOwnBaseOffset = 0xD27AAC;
	int nTargetOffset = 0x4D0D0a4;
	//int nTargetEspOffset = 0x4D09F04;
	
	g_nMetrixBaseAddress = static_cast<int>(nClientBaseAddress) + nMatrixOffset;
	g_nAngleBaseAddress = static_cast<int>(nEngineBaseAddress) + nAngleOffset;
	g_nOwnBaseAddress = static_cast<int>(nClientBaseAddress) + nOwnBaseOffset;
	g_nTargetBaseAddress = static_cast<int>(nClientBaseAddress) + nTargetOffset;
	//g_nTargetEsp = static_cast<int>(nClientBaseAddress) + nTargetEspOffset;
}

void DrawBox(IDirect3DDevice9* pDirect3DDevice, D3DCOLOR dwColor, int x, int y, int w, int h,int nBlood)
{
	auto FillRgbColor = [&](int x, int y, int w, int h, D3DCOLOR _dwColor) -> void
	{
		x = (x <= 0) ? 1 : x;
		y = (y <= 0) ? 1 : y;
		w = (w <= 0) ? 1 : w;
		h = (h <= 0) ? 1 : h;

		D3DRECT stRect = { x,y,x + w,y + h };
		pDirect3DDevice->Clear(1, &stRect, D3DCLEAR_TARGET, _dwColor, 0.0f, 0);
	};
	auto DrawBorder = [&](int x, int y, int w, int h, int nPixel = 1) -> void
	{
		FillRgbColor(x, y + h, w, nPixel, dwColor);			//�����±�
		FillRgbColor(x, y, nPixel, h, dwColor);				//�������
		FillRgbColor(x, y, w, nPixel, dwColor);				//�����ϱ�
		FillRgbColor(x + w, y, nPixel, h, dwColor);			//�����ұ�
	};
	auto DrawBlood = [&]() -> void
	{
		int nHeight = (int)(((float)nBlood / 100.0f) * (float)h);	//��ȡѪ��������
		FillRgbColor(x - 10, y, 3, nHeight, dwColor - 50);			//����Ѫ����
	};

	DrawBorder(x, y, w, h);
	if (nBlood) DrawBlood();
}

void DrawPlayerBox(IDirect3DDevice9* pDirect3DDevice)
{
	auto IsOwn = [](const float* fPos1, const float* fPos2) -> bool
	{
		if ((abs(fPos1[Pos_X] - fPos2[Pos_X]) < 10) && (abs(fPos1[Pos_Y] - fPos2[Pos_Y]) < 10))
				return true;
		return false;
	};

	if (g_bDrawBox)
	{
		//��ȡ��Ϸ���ڵ�һ���Ⱥ͸߶�
		D3DVIEWPORT9 stView;
		if (FAILED(pDirect3DDevice->GetViewport(&stView)))return;
		DWORD dwHalfWidth = stView.Width / 2;
		DWORD dwHalfHeight = stView.Height / 2;

		int nOwnPosBaseAddress = 0;							//�Լ��������ַ
		int nOwnPosOffset = 0x35A4;							//�Լ�����ƫ��
		float pOwnPos[Coordinate_Number];					//�Լ�����

		int nBoxColor = D3DCOLOR_XRGB(0, 0, 255);			//���﷽����ɫ

		int nPlayerCamp = 3;								//������Ӫ
		int nPlayerCmapOffset = 0xf4;						//������Ӫƫ��

		int nPlayerAddress = 0;								//�����ַ
		int nNextPlayerAddress = 0x10;						//��һ������ĵ�ַƫ��
		int nPlayerPosOffset = 0xa0;						//�����ַƫ��

		int nPlayerBlood = 0;								//����Ѫ��
		int nPlayerBloodOffset = 0x100;						//����Ѫ��ƫ��

		float fPlayerPos[Coordinate_Number];				//�����ַ
		AiMBotInfo stAiMBotInfo;							//�������������

		int nTargetEspNext = 0x10;							//��һ���������λ��
		int nTargetEspOffset1 = 0x26a8;						//����һ��ƫ��
		int nTargetEspOffset2 = 0;							//��������ƫ��
		int nTaegerEspAddress = 0;							//����ͷ����ַ
		float fEspPlayerPos[Coordinate_Number];				//����ͷ������

		int nFlashAlphaAddress = 0xA3F0;					//����͸���ȵ�ַ
		float fFlashAlpha = 10.0;							//����͸����

		float fOwnMatrix[4][4];								//����

		//��ȡ������Ϣ
		g_cGame.memory().Read(g_nMetrixBaseAddress, sizeof(fOwnMatrix), fOwnMatrix);
		//��ȡ�ڴ��Լ���ַ
		g_cGame.memory().Read(g_nOwnBaseAddress, sizeof(int), &nOwnPosBaseAddress);
		//�����ڴ����ַ����ƫ�Ƶõ����ǵ�ǰ������
		g_cGame.memory().Read(nOwnPosBaseAddress + nOwnPosOffset, sizeof(pOwnPos), pOwnPos);

		//��ȡ�����Լ�����Ӫ
		//g_cGame.memory().Read(g_nTargetBaseAddress, sizeof(int), &nPlayerAddress);
		//g_cGame.memory().Read(nPlayerAddress + nPlayerCmapOffset, sizeof(nOwnCamp), &nOwnCamp);
		//g_nOwnCamp = nOwnCamp;
		//std::cout << nOwnCamp << std::endl;
		
		for (int i = 0; i <= 30; i++)
		{
			//��ȡ�����ַ
			g_cGame.memory().Read(g_nTargetBaseAddress + i * nNextPlayerAddress, sizeof(int), &nPlayerAddress);
			if(nPlayerAddress == 0)continue;

			//��ȡ����Ѫ��
			g_cGame.memory().Read(nPlayerAddress + nPlayerBloodOffset, sizeof(nPlayerBlood), &nPlayerBlood);

			//����������
			if (nPlayerBlood <= 0)continue;

			//��ȡ��������
			g_cGame.memory().Read(nPlayerAddress + nPlayerPosOffset, sizeof(fPlayerPos), fPlayerPos);

			//��ȡ������Ӫ
			g_cGame.memory().Read(nPlayerAddress + nPlayerCmapOffset, sizeof(nPlayerCamp), &nPlayerCamp);

			//���Լ�
			if (IsOwn(pOwnPos, fPlayerPos))
			{
				//����͸����
				g_cGame.memory().Write(nPlayerAddress + nFlashAlphaAddress, sizeof(float), &fFlashAlpha);

				g_nOwnCamp = nPlayerCamp;
				continue;
			}

			//����
			float fTarget =
				fOwnMatrix[2][0] * fPlayerPos[Pos_X]
				+ fOwnMatrix[2][1] * fPlayerPos[Pos_Y]
				+ fOwnMatrix[2][2] * fPlayerPos[Pos_Z]
				+ fOwnMatrix[2][3];

			//���Ǻ���ĵ��˲���Ҫ����͸��
			if (fTarget < 0.01f)continue;

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

			//��ȡ���˹���
			if (g_bEspAimBot)
			{
				//g_cGame.memory().Read(g_nTargetEsp + nTargetEspNext*i, sizeof(int), &nTaegerEspAddress);
				//std::cout << "Base Address: " << nTaegerEspAddress << std::endl;
				g_cGame.memory().Read(nPlayerAddress + nTargetEspOffset1, sizeof(int), &nTaegerEspAddress);
				//std::cout << "Firs tAddress: " << nTaegerEspAddress << std::endl;
				//g_cGame.memory().Read(nTaegerEspAddress + nTargetEspOffset2, sizeof(int), &nTaegerEspAddress);
				nTaegerEspAddress += nTargetEspOffset2;
				//std::cout << "Next Address : " << nTaegerEspAddress << std::endl;	//87
				g_cGame.memory().Read(nTaegerEspAddress + 99 * sizeof(float), sizeof(float), &fEspPlayerPos[Pos_X]);
				g_cGame.memory().Read(nTaegerEspAddress + 103 * sizeof(float), sizeof(float), &fEspPlayerPos[Pos_Y]);
				g_cGame.memory().Read(nTaegerEspAddress + 107 * sizeof(float), sizeof(float), &fEspPlayerPos[Pos_Z]);
			}

			if ((nPlayerCamp != g_nOwnCamp) && g_bEspAimBot)//�����������
			{
				int X = abs((int)(dwHalfWidth - fBoxX));
				int Y = abs((int)dwHalfHeight - static_cast<int>(fBoxY_H));
				if (stAiMBotInfo.fVector > X + Y)
				{
					stAiMBotInfo.fVector = (float)(X + Y);
					stAiMBotInfo.fPlayerPos[Pos_X] = fEspPlayerPos[Pos_X];
					stAiMBotInfo.fPlayerPos[Pos_Y] = fEspPlayerPos[Pos_Y];
					stAiMBotInfo.fPlayerPos[Pos_Z] = fEspPlayerPos[Pos_Z];
				}
				//std::cout << "����:" << fLen << std::endl;
				//std::cout << fEspPlayerPos[Pos_X] << std::endl;
				//std::cout << fEspPlayerPos[Pos_Y] << std::endl;
				//std::cout << fEspPlayerPos[Pos_Z] << std::endl;
			}
			//������Ӫ��Ҳ���ǲ��Զ��ѽ����������
			else if (g_bAiMBot && (nPlayerCamp != g_nOwnCamp))
			{
				if (g_bDistanceAimBot)//�����������
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
				else//׼�Ǿ�������
				{
					int X = abs((int)(dwHalfWidth - fBoxX));
					int Y = abs((int)dwHalfHeight - static_cast<int>(fBoxY_H));
					if (stAiMBotInfo.fVector > X + Y)
					{
						stAiMBotInfo.fVector = (float)(X + Y);
						stAiMBotInfo.fPlayerPos[Pos_X] = fPlayerPos[Pos_X];
						stAiMBotInfo.fPlayerPos[Pos_Y] = fPlayerPos[Pos_Y];
						stAiMBotInfo.fPlayerPos[Pos_Z] = fPlayerPos[Pos_Z];
					}
				}
			}

			if (nPlayerCamp == 2)
				nBoxColor = D3DCOLOR_XRGB(255, 0, 0);//Ǳ����
			if (nPlayerCamp == 3)
				nBoxColor = D3DCOLOR_XRGB(0, 255, 0);//������

			//std::cout
			//	<< "X����ԭ��" << abs((int)(dwHalfWidth - fBoxX))
			//	<< "Y����ԭ��" << abs((int)dwHalfHeight - static_cast<int>(fBoxY_H))
			//	<< std::endl;

			DrawBox(pDirect3DDevice, nBoxColor,
				static_cast<int>(fBoxX - ((fBoxY_W - fBoxY_H) / 4.0f)), static_cast<int>(fBoxY_H),
				static_cast<int>((fBoxY_W - fBoxY_H) / 2.0f), static_cast<int>(fBoxY_W - fBoxY_H), nPlayerBlood);
		}

		//������������������
		if (g_bAiMBot || g_bEspAimBot)
		{
			if (GetAsyncKeyState(VK_SHIFT) & 0x8000)//����Shite
				AiMBot(pOwnPos, stAiMBotInfo.fPlayerPos);
			else
			{
				g_bInitAimbot = false;
			}
		}
	}
}

void AiMBot(float* pOwnPos,float* pEnemyPos)
{
	int nAngleAddress = 0;					//ƫб�Ǻ͸����ǵ�ַ
	int nAngleOffsetX = 0x4d88;				//ƫб�Ǻ͸�����ƫ��

	float pAngle[Angle_Number];				//ƫб�Ǻ͸�����
	static float PI = 3.1415f;				//

	//�ոտ�ʼ����һ������
	if (g_bInitAimbot == false)
	{
		g_fAimBotPos[Pos_X] = pEnemyPos[Pos_X];
		g_fAimBotPos[Pos_Y] = pEnemyPos[Pos_Y];
		g_fAimBotPos[Pos_Z] = pEnemyPos[Pos_Z];
		g_bInitAimbot = true;
	}
	else//���������ŵ��ˣ��ж�һ�½Ƕȣ���ֹ����һ�����˺�������׼��һ������
	{
		//������5�Ⱦ�����һ�������ˣ����Բ������������
		float fMaxLen = 80.0f;
		float fLenX = abs(g_fAimBotPos[Pos_X] - pEnemyPos[Pos_X]);
		float fLenY = abs(g_fAimBotPos[Pos_Y] - pEnemyPos[Pos_Y]);
		float fLenZ = abs(g_fAimBotPos[Pos_Z] - pEnemyPos[Pos_Z]);

		if (fLenX > fMaxLen || fLenY > fMaxLen || fLenZ > fMaxLen) return;
	}

	//�õ���������͵�������Ĳ�ֵ
	float fDifferX = pOwnPos[Pos_X] - pEnemyPos[Pos_X];
	float fDifferY = pOwnPos[Pos_Y] - pEnemyPos[Pos_Y];
	float fDifferZ = pOwnPos[Pos_Z] - pEnemyPos[Pos_Z];
	if (g_bEspAimBot)fDifferZ += 60 + g_fAiMBotSet;

	//����ƫб�Ƕ�X
	pAngle[Angle_X] = atan(fDifferY / fDifferX);
	if (fDifferX >= 0.0f && fDifferY >= 0.0f)
		pAngle[Angle_X] = pAngle[Angle_X] / PI * 180 - 180;
	else if (fDifferX < 0.0f && fDifferY >= 0.0f)
		pAngle[Angle_X] = pAngle[Angle_X] / PI * 180;
	else if (fDifferX < 0.0f && fDifferY < 0.0)
		pAngle[Angle_X] = pAngle[Angle_X] / PI * 180;
	else if (fDifferX >= 0.0f && fDifferY < 0.0f)
		pAngle[Angle_X] = pAngle[Angle_X] / PI * 180 + 180;

	//����������Y
	pAngle[Angle_Y] = atan(fDifferZ / sqrt(fDifferX * fDifferX + fDifferY * fDifferY)) / PI * 180.0f;
	if (g_bAiMBot)pAngle[Angle_Y] += g_fAiMBotSet;

	//��ȡ�Ƕȵĵ�ַ
	g_cGame.memory().Read(g_nAngleBaseAddress, sizeof(int), &nAngleAddress);

	//���Ƕ�д��
	g_cGame.memory().Write(nAngleAddress + nAngleOffsetX, sizeof(pAngle), pAngle);
}

void HideModule(void* pModule)
{
	void* pPEB = nullptr;

	//��ȡPEBָ��
	_asm
	{
		push eax
		mov eax, fs:[0x30]
		mov pPEB, eax
		pop eax
	}

	//�����õ�����ȫ��ģ���˫������ͷָ��
	void* pLDR = *((void**)((unsigned char*)pPEB + 0xc));
	void* pCurrent = *((void**)((unsigned char*)pLDR + 0x0c));
	void* pNext = pCurrent;

	//��������б�������ָ��ģ����ж�������
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