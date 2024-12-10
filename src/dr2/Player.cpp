#include "dr2/Player.h"

DR2::PlayerData* DR2::GetLocalPlayer() {
	return ((PlayerData**)0x00dede28)[0];
}