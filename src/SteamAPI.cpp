#include "pch.h"
#include "SteamAPI.h"

ISteamUserStats* SteamAPI::SteamUserStats() {
	DWORD* funcAddr = (DWORD*)0x00c58490;
	return ((ISteamUserStats* (__stdcall*)())funcAddr[0])();
}

ISteamApps* SteamAPI::SteamApps() {
	DWORD* funcAddr = (DWORD*)0x00c5849c;
	return ((ISteamApps* (__stdcall*)())funcAddr[0])();
}