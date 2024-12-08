#include "pch.h"
#include "GameAddresses.h"

std::map<std::string, char*> GameAddresses::Addresses;

bool GameAddresses::RegisterAddress(char* name, char* address) {
	if (address != nullptr) {
		printf("GameAddresses: Registering %s pointing to %p\n", name, address);
		Addresses[name] = address;
		return true;
	}
	printf("GameAddresses: Failed to find address for %s\n", name);
	return false;
}

bool GameAddresses::Initialize() {
	if (!RegisterAddress("skip_logos", (char*)0xDEC1D5)) return false;
	// Not really god mode, attacks don't do anything to you but you can still get killed in other ways like grabs and such.
	if (!RegisterAddress("chuck_in_god_mode", (char*)0x00dec0f3)) return false;
	// Ignored by enemies.
	if (!RegisterAddress("chuck_ghost_mode", (char*)0x00dec0f6)) return false;
	// Play as chuck.
	if (!RegisterAddress("frank_off", (char*)0x00dec036)) return false;
	// Dunno.
	if (!RegisterAddress("special_chuck", (char*)0x00dec037)) return false;
	// Might allow some features to be enabled?
	if (!RegisterAddress("enable_dev_features", (char*)0x00dec038)) return false;
	// nothing?
	if (!RegisterAddress("RenderPerformanceInfo", (char*)0x00dec03a)) return false;
	// Allows you to override settings like fullscreen.
	if (!RegisterAddress("OverrideRenderSettings", (char*)0x00dec043)) return false;
	if (!RegisterAddress("RenderFullScreen", (char*)0x00dec044)) return false;
	// nothing?
	if (!RegisterAddress("RENDER.forcesoftware", (char*)0x00dec04a)) return false;
	// Crashes...
	if (!RegisterAddress("skip_fe", (char*)0x00dec080)) return false;
	if (!RegisterAddress("skip_fe_autoload_sandbox", (char*)0x00dec083)) return false;
	if (!RegisterAddress("skip_fe_autoload", (char*)0x00dec082)) return false;
	// nothing?
	if (!RegisterAddress("display_fe_screen_info", (char*)0x00dec085)) return false;
	// black screen on start.
	if (!RegisterAddress("enable_e3_demo_experience", (char*)0x00dec08d)) return false;
	// nothing?
	if (!RegisterAddress("double_players_run_speed", (char*)0x00dec0c4)) return false;
	// sprint when unarmed.
	if (!RegisterAddress("enable_sprinting", (char*)0x00dec0c5)) return false;
	// nothing?
	if (!RegisterAddress("super_test_mode", (char*)0x00dec0cf)) return false;
	// unlocks everything
	if (!RegisterAddress("missions_everything_unlocked", (char*)0x00dec170)) return false;
	// nothing?
	if (!RegisterAddress("RENDER.wii_lowgraphics", (char*)0x00e73a1c)) return false;
	if (!RegisterAddress("suspend_on_lost_focus", (char*)0x00dec19f)) return false;
	if (!RegisterAddress("InitializeGame", (char*)0x007e7040)) return false;
	if (!RegisterAddress("InitializeController", (char*)0x00ac0580)) return false;
	if (!RegisterAddress("gController", (char*)0x00e180b0)) return false;
	if (!RegisterAddress("ControllerInputDevice", (char*)0x00ac00b0)) return false;
	if (!RegisterAddress("GetInputs", (char*)0x00abba60)) return false;
	if (!RegisterAddress("FPSLimit", (char*)0x00D6EE70)) return false;
	if (!RegisterAddress("CutsceneFPS", (char*)0x007CD692)) return false;
	if (!RegisterAddress("CutsceneDelta", (char*)0x007CD6A6)) return false;
	if (!RegisterAddress("ControllerCheck", (char*)0x00ac0c83)) return false;
	if (!RegisterAddress("ControllerCheckJumpTarget", (char*)0x00ac0c93)) return false;
	if (!RegisterAddress("DebugPrint", (char*)0x00ab75b0)) return false;
	// [edi+4] has outfit hash
	if (!RegisterAddress("OutfitUnlockJump", (char*)0x00878007)) return false;
	if (!RegisterAddress("UpdateNetworking", (char*)0x008c9d60)) return false;
	if (!RegisterAddress("online_disable_heartbeat", (char*)0x00dec1a9)) return false;
	if (!RegisterAddress("disable_initial_login_dialog", (char*)0x00dec09c)) return false;
	if (!RegisterAddress("online_normal_heart_beat", (char*)0x00debed8)) return false;
	if (!RegisterAddress("online_extended_heart_beat", (char*)0x00debedc)) return false;
	if (!RegisterAddress("SteamSocketSendDataHook", (char*)0x0091104b)) return false;
	if (!RegisterAddress("OnlineMaxSentRatePtr", (char*)0x008b2d41)) return false;
	if (!RegisterAddress("OnlineRatePtr", (char*)(0x008b2cfb + 4))) return false;
	if (!RegisterAddress("Online40RatePtr", (char*)(0x008b2d2d + 4))) return false;
	if (!RegisterAddress("Online100RatePtr", (char*)(0x008b2d4d + 4))) return false;
	if (!RegisterAddress("AutoAimTestInstruction", (char*)(0x006AD60F))) return false;
	return true;
}