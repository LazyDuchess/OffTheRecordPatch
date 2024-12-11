#pragma once
#define ADDRESS(name, address) void* name = (void*)address

namespace Addresses {
	ADDRESS(FPSLimit, 0x00D6EE70);
	ADDRESS(CutsceneFPS, 0x007CD692);
	ADDRESS(CutsceneDelta, 0x007CD6A6);
	ADDRESS(AutoAimTestInstruction, 0x006AD60F);
	ADDRESS(online_disable_heartbeat, 0x00dec1a9);
	ADDRESS(skip_logos, 0xDEC1D5);
	ADDRESS(chuck_in_god_mode, 0x00dec0f3);
	ADDRESS(chuck_ghost_mode, 0x00dec0f6);
	ADDRESS(frank_off, 0x00dec036);
	ADDRESS(enable_sprinting, 0x00dec0c5);
	ADDRESS(missions_everything_unlocked, 0x00dec170);
	ADDRESS(OverrideRenderSettings, 0x00dec043);
	ADDRESS(online_normal_heart_beat, 0x00debed8);
	ADDRESS(online_extended_heart_beat, 0x00debedc);
	ADDRESS(RenderFullScreen, 0x00dec044);
	ADDRESS(UpdateSystems, 0x007e61a0);
	ADDRESS(AmmoDepleteInstruction, 0x0066ef5d);
	ADDRESS(PushableDetermineAssignment, 0x004e73f0);
	ADDRESS(InitializeGame, 0x007e7040);
	ADDRESS(OutfitUnlockJump, 0x00878007);
	ADDRESS(DebugPrint, 0x00ab75b0);
}

#undef ADDRESS