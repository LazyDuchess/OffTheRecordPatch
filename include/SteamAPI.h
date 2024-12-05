#pragma once

typedef unsigned int AppId_t;

class ISteamUserStats {
public:
	virtual bool RequestCurrentStats();
private:
	virtual void GetStat1();
	virtual void GetStat2();
	virtual void SetStat1();
	virtual void SetStat2();
	virtual void UpdateAvgRateStat();
public:
	virtual bool GetAchievement(const char *pchName, bool *pbAchieved);
};

class ISteamApps {
private:
	virtual bool BIsSubscribed();
	virtual bool BIsLowViolence();
	virtual bool BIsCybercafe();
	virtual bool BIsVACBanned();
	virtual const char* GetCurrentGameLanguage();
	virtual const char* GetAvailableGameLanguages();

public:
	virtual bool BIsSubscribedApp(AppId_t appID);
};

namespace SteamAPI {
	ISteamApps* SteamApps();
	ISteamUserStats* SteamUserStats();
	void Initialize();
}