/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GUILDJOB_H
#define GAME_SERVER_GUILDJOB_H

#include <game/server/mmocore/GameEntities/decoration_houses.h>
#include "../MmoComponent.h"

#include <string>
#include <tuple>

/* TODO:
	Need refactoring it in two parts, the system and the game part
*/
class GuildDoor;
class GuildJob : public MmoComponent
{
	~GuildJob()
	{
		ms_aGuild.clear();
		ms_aHouseGuild.clear();
		ms_aRankGuild.clear();
	};

	struct GuildStruct
	{
		enum
		{
			AVAILABLE_SLOTS = 0,
			CHAIR_EXPERIENCE = 1,
			NUM_GUILD_UPGRADES,
		};
		GuildStruct()
		{
			m_Upgrades[AVAILABLE_SLOTS] = { "Available slots", "AvailableSlots", 0 };
			m_Upgrades[CHAIR_EXPERIENCE] = { "Chair experience", "ChairExperience", 0 };
		}
		struct
		{
			char m_aName[64];
			char m_aFieldName[64];
			int m_Value;
		} m_Upgrades[NUM_GUILD_UPGRADES];

		char m_aName[32];
		int m_Level;
		int m_Exp;
		int m_OwnerID;
		int m_Bank;
		int m_Score;

	};
	
	struct GuildStructHouse
	{
		int m_PosX;
		int m_PosY;
		int m_DoorX;
		int m_DoorY;
		int m_TextX;
		int m_TextY;
		int m_WorldID;
		int m_Price;
		int m_Payment;
		int m_GuildID;
		GuildDoor *m_pDoor;
	};

	struct GuildStructRank
	{
		char m_aRank[32];
		int m_GuildID;
		int m_Access;
	};

	static std::map < int, GuildStruct > ms_aGuild;
	static std::map < int, GuildStructHouse > ms_aHouseGuild;
	static std::map < int, GuildStructRank > ms_aRankGuild;
	std::map < int, CDecorationHouses* > m_DecorationHouse;

	void OnInit() override;
	void OnInitWorld(const char* pWhereLocalWorld) override;
	void OnTick() override;
	bool OnHandleTile(CCharacter* pChr, int IndexCollision) override;
	bool OnHandleVoteCommands(CPlayer* pPlayer, const char* CMD, const int VoteID, const int VoteID2, int Get, const char* GetText) override;
	bool OnHandleMenulist(CPlayer* pPlayer, int Menulist, bool ReplaceMenu) override;

private:
	void LoadGuildRank(int GuildID);
	void TickHousingText();

public:
	int SearchGuildByName(const char* pGuildName) const;

	const char *GuildName(int GuildID) const;
	int GetMemberAccess(CPlayer *pPlayer) const;
	bool CheckMemberAccess(CPlayer *pPlayer, int Access = GuildAccess::ACCESS_LEADER) const;
	int GetMemberChairBonus(int GuildID, int Field) const;

	void CreateGuild(CPlayer *pPlayer, const char *pGuildName);
	void DisbandGuild(int GuildID);
	bool JoinGuild(int AccountID, int GuildID);
	void ExitGuild(int AccountID);

private:
	void ShowMenuGuild(CPlayer *pPlayer);
	void ShowGuildPlayers(CPlayer *pPlayer, int GuildID);

public:
	void AddExperience(int GuildID);	
	bool AddMoneyBank(int GuildID, int Money);
	bool RemoveMoneyBank(int GuildID, int Money);
	bool UpgradeGuild(int GuildID, int Field);
	bool AddDecorationHouse(int DecoID, int GuildID, vec2 Position);

private:
	bool DeleteDecorationHouse(int ID);
	void ShowDecorationList(CPlayer* pPlayer);

public:
	const char *AccessNames(int Access);
	const char *GetGuildRank(int GuildID, int RankID);
	int FindGuildRank(int GuildID, const char *Rank) const;

private:
	void AddRank(int GuildID, const char *Rank);
	void DeleteRank(int RankID, int GuildID);
	void ChangeRank(int RankID, int GuildID, const char *NewRank);
	void ChangeRankAccess(int RankID);
	void ChangePlayerRank(int AccountID, int RankID);
	void ShowMenuRank(CPlayer *pPlayer);

public:
	int GetGuildPlayerCount(int GuildID);

private:
	void ShowInvitesGuilds(int ClientID, int GuildID);
	void ShowFinderGuilds(int ClientID);
	void SendInviteGuild(int GuildID, CPlayer* pPlayer);

	void ShowHistoryGuild(int ClientID, int GuildID);
	void AddHistoryGuild(int GuildID, const char *Buffer, ...);

public:
	int GetHouseGuildID(int HouseID) const;
	int GetHouseWorldID(int HouseID) const;
	int GetPosHouseID(vec2 Pos) const;

	bool GetGuildDoor(int GuildID) const;
	vec2 GetPositionHouse(int GuildID) const;
	int GetGuildHouseID(int GuildID) const;

	void BuyGuildHouse(int GuildID, int HouseID);
	void SellGuildHouse(int GuildID);
	void ShowBuyHouse(CPlayer *pPlayer, int HouseID);
	bool ChangeStateDoor(int GuildID);
};

class GuildDoor : public CEntity
{
	int m_GuildID;
public:
	GuildDoor(CGameWorld *pGameWorld, vec2 Pos, int HouseID);
	~GuildDoor();

	virtual void Tick();
	virtual void Snap(int SnappingClient);
};

#endif
 