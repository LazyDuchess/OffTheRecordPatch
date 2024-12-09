namespace DR2 {
	class PlayerStats {
	private:
		char pad[48];
	public:
		int level;
		int pp;
	private:
		char pad2[4];
	public:
		float health;
	private:
		char pad3[8];
	public:
		int zombieskilled;
	private:
		char pad4[32];
	public:
		int healthBars;
	private:
		char pad5[16];
	public:
		int money;
		float GetMaxHealth() {
			return (float)healthBars * 100.0;
		}
	};

	class PlayerData {
	private:
		char pad[0x8];
	public:
		PlayerStats* stats;
	};

	PlayerData* GetLocalPlayer();
}