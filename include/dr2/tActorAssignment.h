#pragma once
#include "tPushableGoalInfo.h"

namespace DR2 {
	class tActorAssignment {
	private:
		char pad[1052];
	public:
		tPushableGoalInfo mPushableInfo;
	};
}