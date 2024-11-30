#pragma once
#include "ini.h"

class Core {
public:
	bool Initialize();
	static bool Create();
	static Core* GetInstance();
	static mINI::INIStructure Ini;
	static float CutsceneFPS;
	static float CutsceneDelta;
	static float GameDelta;
	static bool Borderless;
	static bool Windowed;
	static int FastAffinity;
private:
	static Core* _instance;
};