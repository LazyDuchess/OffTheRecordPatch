#include "pch.h"
#include "Core.h"
#include "iostream"
#include "MinHook.h"
#include "GameAddresses.h"
#include <tlhelp32.h>
#include "inject.h"

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

// hook tuah
void HookFramerate() {
	memcpy_s(GameAddresses::Addresses["FPSLimit"], sizeof(float), &Core::GameDelta, sizeof(float));
	float* fpsMemLoc = &Core::CutsceneFPS;
	float* deltaMemLoc = &Core::CutsceneDelta;
	Inject::WriteToMemory((DWORD)GameAddresses::Addresses["CutsceneFPS"], &fpsMemLoc, 4);
	Inject::WriteToMemory((DWORD)GameAddresses::Addresses["CutsceneDelta"], &deltaMemLoc, 4);
}

BOOL WINAPI DetourSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) {
	wchar_t windowName[256];
	int length = GetWindowTextW(hWnd, windowName, sizeof(windowName) / sizeof(windowName[0]));
	if (windowName && wcscmp(windowName, L"Dead Rising 2: Off The Record") == 0 && Core::Borderless) {
		printf("Setting game window pos.\n");
		RECT desktopRect;
		GetWindowRect(GetDesktopWindow(), &desktopRect);
		return fpSetWindowPos(hWnd, hWndInsertAfter, desktopRect.left, desktopRect.top, desktopRect.right - desktopRect.left, desktopRect.bottom - desktopRect.top, uFlags);
	}
	return fpSetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

LONG WINAPI DetourSetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong) {
	wchar_t windowName[256];
	int length = GetWindowTextW(hWnd, windowName, sizeof(windowName) / sizeof(windowName[0]));
	if (windowName && wcscmp(windowName, L"Dead Rising 2: Off The Record") == 0 && nIndex == GWL_STYLE && Core::Borderless) {
		printf("Adjusting game window.\n");
		return fpSetWindowLongW(hWnd, nIndex, WS_POPUP | WS_VISIBLE);
	}
	return fpSetWindowLongW(hWnd, nIndex, dwNewLong);
}

HWND WINAPI DetourCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
	if (lpWindowName && wcscmp(lpWindowName, L"Dead Rising 2: Off The Record") == 0 && Core::Borderless) {
		printf("Creating game window.\n");
		dwStyle = WS_POPUP;
	}
	HWND res = fpCreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	return res;
}

void __stdcall DetourInitializeGame() {
	
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

	if (Core::Ini["Display"]["Windowed"] == "true" || Core::Ini["Display"]["Borderless"] == "true")
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

bool Core::Initialize() {
	printf("Core initializing\n");

	mINI::INIFile file("OffTheRecordPatch.ini");
	file.read(Ini);

	Core::Borderless = Ini["Display"]["Borderless"] == "true";

	if (Ini["General"]["Console"] == "true") {
		AllocConsole();
		freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
		freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
		freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
	}

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