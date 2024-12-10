#include "dr2/cFrontEnd.h"

DR2::cFrontEnd* DR2::cFrontEnd::GetInstance() {
	return *(DR2::cFrontEnd**)0x00eaf200;
}

bool DR2::cFrontEnd::IsLoadingScreenActive() {
	return ((bool(__thiscall*)(DR2::cFrontEnd*))0x00af8bd0)(this);
}