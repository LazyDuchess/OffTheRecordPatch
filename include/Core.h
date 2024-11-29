#pragma once

class Core {
public:
	bool Initialize();
	static bool Create();
	static Core* GetInstance();
private:
	static Core* _instance;
};

class Dummy {
public:
	int vtable;
	char pad[0xc4a0];
};