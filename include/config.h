#pragma once

namespace Config {
	void Load();

	extern bool Console;
	extern bool SkipLogos;

	extern bool Windowed;
	extern bool Borderless;
	extern float FPSLimit;
	extern float CinematicFPS;
	extern bool UnlockFPSDuringLoading;

	extern int FastAffinity;
	extern bool Debug;

	extern bool PlayAsChuck;
	extern bool GodMode;
	extern bool GhostMode;
	extern bool Sprinting;
	extern bool EverythingUnlocked;
	extern bool OutfitsUnlocked;
	extern bool JumpMenu;
	extern bool InfiniteMoney;
	extern bool MaxPP;

	extern bool FixOutfitUnlocks;
	extern bool FixControllerSupport;
	extern bool FixJumpAttackAutoAim;
	extern bool FixFramerateDependency;
	extern bool FixFrameLimiter;

	extern float Heartbeat;
	extern float ExtendedHeartbeat;
	extern bool DisableHeartbeat;

	extern float FPSDelta;
	extern float CinematicFPSDelta;
}