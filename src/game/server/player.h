/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_PLAYER_H
#define GAME_SERVER_PLAYER_H

#include "mmocore/ComponentsCore/AccountJob/AccountMainJob.h"
#include "mmocore/ComponentsCore/BotJob.h"

#include "mmocore/ComponentsCore/InventoryJob/Item.h"
#include "mmocore/ComponentsCore/SkillsJob/Skill.h"
#include "mmocore/ComponentsCore/QuestsJob/PlayerQuests.h"

#include "entities/character.h"
#include <game/voting.h>

enum
{
	WEAPON_SELF = -2, // self die
	WEAPON_WORLD = -1, // swap world etc
};

class CPlayer
{
	MACRO_ALLOC_POOL_ID()

	struct StructLatency
	{
		int m_AccumMin;
		int m_AccumMax;
		int m_Min;
		int m_Max;
	};

	struct StructLastAction
	{
		int m_TargetX;
		int m_TargetY;
	};

	struct StructDialogNPC
	{
		int m_TalkedID;
		int m_Progress;
		bool m_FreezedProgress;
	};
	StructDialogNPC m_DialogNPC;
	char m_aFormatDialogText[512];
	std::map < int, bool > m_aHiddenMenu;

protected:
	CCharacter* m_pCharacter;
	CGS* m_pGS;

	IServer* Server() const;
	int m_ClientID;

public:
	CGS* GS() const { return m_pGS; }
	vec2 m_ViewPos;
	int m_PlayerFlags;
	int m_aPlayerTick[TickState::NUM_TICK];
	bool m_Flymode;
	int m_MoodState;

	StructLatency m_Latency;
	StructLastAction m_LatestActivity;

	/* #########################################################################
		VAR AND OBJECTS PLAYER MMO
	######################################################################### */
	CTuningParams m_PrevTuningParams;
	CTuningParams m_NextTuningParams;

	bool m_Spawned;

	vec3 m_VoteColored;
	short m_aSortTabs[NUM_SORT_TAB];
	short m_OpenVoteMenu;
	short m_LastVoteMenu;

	// TODO: fixme. improve the system using the ID method, as well as the ability to implement Backpage
	CVoteOptionsCallback m_ActiveMenuOptionCallback;
	VoteCallBack m_ActiveMenuRegisteredCallback;

	/* #########################################################################
		FUNCTIONS PLAYER ENGINE
	######################################################################### */
public:
	CPlayer(CGS* pGS, int ClientID);
	virtual ~CPlayer();

	virtual int GetTeam();
	virtual bool IsBot() const { return false; }
	virtual int GetBotID() const { return -1; };
	virtual int GetBotType() const { return -1; };
	virtual int GetBotSub() const { return -1; };
	virtual	int GetPlayerWorldID() const;

	virtual int GetStartHealth();
	int GetStartMana();
	virtual	int GetHealth() { return GetTempData().m_TempHealth; };
	virtual	int GetMana() { return GetTempData().m_TempMana; };

	virtual void HandleTuningParams();
	virtual int IsActiveSnappingBot(int SnappingClient) const { return 2; };
	virtual int GetEquippedItemID(int EquipID, int SkipItemID = -1) const;
	virtual int GetAttributeCount(int BonusID, bool ActiveFinalStats = false);
	int GetItemsAttributeCount(int AttributeID) const;
	virtual void UpdateTempData(int Health, int Mana);
	virtual void SendClientInfo(int TargetID);

	virtual void GiveEffect(const char* Potion, int Sec, int Random = 0);
	virtual bool IsActiveEffect(const char* Potion) const;
	virtual void ClearEffects();

	virtual void Tick();
	virtual void PostTick();
	virtual void Snap(int SnappingClient);
	
private:
	void EffectsTick();
	void TickSystemTalk();
	virtual void TryRespawn();

public:
	CCharacter *GetCharacter();

	void KillCharacter(int Weapon = WEAPON_WORLD);
	void OnDisconnect();
	void OnDirectInput(CNetObj_PlayerInput *NewInput);
	void OnPredictedInput(CNetObj_PlayerInput *NewInput);

	int GetCID() const { return m_ClientID; };
	/* #########################################################################
		FUNCTIONS PLAYER HELPER 
	######################################################################### */
	void ProgressBar(const char *Name, int MyLevel, int MyExp, int ExpNeed, int GivedExp);
	bool Upgrade(int Count, int *Upgrade, int *Useless, int Price, int MaximalUpgrade);

	/* #########################################################################
		FUNCTIONS PLAYER ACCOUNT 
	######################################################################### */
	bool SpendCurrency(int Price, int ItemID = 1);
	const char* GetLanguage() const;
	void AddExp(int Exp);
	void AddMoney(int Money);

	bool GetHidenMenu(int HideID) const;
	bool IsAuthed();
	int GetStartTeam();

	int ExpNeed(int Level) const;
	void ShowInformationStats();

	/* #########################################################################
		FUNCTIONS PLAYER PARSING 
	######################################################################### */
	bool ParseItemsF3F4(int Vote);
  	bool ParseVoteUpgrades(const char *CMD, const int VoteID, const int VoteID2, int Get);

	/* #########################################################################
		FUNCTIONS PLAYER ITEMS 
	######################################################################### */
	InventoryItem& GetItem(int ItemID);
	CSkill &GetSkill(int SkillID);
	CPlayerQuest& GetQuest(int QuestID);
	AccountMainJob::StructTempPlayerData& GetTempData() { return AccountMainJob::ms_aPlayerTempData[m_ClientID]; }
	AccountMainJob::StructData& Acc() { return AccountMainJob::ms_aData[m_ClientID]; }

	int GetLevelTypeAttribute(int Class);
	int GetLevelAllAttributes();

	// npc conversations
	void SetTalking(int TalkedID, bool IsStartDialogue);
	void ClearTalking();
	int GetTalkedID() const { return m_DialogNPC.m_TalkedID; };

	// dialog formating
	const char *GetDialogText();
	void FormatDialogText(int DataBotID, const char *pText);
	void ClearDialogText();

	int GetMoodState() const { return MOOD_NORMAL; }
	void ChangeWorld(int WorldID);
};

#endif
