#include "pch.h"
#include "Core.h"
#include "iostream"
#include "MinHook.h"
#include "GameAddresses.h"
#include <tlhelp32.h>
#include "inject.h"

typedef void(__stdcall* INITIALIZEGAME)();

INITIALIZEGAME fpInitializeGame = NULL;

mINI::INIStructure Core::Ini;
Core* Core::_instance = nullptr;
float Core::CutsceneFPS = 30.0;
float Core::CutsceneDelta = 1.0 / 30.0;
float Core::GameDelta = 1.0 / 120.0;

// hook tuah
void HookFramerate() {
	memcpy_s(GameAddresses::Addresses["FPSLimit"], sizeof(float), &Core::GameDelta, sizeof(float));
	float* fpsMemLoc = &Core::CutsceneFPS;
	float* deltaMemLoc = &Core::CutsceneDelta;
	Inject::WriteToMemory((DWORD)GameAddresses::Addresses["CutsceneFPS"], &fpsMemLoc, 4);
	Inject::WriteToMemory((DWORD)GameAddresses::Addresses["CutsceneDelta"], &deltaMemLoc, 4);
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

	if (MH_EnableHook((void*)GameAddresses::Addresses["InitializeGame"]) != MH_OK)
	{
		return false;
	}

	return true;
}