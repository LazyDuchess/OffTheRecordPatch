#pragma once

class Core {
public:
	bool Initialize();
	void Shutdown();
	static bool Create();
	static Core* GetInstance();
	static float DeltaTime;
	static float AdjustedDeltaTime;
private:
	static Core* _instance;
};