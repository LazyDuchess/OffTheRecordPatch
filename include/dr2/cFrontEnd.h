#pragma once

namespace DR2 {
	class cFrontEnd {
	public:
		static cFrontEnd* GetInstance();
		bool IsLoadingScreenActive();
	private:
		char pad[0x10];
	public:
		void* ScreenManager;
	};
}