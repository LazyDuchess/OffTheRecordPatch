#include "pch.h"
#include "Core.h"
#include "iostream"
#include "MinHook.h"
#include "GameAddresses.h"
#include <tlhelp32.h>
#include "inject.h"
#include <thread>
#include <Xinput.h>

typedef BOOL (WINAPI* SETWINDOWPOS)(HWND, HWND, int, int, int, int, UINT);
typedef LONG (WINAPI* SETWINDOWLONGW)(HWND, int, LONG);
typedef HWND (WINAPI* CREATEWINDOWEXW)(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
typedef void(__stdcall* INITIALIZEGAME)();

INITIALIZEGAME fpInitializeGame = NULL;
CREATEWINDOWEXW fpCreateWindowExW = NULL;
SETWINDOWLONGW fpSetWindowLongW = NULL;
SETWINDOWPOS fpSetWindowPos = NULL;

mINI::INIStructure Core::Ini;
Core* Core::_instance = nullptr;
float Core::CutsceneFPS = 30.0;
float Core::CutsceneDelta = 1.0 / 30.0;
float Core::GameDelta = 1.0 / 120.0;
bool Core::Borderless = false;
bool Core::Windowed = false;
int Core::FastAffinity = 0;

static DWORD AffinityMask = 1;

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
	memcpy_s(GameAddresses::Addresses["FPSLimit"], sizeof(float), &Core::GameDelta, sizeof(float));
	float* fpsMemLoc = &Core::CutsceneFPS;
	float* deltaMemLoc = &Core::CutsceneDelta;
	Inject::WriteToMemory((DWORD)GameAddresses::Addresses["CutsceneFPS"], &fpsMemLoc, 4);
	Inject::WriteToMemory((DWORD)GameAddresses::Addresses["CutsceneDelta"], &deltaMemLoc, 4);
}

BOOL WINAPI DetourSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) {
	if (IsDR2Window(hWnd) && Core::Borderless) {
		printf("Setting game window pos.\n");
		RECT desktopRect;
		GetWindowRect(GetDesktopWindow(), &desktopRect);
		return fpSetWindowPos(hWnd, hWndInsertAfter, desktopRect.left, desktopRect.top, desktopRect.right - desktopRect.left, desktopRect.bottom - desktopRect.top, uFlags);
	}
	return fpSetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

LONG WINAPI DetourSetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong) {
	if (IsDR2Window(hWnd) && nIndex == GWL_STYLE && Core::Borderless) {
		printf("Adjusting game window.\n");
		return fpSetWindowLongW(hWnd, nIndex, WS_POPUP | WS_VISIBLE);
	}
	return fpSetWindowLongW(hWnd, nIndex, dwNewLong);
}

HWND WINAPI DetourCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
	if (lpWindowName && IsDR2WindowTitle(lpWindowName) && Core::Borderless) {
		printf("Creating game window.\n");
		dwStyle = WS_POPUP;
	}
	HWND res = fpCreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	return res;
}

void __stdcall DetourInitializeGame() {
	
	if (Core::FastAffinity > 0) {
		printf("Using %i cores for game logic.\n", Core::FastAffinity);
		RunFastAffinity(Core::FastAffinity);
	}

	fpInitializeGame();

	if (Core::Ini["General"]["SkipLogos"] == "true")
		GameAddresses::Addresses["skip_logos"][0] = true;

	if (Core::Ini["Cheats"]["GodMode"] == "true")
		GameAddresses::Addresses["chuck_in_god_mode"][0] = true;

	if (Core::Ini["Cheats"]["GhostMode"] == "true")
		GameAddresses::Addresses["chuck_ghost_mode"][0] = true;

	if (Core::Ini["Cheats"]["PlayAsChuck"] == "true")
		GameAddresses::Addresses["frank_off"][0] = true;

	if (Core::Ini["Cheats"]["Sprinting"] == "true")
		GameAddresses::Addresses["enable_sprinting"][0] = true;

	if (Core::Ini["Cheats"]["EverythingUnlocked"] == "true")
		GameAddresses::Addresses["missions_everything_unlocked"][0] = true;

	GameAddresses::Addresses["OverrideRenderSettings"][0] = true;

	if (Core::Windowed || Core::Borderless)
		GameAddresses::Addresses["RenderFullScreen"][0] = false;

	HookFramerate();
}

Core* Core::GetInstance() {
	return _instance;
}

bool Core::Create() {
	_instance = new Core();
	return _instance->Initialize();
}

bool IsControllerConnected(int index) {
	XINPUT_STATE state;
	ZeroMemory(&state, sizeof(XINPUT_STATE));
	DWORD result = XInputGetState(index, &state);

	if (result == ERROR_SUCCESS)
		return true;
	else
		return false;
}

void __declspec(naked) ControllerHook() {
	__asm {
		and eax, 0x0000FFFF
		cmp eax, 0x0000045E
		mov eax, 0x00AC0C7B
		jmp eax
	}
}

bool Core::Initialize() {
	printf("Core initializing\n");

	mINI::INIFile file("OffTheRecordPatch.ini");
	file.read(Ini);

	if (Ini["General"]["Console"] == "true") {
		AllocConsole();
		freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
		freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
		freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
	}

	bool hasController = false;

	printf("Looking for XInput controller\n");
	for (int i = 0; i < 4; i++) {
		if (IsControllerConnected(i)) {
			printf("Found valid XInput controller!\n");
			hasController = true;
			break;
		}
	}
	
	if (hasController) {
		Inject::MakeJMP((BYTE*)0x00AC0C76, (DWORD)ControllerHook, 5);
	}

	Core::Borderless = Ini["Display"]["Borderless"] == "true";
	Core::Windowed = Ini["Display"]["Windowed"] == "true";
	Core::FastAffinity = std::stoi(Ini["Advanced"]["FastAffinity"]);

	float fpsCap = std::stof(Ini["Display"]["FPSLimit"]);
	float cutCap = std::stof(Ini["Display"]["CinematicFPS"]);

	if (fpsCap <= 0.0)
		GameDelta = 0.0;
	else
		GameDelta = 1.0 / fpsCap;

	if (cutCap <= 0.0)
		cutCap = 30.0;

	CutsceneFPS = cutCap;
	CutsceneDelta = 1.0 / cutCap;

	if (!GameAddresses::Initialize())
		return false;

	// Initialize MinHook.
	if (MH_Initialize() != MH_OK)
		return false;

	if (MH_CreateHook((void*)GameAddresses::Addresses["InitializeGame"], &DetourInitializeGame,
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

	if (MH_EnableHook((void*)GameAddresses::Addresses["InitializeGame"]) != MH_OK)
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