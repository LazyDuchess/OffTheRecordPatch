#pragma once
#include "ini.h"

class Core {
public:
	bool Initialize();
	void Shutdown();
	static bool Create();
	static Core* GetInstance();
	static mINI::INIStructure Ini;
	static float CutsceneFPS;
	static float CutsceneDelta;
	static float GameDelta;
	static bool Borderless;
	static bool Windowed;
	static bool FixOutfitUnlocks;
	static int FastAffinity;
	static float DeltaTime;
	static float AdjustedDeltaTime;
	static bool MoneyCheat;
	static bool PPCheat;
	static bool GodMode;
	static bool UnlockFPSDuringLoading;
private:
	static Core* _instance;
};