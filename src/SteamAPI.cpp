#include "pch.h"
#include "SteamAPI.h"

typedef ISteamUserStats* (__stdcall* STEAMUSERSTATS)();
typedef ISteamApps* (__stdcall* STEAMAPPS)();
STEAMUSERSTATS fnSteamUserStats = NULL;
STEAMAPPS fnSteamApps = NULL;

void SteamAPI::Initialize() {
	HMODULE hSteamAPI = GetModuleHandleA("steam_api.dll");
	if (!hSteamAPI) return;
	fnSteamUserStats = (STEAMUSERSTATS)GetProcAddress(hSteamAPI, "SteamUserStats");
	fnSteamApps = (STEAMAPPS)GetProcAddress(hSteamAPI, "SteamApps");
}

ISteamUserStats* SteamAPI::SteamUserStats() {
	if (fnSteamUserStats == NULL)
		return NULL;
	return fnSteamUserStats();
}

ISteamApps* SteamAPI::SteamApps() {
	if (fnSteamApps == NULL)
		return NULL;
	return fnSteamApps();
}