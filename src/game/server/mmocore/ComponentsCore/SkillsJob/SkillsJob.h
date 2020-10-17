/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_SKILLJOB_H
#define GAME_SERVER_SKILLJOB_H

#include <game/server/mmocore/MmoComponent.h>

class SkillsJob : public MmoComponent
{
public:
	static std::map < int, CSkillInformation > ms_aSkillsData;
	static std::map < int, std::map < int, CSkill > > ms_aSkills;

	virtual void OnInit();
	virtual void OnInitAccount(CPlayer *pPlayer);
	virtual void OnResetClient(int ClientID);
	virtual bool OnHandleTile(CCharacter* pChr, int IndexCollision);
	virtual bool OnHandleMenulist(CPlayer* pPlayer, int Menulist, bool ReplaceMenu);
	virtual bool OnParsingVoteCommands(CPlayer* pPlayer, const char* CMD, const int VoteID, const int VoteID2, int Get, const char* GetText);

	void ParseEmoticionSkill(CPlayer* pPlayer, int EmoticionID);

private:
	void ShowMailSkillList(CPlayer* pPlayer, bool Pasive);
	void SkillSelected(CPlayer* pPlayer, int SkillID);
};

#endif
 