#include "pch.h"
#include "Core.h"
#include "iostream"
#include "MinHook.h"
#include "GameAddresses.h"
#include <tlhelp32.h>

typedef void(__stdcall* INITIALIZEGAME)();

INITIALIZEGAME fpInitializeGame = NULL;

// hook tuah
void __stdcall DetourInitializeGame() {
	
	fpInitializeGame();
	GameAddresses::Addresses["skip_logos"][0] = true;
	GameAddresses::Addresses["chuck_in_god_mode"][0] = true;
	GameAddresses::Addresses["chuck_ghost_mode"][0] = true;
	//GameAddresses::Addresses["frank_off"][0] = true;
	GameAddresses::Addresses["OverrideRenderSettings"][0] = true;
	GameAddresses::Addresses["RenderFullScreen"][0] = false;
	GameAddresses::Addresses["enable_sprinting"][0] = true;
	GameAddresses::Addresses["missions_everything_unlocked"][0] = true;
	
}

Core* Core::_instance = nullptr;

Core* Core::GetInstance() {
	return _instance;
}

bool Core::Create() {
	_instance = new Core();
	return _instance->Initialize();
}

bool Core::Initialize() {
	printf("Core initializing\n");

	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

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