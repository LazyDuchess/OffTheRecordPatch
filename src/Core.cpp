#include "pch.h"
#include "Core.h"
#include "iostream"
#include "MinHook.h"
#include <tlhelp32.h>
#include "inject.h"
#include <thread>
#include <dinput.h>
#include "SteamAPI.h"
#include "outfits.h"
#include "dr2/Player.h"
#include "dr2/tActorAssignment.h"
#include "dr2/tPushableGoalInfo.h"
#include "dr2/cFrontEnd.h"
#include <timeapi.h>
#include "config.h"
#include "Addresses.h"

typedef void(__cdecl* DEBUGPRINT)(int, int, char*, ...);
typedef BOOL (WINAPI* SETWINDOWPOS)(HWND, HWND, int, int, int, int, UINT);
typedef LONG (WINAPI* SETWINDOWLONGW)(HWND, int, LONG);
typedef HWND (WINAPI* CREATEWINDOWEXW)(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
typedef void(__stdcall* INITIALIZEGAME)();
typedef int(__thiscall* UPDATESYSTEMS)(void*, float);
typedef bool(__thiscall* DETERMINEASSIGNMENTPUSHABLE)(void*, void*, DR2::tActorAssignment*);

DEBUGPRINT fpDebugPrint = NULL;
INITIALIZEGAME fpInitializeGame = NULL;
CREATEWINDOWEXW fpCreateWindowExW = NULL;
SETWINDOWLONGW fpSetWindowLongW = NULL;
SETWINDOWPOS fpSetWindowPos = NULL;
UPDATESYSTEMS fpUpdateSystems = NULL;
DETERMINEASSIGNMENTPUSHABLE fpDetermineAssignmentPushable = NULL;

Core* Core::_instance = nullptr;
float Core::DeltaTime = 0.0;
float Core::AdjustedDeltaTime = 0.0;

static DWORD AffinityMask = 1;

void __stdcall WaitForNextFrame(float deltaTime) {
	float howLongToWaitInSecs = Config::FPSDelta - deltaTime;
	auto howLongToWaitInMS = std::chrono::duration<float, std::milli>(howLongToWaitInSecs * 1000.0f);
	if (howLongToWaitInMS.count() < 1.0f)
		std::this_thread::yield();
	else
		std::this_thread::sleep_for(howLongToWaitInMS);
}

void __stdcall UpdateFPS() {
	if (!Config::UnlockFPSDuringLoading) return;
	auto fe = DR2::cFrontEnd::GetInstance();
	if (fe && fe->ScreenManager)
	{
		bool loading = fe->IsLoadingScreenActive();
		float uncapDelta = 0.0;
		if (loading)
			memcpy_s(Addresses::FPSLimit, sizeof(float), &uncapDelta, sizeof(float));
		else
			memcpy_s(Addresses::FPSLimit, sizeof(float), &Config::FPSDelta, sizeof(float));
	}
}

void __declspec(naked) MainLoopHook() {
	__asm {
		push edi
		push esi
		push edx
		push ebp
		push edx
		call UpdateFPS
		pop edx
		pop ebp
		pop edx
		pop esi
		pop edi
		mov eax, 0x00ac3f00
		call eax
		mov ebx, 0x00905281
		jmp ebx
	}
}

void __declspec(naked) FrameLimiterHook1() {
	__asm {
		fst[Core::DeltaTime]
		mov edi, 0x00D6EE70
		fld [edi]
		mov edi, 0x0090529f
		jmp edi
	}
}

void __declspec(naked) FrameLimiterHook2() {
	__asm {
		push [Core::DeltaTime]
		call WaitForNextFrame
		mov edi, 0x0090535f
		jmp edi
	}
}

void GiveHealth() {
	DR2::PlayerData* playerPtr = ((DR2::PlayerData**)0x00dede28)[0];
	if (!playerPtr) return;
	playerPtr->stats->health = playerPtr->stats->GetMaxHealth();
}

void GiveMoney() {
	DR2::PlayerData* playerPtr = ((DR2::PlayerData**)0x00dede28)[0];
	if (!playerPtr) return;
	playerPtr->stats->money = 999999999;
}

void GivePP() {
	DR2::PlayerData* playerPtr = ((DR2::PlayerData**)0x00dede28)[0];
	if (!playerPtr) return;
	playerPtr->stats->pp = 999999999;
}

DWORD CreateAffinityMask(int numCoresToUse) {
	int totalCores = std::thread::hardware_concurrency();

	if (numCoresToUse <= 0 || numCoresToUse > totalCores)
		numCoresToUse = totalCores;

	DWORD mask = 0;
	for (int i = 0; i < numCoresToUse; ++i) {
		mask |= (1ULL << (totalCores - 1 - i)); // Set the topmost cores
	}

	return mask;
}

HANDLE WINAPI CustomCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress,
	LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId)
{
	HANDLE hThread = CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
	if (hThread)
	{
		SetThreadAffinityMask(hThread, AffinityMask);
	}
	return hThread;
}

// From https://github.com/ThirteenAG/WidescreenFixesPack/pull/1045
// Forces threads spawned by the game to use less cores, in order to attempt to fix stability/crashing issues. Should perform better than just setting affinity on the process as a whole.

void RunFastAffinity(int coreAmount) {
	AffinityMask = CreateAffinityMask(coreAmount);

	HINSTANCE					hInstance = GetModuleHandle(nullptr);
	PIMAGE_NT_HEADERS			ntHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)hInstance + ((PIMAGE_DOS_HEADER)hInstance)->e_lfanew);
	PIMAGE_IMPORT_DESCRIPTOR	pImports = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD_PTR)hInstance + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	// Find KERNEL32.DLL
	for (; pImports->Name != 0; pImports++)
	{
		if (!_stricmp((const char*)((DWORD_PTR)hInstance + pImports->Name), "KERNEL32.DLL"))
		{
			if (pImports->OriginalFirstThunk != 0)
			{
				PIMAGE_IMPORT_BY_NAME* pFunctions = (PIMAGE_IMPORT_BY_NAME*)((DWORD_PTR)hInstance + pImports->OriginalFirstThunk);
				for (ptrdiff_t j = 0; pFunctions[j] != nullptr; j++)
				{
					if (!strcmp((const char*)((DWORD_PTR)hInstance + pFunctions[j]->Name), "CreateThread"))
					{
						// Overwrite the address with the address to a custom CreateThread
						DWORD dwProtect[2];
						DWORD_PTR* pAddress = &((DWORD_PTR*)((DWORD_PTR)hInstance + pImports->FirstThunk))[j];
						VirtualProtect(pAddress, sizeof(DWORD_PTR), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
						*pAddress = (DWORD_PTR)CustomCreateThread;
						VirtualProtect(pAddress, sizeof(DWORD_PTR), dwProtect[0], &dwProtect[1]);
						SetThreadAffinityMask(GetCurrentThread(), AffinityMask);
						break;
					}
				}
			}
		}
	}
}

bool IsDR2WindowTitle(std::wstring title) {
	return title.find(L"Dead Rising 2") != std::wstring::npos;
}

bool IsDR2Window(HWND hWnd) {
	wchar_t windowName[256] = { 0 };
	int length = GetWindowTextW(hWnd, windowName, sizeof(windowName) / sizeof(windowName[0]));
	
	if (length > 0) {
		std::wstring windowStr(windowName);
		return IsDR2WindowTitle(windowStr);
	}
	return false;
}

// hook tuah
void HookFramerate() {
	memcpy_s(Addresses::FPSLimit, sizeof(float), &Config::FPSDelta, sizeof(float));
	float* fpsMemLoc = &Config::CinematicFPS;
	float* deltaMemLoc = &Config::CinematicFPSDelta;
	Inject::WriteToMemory((DWORD)Addresses::CutsceneFPS, &fpsMemLoc, 4);
	Inject::WriteToMemory((DWORD)Addresses::CutsceneDelta, &deltaMemLoc, 4);
}

float targetDeltaTime = 0.033;

int __fastcall DetourUpdateSystems(void* me, void* _, float deltaTime) {
	if (Config::MaxPP)
		GivePP();
	if (Config::InfiniteMoney)
		GiveMoney();
	if (Config::GodMode)
		GiveHealth();
	Core::DeltaTime = deltaTime;
	Core::AdjustedDeltaTime = deltaTime / targetDeltaTime;
	return fpUpdateSystems(me, deltaTime);
}

bool __fastcall DetourDetermineAssignmentPushable(void* me, void* _, void* actor, DR2::tActorAssignment* assignment) {
	bool res = fpDetermineAssignmentPushable(me, actor, assignment);
	assignment->mPushableInfo.mRotAmt *= Core::AdjustedDeltaTime;
	return res;
}

void __declspec(naked) AmmoDepleteHook() {
	__asm {
		// calculate ammo deplete
		movss xmm1, [Core::AdjustedDeltaTime]
		mulss xmm0, xmm1
		movss dword ptr [esp + 0xc], xmm0
		// return to code
		mov eax, 0x0066ef63
		jmp eax
	}
}

void __cdecl DetourDebugPrint(int debugId, int verbosity, char* str, ...) {
	va_list args;
	va_start(args, str);
	vprintf(str, args);
	va_end(args);
}

BOOL WINAPI DetourSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) {
	if (IsDR2Window(hWnd) && Config::Borderless) {
		printf("Setting game window pos.\n");
		RECT desktopRect;
		GetWindowRect(GetDesktopWindow(), &desktopRect);
		return fpSetWindowPos(hWnd, hWndInsertAfter, desktopRect.left, desktopRect.top, desktopRect.right - desktopRect.left, desktopRect.bottom - desktopRect.top, uFlags);
	}
	return fpSetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

LONG WINAPI DetourSetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong) {
	if (IsDR2Window(hWnd) && nIndex == GWL_STYLE && Config::Borderless) {
		printf("Adjusting game window.\n");
		return fpSetWindowLongW(hWnd, nIndex, WS_POPUP | WS_VISIBLE);
	}
	return fpSetWindowLongW(hWnd, nIndex, dwNewLong);
}

HWND WINAPI DetourCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
	if (lpWindowName && IsDR2WindowTitle(lpWindowName) && Config::Borderless) {
		printf("Creating game window.\n");
		dwStyle = WS_POPUP;
	}
	HWND res = fpCreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	return res;
}

bool __stdcall OutfitUnlocked(Outfits outfit) {
	if (!Config::FixOutfitUnlocks) return false;

	ISteamUserStats* stats = SteamAPI::SteamUserStats();
	ISteamApps* apps = SteamAPI::SteamApps();

	if (stats)
		stats->RequestCurrentStats();

	if (outfit == Outfits::AlternateOutfit) {
		if (!apps) return false;
		// Check Dead Rising 2 is owned.
		return (apps->BIsSubscribedApp(45740));
	}
	else if (outfit == Outfits::ProtomanBody) {
		if (!stats) return false;
		bool achievedSPChallenges = false;
		bool achievedCoopChallenges = false;
		stats->GetAchievement("ACHIEVEMENT_16_THE_CHALLENGE_EXPERIENCE", &achievedSPChallenges);
		stats->GetAchievement("ACHIEVEMENT_17_MORE_HELP_FROM_MY_FRIENDS", &achievedCoopChallenges);
		if (achievedSPChallenges || achievedCoopChallenges) return true;
		return false;
	}
	return false;
}

DWORD OutfitJumpHookFalseTarget = 0x00878048;
DWORD OutfitJumpHookTrueTarget = 0x0087800E;

void __declspec(naked) OutfitJumpHook() {
	__asm {
	jne unlockedBranch
	push ebp
	push ebx
	push esi
	push edx
	push edi
	push [edi+0x4]
	//call here [edi+4] has outfit hash
	call OutfitUnlocked
	test al, al
	je lockedBranch
	pop edi
	pop edx
	pop esi
	pop ebx
	pop ebp
	unlockedBranch:
		mov edx,[edi]
		mov eax,[edx+0x78]
		mov ecx, OutfitJumpHookTrueTarget
		jmp ecx
	lockedBranch:
		pop edi
		pop edx
		pop esi
		pop ebx
		pop ebp
		mov ecx, OutfitJumpHookFalseTarget
		jmp ecx
	}
}

void EnableJumpMenu() {
	// enable_dev_features
	((bool*)0x00dec038)[0] = true;
	// enable_quickie_debug_menu
	((bool*)0x00dec093)[0] = true;
	// enable_dev_only_debug_tiwwchnt
	((bool*)0x00dec090)[0] = true;
	// enable_debug_jump_menu
	((bool*)0x00dec09b)[0] = true;
}

char autoAimPatch[] = { 0x39, 0xED };

void FixJumpAttackAutoAim() {
	Inject::WriteToMemory((DWORD)Addresses::AutoAimTestInstruction, autoAimPatch, 2);
}

void __stdcall DetourInitializeGame() {
	if (Config::FastAffinity > 0) {
		printf("Using %i cores for game logic.\n", Config::FastAffinity);
		RunFastAffinity(Config::FastAffinity);
	}

	fpInitializeGame();

	if (Config::DisableHeartbeat)
		((bool*)Addresses::online_disable_heartbeat)[0] = true;

	if (Config::SkipLogos)
		((bool*)Addresses::skip_logos)[0] = true;

	if (Config::GodMode)
		((bool*)Addresses::chuck_in_god_mode)[0] = true;

	if (Config::GhostMode)
		((bool*)Addresses::chuck_ghost_mode)[0] = true;

	if (Config::PlayAsChuck)
		((bool*)Addresses::frank_off)[0] = true;

	if (Config::Sprinting)
		((bool*)Addresses::enable_sprinting)[0] = true;

	if (Config::EverythingUnlocked)
		((bool*)Addresses::missions_everything_unlocked)[0] = true;

	if (Config::JumpMenu)
		EnableJumpMenu();

	if (Config::FixJumpAttackAutoAim)
		FixJumpAttackAutoAim();

	((bool*)Addresses::OverrideRenderSettings)[0] = true;
	//GameAddresses::Addresses["disable_initial_login_dialog"][0] = true;
	((float*)Addresses::online_normal_heart_beat)[0] = Config::Heartbeat;
	((float*)Addresses::online_extended_heart_beat)[0] = Config::ExtendedHeartbeat;

	if (Config::Windowed || Config::Borderless)
		((bool*)Addresses::RenderFullScreen)[0] = false;

	HookFramerate();
}

Core* Core::GetInstance() {
	return _instance;
}

bool Core::Create() {
	_instance = new Core();
	return _instance->Initialize();
}

IDirectInput8* pDirectInput = nullptr;

BOOL CALLBACK _DIEnumDevCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	printf("Found product name %s\n", lpddi->tszInstanceName);

	DIPROPGUIDANDPATH guidAndPath;
	guidAndPath.diph.dwSize = sizeof DIPROPGUIDANDPATH;
	guidAndPath.diph.dwHeaderSize = sizeof DIPROPHEADER;
	guidAndPath.diph.dwObj = 0;
	guidAndPath.diph.dwHow = DIPH_DEVICE;

	LPDIRECTINPUTDEVICE8 joystick;

	HRESULT hr = pDirectInput->CreateDevice(lpddi->guidInstance, &joystick, nullptr);
	if (FAILED(hr))
		return DIENUM_CONTINUE;
	hr = joystick->GetProperty(DIPROP_GUIDANDPATH, &guidAndPath.diph);
	if (FAILED(hr))
		return DIENUM_CONTINUE;
	bool isXInputDevice = wcsstr(guidAndPath.wszPath, L"IG_") != nullptr || wcsstr(guidAndPath.wszPath, L"ig_") != nullptr;

	printf("Product Path: %ws\n", guidAndPath.wszPath);

	DIPROPDWORD dipdw;
	dipdw.diph.dwSize = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj = 0; // Property applies to the entire device
	dipdw.diph.dwHow = DIPH_DEVICE;
	hr = joystick->GetProperty(DIPROP_VIDPID, &dipdw.diph);
	if (FAILED(hr))
		return DIENUM_CONTINUE;

	DWORD vidpid = dipdw.dwData;

	std::cout << "VIDPID: 0x" << std::hex << vidpid << std::endl;

	if (isXInputDevice)
	{
		printf("It's XInput!\n");
		Inject::WriteToMemory(0x00ac0073, &vidpid, 4);
		Inject::WriteToMemory(0x00ac0c77, &vidpid, 4);
		Inject::WriteToMemory(0x00ac0f37, &vidpid, 4);
		return DIENUM_CONTINUE;
	}
	else
	{
		printf("Not XInput.\n");
	}
	return DIENUM_CONTINUE;
}


void DetectController() {
	printf("Looking for XInput controller\n");
	HRESULT hr = DirectInput8Create(
		GetModuleHandle(nullptr),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(void**)&pDirectInput,
		nullptr
	);
	if (FAILED(hr)) {
		printf("Unable to create DirectInput8.\n");
		return;
	}

	pDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, _DIEnumDevCallback, 0, DIEDFL_ATTACHEDONLY);
}

bool timeResolutionSet = false;

void Core::Shutdown() {
	if (timeResolutionSet)
		timeEndPeriod(1);
}

bool Core::Initialize() {
	if (timeBeginPeriod(1) == TIMERR_NOERROR)
		timeResolutionSet = true;
	printf("Core initializing\n");
	Config::Load();

	if (Config::Console) {
		AllocConsole();
		freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
		freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
		freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
	}

	if (Config::FixControllerSupport)
		DetectController();

	// Initialize MinHook.
	if (MH_Initialize() != MH_OK)
		return false;

	if (MH_CreateHook(Addresses::UpdateSystems, &DetourUpdateSystems,
		reinterpret_cast<LPVOID*>(&fpUpdateSystems)) != MH_OK)
	{
		return false;
	}
	if (MH_EnableHook(Addresses::UpdateSystems) != MH_OK)
	{
		return false;
	}

	if (Config::FixFrameLimiter) {
		Inject::MakeJMP((BYTE*)0x00905299, (DWORD)FrameLimiterHook1, 6);
		Inject::MakeJMP((BYTE*)0x00905353, (DWORD)FrameLimiterHook2, 12);
	}

	if (Config::FixFramerateDependency) {
		Inject::MakeJMP((BYTE*)Addresses::AmmoDepleteInstruction, (DWORD)AmmoDepleteHook, 6);

		if (MH_CreateHook(Addresses::PushableDetermineAssignment, &DetourDetermineAssignmentPushable,
			reinterpret_cast<LPVOID*>(&fpDetermineAssignmentPushable)) != MH_OK)
		{
			return false;
		}
		if (MH_EnableHook(Addresses::PushableDetermineAssignment) != MH_OK)
		{
			return false;
		}
	}

	if (MH_CreateHook(Addresses::InitializeGame, &DetourInitializeGame,
		reinterpret_cast<LPVOID*>(&fpInitializeGame)) != MH_OK)
	{
		return false;
	}

	if (MH_CreateHook(&CreateWindowExW, &DetourCreateWindowExW,
		reinterpret_cast<LPVOID*>(&fpCreateWindowExW)) != MH_OK) 
	{
		return false;
	}

	if (MH_CreateHook(&SetWindowLongW, &DetourSetWindowLongW,
		reinterpret_cast<LPVOID*>(&fpSetWindowLongW)) != MH_OK)
	{
		return false;
	}

	if (MH_CreateHook(&SetWindowPos, &DetourSetWindowPos,
		reinterpret_cast<LPVOID*>(&fpSetWindowPos)) != MH_OK)
	{
		return false;
	}

	Inject::MakeJMP((BYTE*)0x0090527c, (DWORD)MainLoopHook, 5);

	if (Config::OutfitsUnlocked) {
		Inject::Nop((BYTE*)Addresses::OutfitUnlockJump, 2);
	}
	else {
		Inject::MakeJMP((BYTE*)Addresses::OutfitUnlockJump, (DWORD)OutfitJumpHook, 7);
	}

	if (Config::Debug) {
		if (MH_CreateHook(Addresses::DebugPrint, &DetourDebugPrint,
			reinterpret_cast<LPVOID*>(&fpDebugPrint)) != MH_OK)
		{
			return false;
		}
		if (MH_EnableHook(Addresses::DebugPrint) != MH_OK)
		{
			return false;
		}
	}

	if (MH_EnableHook(Addresses::InitializeGame) != MH_OK)
	{
		return false;
	}
	
	if (MH_EnableHook(&CreateWindowExW) != MH_OK)
	{
		return false;
	}

	if (MH_EnableHook(&SetWindowLongW) != MH_OK)
	{
		return false;
	}

	if (MH_EnableHook(&SetWindowPos) != MH_OK)
	{
		return false;
	}

	return true;
}