/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_SQLSKILL_H
#define GAME_SERVER_SQLSKILL_H

#include "../component.h"

class SkillsSql : public CMmoComponent
{
/* #########################################################################
	GLOBAL SKILL CLASS 
######################################################################### */
public:
	static std::map < int , SkillInfo > SkillData;
	static std::map < int , std::map < int , SkillPlayer > > Skill;

	virtual void OnInitGlobal();
	virtual void OnInitAccount(CPlayer *pPlayer);

/* #########################################################################
	HELPER SKILL CLASS 
######################################################################### */
	int GetSkillBonus(int ClientID, int SkillID) const;
	int GetSkillLevel(int ClientID, int SkillID) const;

/* #########################################################################
	FUNCTION SKILL CLASS 
######################################################################### */
	void ShowMailSkillList(CPlayer *pPlayer);
	void SkillSelected(CPlayer *pPlayer, int SkillID);
	bool UpgradeSkill(CPlayer *pPlayer, int SkillID);
	bool UseSkill(CPlayer *pPlayer, int SkillID);
	virtual bool OnParseVotingMenu(CPlayer *pPlayer, const char *CMD, const int VoteID, const int VoteID2, int Get, const char *GetText);
};

#endif
 