#include "Player.h"

PlayerData* DR2::GetLocalPlayer() {
	return ((PlayerData**)0x00dede28)[0];
}