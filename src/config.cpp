#include "config.h"
#include "ini.h"
#include <string>
#include "Logging.h"

namespace Config {

	static const char* ConfigFilename = "OffTheRecordPatch.ini";
	static mINI::INIStructure Ini;
	bool Console;
	bool SkipLogos;

	bool Windowed;
	bool Borderless;
	float FPSLimit;
	float CinematicFPS;
	bool UnlockFPSDuringLoading;

	int FastAffinity;
	bool Debug;

	bool PlayAsChuck;
	bool GodMode;
	bool GhostMode;
	bool Sprinting;
	bool EverythingUnlocked;
	bool OutfitsUnlocked;
	bool JumpMenu;
	bool InfiniteMoney;
	bool MaxPP;

	bool FixOutfitUnlocks;
	bool FixControllerSupport;
	bool FixJumpAttackAutoAim;
	bool FixFramerateDependency;
	bool FixFrameLimiter;

	float Heartbeat;
	float ExtendedHeartbeat;
	bool DisableHeartbeat;

	float FPSDelta;
	float CinematicFPSDelta;

	static bool Has(const std::string& section, const std::string& key) {
		if (!Ini[section].has(key)) return false;
		return true;
	}

	static std::string GetString(const std::string& section, const std::string& key, std::string defaultValue) {
		if (!Has(section, key)) {
			Ini[section][key] = defaultValue;
			return defaultValue;
		}
		return Ini[section][key];
	}

	static int GetInt(const std::string& section, const std::string& key, int defaultValue) {
		if (!Has(section, key)) {
			Ini[section][key] = std::to_string(defaultValue);
			return defaultValue;
		}
		return std::stoi(Ini[section][key]);
	}

	static float GetFloat(const std::string& section, const std::string& key, float defaultValue) {
		if (!Has(section, key)) {
			Ini[section][key] = std::to_string(defaultValue);
			return defaultValue;
		}
		return std::stof(Ini[section][key]);
	}

	static bool GetBool(const std::string& section, const std::string& key, bool defaultValue) {
		std::string value = GetString(section, key, defaultValue == false ? "false" : "true");
		if (value == "true" || value == "1") return true;
		return false;
	}

	void Load() {
		bool iniExisted = false;
		mINI::INIFile file(ConfigFilename);

		Log("Reading config from %s.\n", ConfigFilename);

		if (file.read(Ini))
			iniExisted = true;

		Console = GetBool("General", "Console", false);
		SkipLogos = GetBool("General", "SkipLogos", true);

		Windowed = GetBool("Display", "Windowed", false);
		Borderless = GetBool("Display", "Borderless", true);
		FPSLimit = GetFloat("Display", "FPSLimit", 120.0);
		CinematicFPS = GetFloat("Display", "CinematicFPS", 120.0);
		UnlockFPSDuringLoading = GetBool("Display", "UnlockFPSDuringLoading", true);

		FastAffinity = GetInt("Advanced", "FastAffinity", 2);
		Debug = GetBool("Advanced", "Debug", false);

		PlayAsChuck = GetBool("Cheats", "PlayAsChuck", false);
		GodMode = GetBool("Cheats", "GodMode", false);
		GhostMode = GetBool("Cheats", "GhostMode", false);
		Sprinting = GetBool("Cheats", "Sprinting", false);
		EverythingUnlocked = GetBool("Cheats", "EverythingUnlocked", false);
		OutfitsUnlocked = GetBool("Cheats", "OutfitsUnlocked", false);
		JumpMenu = GetBool("Cheats", "JumpMenu", false);
		InfiniteMoney = GetBool("Cheats", "InfiniteMoney", false);
		MaxPP = GetBool("Cheats", "MaxPP", false);

		FixOutfitUnlocks = GetBool("Fixes", "FixOutfitUnlocks", true);
		FixControllerSupport = GetBool("Fixes", "FixControllerSupport", true);
		FixJumpAttackAutoAim = GetBool("Fixes", "FixJumpAttackAutoAim", true);
		FixFramerateDependency = GetBool("Fixes", "FixFramerateDependency", true);
		FixFrameLimiter = GetBool("Fixes", "FixFrameLimiter", true);

		Heartbeat = GetFloat("Online", "Heartbeat", 8.0);
		ExtendedHeartbeat = GetFloat("Online", "ExtendedHeartbeat", 30.0);
		DisableHeartbeat = GetBool("Online", "DisableHeartbeat", false);

		if (FPSLimit > 0)
			FPSDelta = 1.0 / FPSLimit;
		else
			FPSDelta = 0.0;

		if (CinematicFPS > 0)
			CinematicFPSDelta = 1.0 / CinematicFPS;
		else
		{
			Log("Cinematic FPS can't be 0 or less - setting to 120.\n");
			CinematicFPS = 120.0;
			CinematicFPSDelta = 1.0 / CinematicFPS;
		}

		if (!iniExisted) {
			Log("Default config is being written to %s cause it didn't exist.\n", ConfigFilename);
			file.generate(Ini, true);
		}
	}
}