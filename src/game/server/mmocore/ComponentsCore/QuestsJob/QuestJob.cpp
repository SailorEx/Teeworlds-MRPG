/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <teeother/system/string.h>

#include <game/server/gamecontext.h>
#include "QuestJob.h"

/*
	Task list:
	- Resume arrow for quest npc
	- More clear structures quests
	- Resume task board quest npc
	- Clear data steps after finished quest (done not full)
*/

std::map < int, std::map <int, CPlayerQuest > > QuestJob::ms_aPlayerQuests;
std::map < int, CDataQuest > QuestJob::ms_aDataQuests;

static const char* GetStateName(int Type)
{
	switch(Type)
	{
		case QuestState::QUEST_ACCEPT: return "Active";
		case QuestState::QUEST_FINISHED: return "Finished";
		default: return "Not active";
	}
}

void QuestJob::ShowQuestsMainList(CPlayer* pPlayer)
{
	// show the quest sheet
	ShowQuestsTabList(pPlayer, QuestState::QUEST_ACCEPT);
	ShowQuestsTabList(pPlayer, QuestState::QUEST_NO_ACCEPT);

	// show the completed menu
	pPlayer->m_Colored = BLUE_COLOR;
	GS()->AVM(pPlayer->GetCID(), "MENU", MenuList::MENU_JOURNAL_FINISHED, NOPE, "List of completed quests");
}

void QuestJob::ShowQuestsTabList(CPlayer* pPlayer, int StateQuest)
{
	const int ClientID = pPlayer->GetCID();
	pPlayer->m_Colored = GOLDEN_COLOR;
	GS()->AVL(ClientID, "null", "★ {STR} quests", GetStateName(StateQuest));

	// check first quest story step
	bool IsEmptyList = true;
	std::list < std::string /*stories was checked*/ > StoriesChecked;
	for(const auto& pDataQuest : ms_aDataQuests)
	{
		if(pPlayer->GetQuest(pDataQuest.first).GetState() != StateQuest)
			continue;

		if(StateQuest == QuestState::QUEST_FINISHED)
		{
			ShowQuestID(pPlayer, pDataQuest.first);
			continue;
		}

		const auto& IsAlreadyChecked = std::find_if(StoriesChecked.begin(), StoriesChecked.end(), [=](const std::string& stories)
		{ return (str_comp_nocase(ms_aDataQuests[pDataQuest.first].m_aStoryLine, stories.c_str()) == 0); });
		if(IsAlreadyChecked == StoriesChecked.end())
		{
			StoriesChecked.emplace_back(ms_aDataQuests[pDataQuest.first].m_aStoryLine);
			ShowQuestID(pPlayer, pDataQuest.first);
			IsEmptyList = false;
		}
	}

	// if the quest list is empty
	if(IsEmptyList)
	{
		pPlayer->m_Colored = LIGHT_GOLDEN_COLOR;
		GS()->AV(ClientID, "null", "This list is empty");
	}
	GS()->AV(ClientID, "null");
}

void QuestJob::ShowQuestID(CPlayer *pPlayer, int QuestID)
{
	CDataQuest pData = pPlayer->GetQuest(QuestID).Info();
	const int ClientID = pPlayer->GetCID();
	const int CountQuest = pData.GetStoryCount();
	const int LineQuest = pData.GetStoryCount(QuestID) + 1;

	// TODO: REMOVE IT
	GS()->AVCALLBACK(ClientID, "MENU", "\0", QuestID, NOPE, NOPE, "{INT}/{INT} {STR}: {STR}", [](CVoteOptionsCallback Callback)
	{
		CPlayer* pPlayer = Callback.pPlayer;
		const int ClientID = pPlayer->GetCID();
		const int QuestID = Callback.VoteID;
		CDataQuest pData = pPlayer->GetQuest(QuestID).Info();

		pPlayer->GS()->ClearVotes(ClientID);
		pPlayer->GS()->Mmo()->Quest()->ShowQuestsActiveNPC(pPlayer, QuestID);
		pPlayer->GS()->AV(ClientID, "null");

		pPlayer->m_Colored = GOLDEN_COLOR;
		pPlayer->GS()->AVL(ClientID, "null", "{STR} : Reward", pData.GetName());
		pPlayer->m_Colored = LIGHT_GOLDEN_COLOR;
		pPlayer->GS()->AVL(ClientID, "null", "Gold: {INT} Exp: {INT}", &pData.m_Gold, &pData.m_Exp);

		pPlayer->m_LastVoteMenu = MenuList::MENU_JOURNAL_MAIN;
		pPlayer->GS()->AddBackpage(ClientID);
	}, &LineQuest, &CountQuest, pData.GetStory(), pData.GetName());
}

// active npc information display
void QuestJob::ShowQuestsActiveNPC(CPlayer* pPlayer, int QuestID)
{
	CPlayerQuest& pPlayerQuest = pPlayer->GetQuest(QuestID);
	if(pPlayerQuest.GetState() != QUEST_ACCEPT)
		return;

	const int clientID = pPlayer->GetCID();
	pPlayer->m_Colored = BLUE_COLOR;
	GS()->AVM(clientID, "null", NOPE, NOPE, "Active NPC for current quests");

	for(auto& pStepBot : ms_aPlayerQuests[clientID][QuestID].m_StepsQuestBot)
	{
		// header
		BotJob::QuestBotInfo* pBotInfo = pStepBot.second.m_Bot;
		const int HideID = (NUM_TAB_MENU + 12500 + pBotInfo->m_SubBotID);
		const int PosX = pBotInfo->m_PositionX / 32, PosY = pBotInfo->m_PositionY / 32;
		GS()->AVH(clientID, HideID, LIGHT_BLUE_COLOR, "{STR} {STR}(x{INT} y{INT})", pBotInfo->GetName(), GS()->Server()->GetWorldName(pBotInfo->m_WorldID), &PosX, &PosY);

		// need for bot
		bool NeedOnlyTalk = true;
		for(int i = 0; i < 2; i++)
		{
			const int NeedKillMobID = pBotInfo->m_aNeedMob[i];
			const int KillNeed = pBotInfo->m_aNeedMobCount[i];
			if(NeedKillMobID > 0 && KillNeed > 0 && Job()->BotsData()->IsDataBotValid(NeedKillMobID))
			{
				GS()->AVMI(clientID, "broken_h", "null", NOPE, HideID, "- Defeat {STR} [{INT}/{INT}]",
					BotJob::ms_aDataBot[NeedKillMobID].m_aNameBot, &pStepBot.second.m_MobProgress[i], &KillNeed);
				NeedOnlyTalk = false;
			}

			const int NeedItemID = pBotInfo->m_aItemSearch[i];
			const int NeedCount = pBotInfo->m_aItemSearchCount[i];
			if(NeedItemID > 0 && NeedCount > 0)
			{
				InventoryItem PlayerItem = pPlayer->GetItem(NeedItemID);
				int ClapmItem = clamp(PlayerItem.m_Count, 0, NeedCount);
				GS()->AVMI(clientID, PlayerItem.Info().GetIcon(), "null", NOPE, HideID, "- Item {STR} [{INT}/{INT}]", PlayerItem.Info().GetName(pPlayer), &ClapmItem, &NeedCount);
				NeedOnlyTalk = false;
			}
		}

		// reward from bot
		for(int i = 0; i < 2; i++)
		{
			const int RewardItemID = pBotInfo->m_aItemGives[i];
			const int RewardCount = pBotInfo->m_aItemGivesCount[i];
			if(RewardItemID > 0 && RewardCount > 0)
			{
				ItemInformation RewardItem = GS()->GetItemInfo(RewardItemID);
				GS()->AVMI(clientID, RewardItem.GetIcon(), "null", NOPE, HideID, "- Receive {STR}x{INT}", RewardItem.GetName(pPlayer), &RewardCount);
			}
		}

		if(NeedOnlyTalk)
			GS()->AVM(clientID, "null", NOPE, HideID, "You just need to talk.");
	}
}

bool QuestJob::InteractiveQuestNPC(CPlayer* pPlayer, BotJob::QuestBotInfo& pBot, bool LastDialog)
{
	const int QuestID = pBot.m_QuestID;
	const int ClientID = pPlayer->GetCID();
	auto Item = std::find_if(ms_aPlayerQuests[ClientID][QuestID].m_StepsQuestBot.begin(), ms_aPlayerQuests[ClientID][QuestID].m_StepsQuestBot.end(), 
		[pBot](const std::pair<int, CPlayerStepQuestBot>& pStepBot) { return pStepBot.second.m_Bot->m_SubBotID == pBot.m_SubBotID; });
	return (Item != ms_aPlayerQuests[ClientID][QuestID].m_StepsQuestBot.end() ? Item->second.Finish(pPlayer, LastDialog) : false);
}

void QuestJob::AddMobProgressQuests(CPlayer* pPlayer, int BotID)
{
	// check complected steps
	const int ClientID = pPlayer->GetCID();
	for(auto& pPlayerQuest : QuestJob::ms_aPlayerQuests[ClientID])
	{
		if(pPlayerQuest.second.m_State != QuestState::QUEST_ACCEPT)
			continue;

		for(auto& pStepBot : pPlayerQuest.second.m_StepsQuestBot)
			pStepBot.second.AddMobProgress(pPlayer, BotID);
	}
}

void QuestJob::UpdateArrowStep(int ClientID)
{
	CPlayer* pPlayer = GS()->GetPlayer(ClientID, true, true);
	if (!pPlayer)
		return;

	for (const auto& qp : ms_aPlayerQuests[ClientID])
	{
		if (qp.second.m_State == QuestState::QUEST_ACCEPT)
			pPlayer->GetCharacter()->CreateQuestsStep(qp.first);
	}
}

void QuestJob::AcceptNextStoryQuestStep(CPlayer *pPlayer, int CheckQuestID)
{
	const CDataQuest CheckingQuest = ms_aDataQuests[CheckQuestID];
	for (auto pQuestData = ms_aDataQuests.find(CheckQuestID); pQuestData != ms_aDataQuests.end(); pQuestData++)
	{
		// search next quest story step
		if(str_comp_nocase(CheckingQuest.m_aStoryLine, pQuestData->second.m_aStoryLine) == 0)
		{
			// skip all if a quest story is found that is still active
			if(pPlayer->GetQuest(pQuestData->first).GetState() == QUEST_ACCEPT)
				break;

			// accept next quest step
			if(!IsValidQuest(pQuestData->first, pPlayer->GetCID()) || pPlayer->GetQuest(pQuestData->first).Accept())
				break;
		}
	}
}

void QuestJob::AcceptNextStoryQuestStep(CPlayer* pPlayer)
{
	// check first quest story step search active quests
	std::list < std::string /*stories was checked*/ > StoriesChecked;
	for(const auto& qp : ms_aPlayerQuests[pPlayer->GetCID()])
	{
		const auto& IsAlreadyChecked = std::find_if(StoriesChecked.begin(), StoriesChecked.end(), [=](const std::string &stories)
		{ return (str_comp_nocase(ms_aDataQuests[qp.first].m_aStoryLine, stories.c_str()) == 0); });
		if(IsAlreadyChecked == StoriesChecked.end())
		{
			StoriesChecked.emplace_back(ms_aDataQuests[qp.first].m_aStoryLine);
			AcceptNextStoryQuestStep(pPlayer, qp.first);
		}
	}
}

void QuestJob::QuestTableShowRequired(CPlayer *pPlayer, BotJob::QuestBotInfo &BotData, const char* TextTalk)
{
	/*if(!BotData.m_ContinuesStepQuest)
		return;

	const int ClientID = pPlayer->GetCID();
	if (GS()->IsMmoClient(ClientID))
	{
		QuestTableShowRequired(pPlayer, BotData);
		return;
	}

	char aBuf[64];
	dynamic_string Buffer;
	bool IsActiveTask = false;
	const int QuestID = BotData.m_QuestID;

	// search item's and mob's
	for(int i = 0; i < 2; i++)
	{
		const int BotID = BotData.m_aNeedMob[i];
		const int CountMob = BotData.m_aNeedMobCount[i];
		if(BotID > 0 && CountMob > 0 && Job()->BotsData()->IsDataBotValid(BotID))
		{
			str_format(aBuf, sizeof(aBuf), "\n- Defeat %s [%d/%d]", BotJob::ms_aDataBot[BotID].m_aNameBot, ms_aPlayerQuests[ClientID][QuestID].m_aMobProgress[i], CountMob);
			Buffer.append_at(Buffer.length(), aBuf);
			IsActiveTask = true;
		}

		const int ItemID = BotData.m_aItemSearch[i];
		const int CountItem = BotData.m_aItemSearchCount[i];
		if(ItemID > 0 && CountItem > 0)
		{
			InventoryItem PlayerQuestItem = pPlayer->GetItem(ItemID);
			str_format(aBuf, sizeof(aBuf), "\n- Need %s [%d/%d]", PlayerQuestItem.Info().GetName(pPlayer), PlayerQuestItem.m_Count, CountItem);
			Buffer.append_at(Buffer.length(), aBuf);
			IsActiveTask = true;
		}
	}

	// type random accept item's
	if(BotData.m_InteractiveType == (int)QuestInteractive::INTERACTIVE_RANDOM_ACCEPT_ITEM)
	{
		const double Chance = BotData.m_InteractiveTemp <= 0 ? 100.0f : (1.0f / (double)BotData.m_InteractiveTemp) * 100;
		str_format(aBuf, sizeof(aBuf), "\nChance that item he'll like [%0.2f%%]\n", Chance);
		Buffer.append_at(Buffer.length(), aBuf);
	}

	// reward item's
	for(int i = 0; i < 2; i++)
	{
		const int ItemID = BotData.m_aItemGives[i];
		const int CountItem = BotData.m_aItemGivesCount[i];
		if(ItemID > 0 && CountItem > 0)
		{
			str_format(aBuf, sizeof(aBuf), "\n- Receive %s [%d]", GS()->GetItemInfo(ItemID).GetName(pPlayer), CountItem);
			Buffer.append_at(Buffer.length(), aBuf);
		}
	}

	GS()->Motd(ClientID, "{STR}\n\n{STR}{STR}\n\n", TextTalk, (IsActiveTask ? "### Task" : "\0"), Buffer.buffer());
	pPlayer->ClearFormatQuestText();
	Buffer.clear();*/
}

void QuestJob::QuestTableShowRequired(CPlayer* pPlayer, BotJob::QuestBotInfo& BotData)
{
	/*char aBuf[64];
	const int ClientID = pPlayer->GetCID();

	// search item's
	for (int i = 0; i < 2; i++)
	{
		const int ItemID = BotData.m_aItemSearch[i];
		const int CountItem = BotData.m_aItemSearchCount[i];
		if(ItemID <= 0 || CountItem <= 0)
			continue;

		if(BotData.m_InteractiveType == (int)QuestInteractive::INTERACTIVE_RANDOM_ACCEPT_ITEM)
		{
			const float Chance = BotData.m_InteractiveTemp <= 0 ? 100.0f : (1.0f / (float)BotData.m_InteractiveTemp) * 100;
			str_format(aBuf, sizeof(aBuf), "%s [takes %0.2f%%]", aBuf, Chance);
		}
		else
			str_format(aBuf, sizeof(aBuf), "%s", pPlayer->GetItem(ItemID).Info().GetName(pPlayer));

		GS()->Mmo()->Quest()->QuestTableAddItem(ClientID, aBuf, CountItem, ItemID, false);
	}

	// search mob's
	for (int i = 0; i < 2; i++)
	{
		const int BotID = BotData.m_aNeedMob[i];
		const int CountMob = BotData.m_aNeedMobCount[i];
		if (BotID <= 0 || CountMob <= 0 || !GS()->Mmo()->BotsData()->IsDataBotValid(BotID))
			continue;

		str_format(aBuf, sizeof(aBuf), "Defeat %s", BotJob::ms_aDataBot[BotID].m_aNameBot);
		GS()->Mmo()->Quest()->QuestTableAddInfo(ClientID, aBuf, CountMob, QuestJob::ms_aPlayerQuests[ClientID][BotData.m_QuestID].m_aMobProgress[i]);
	}

	// reward item's
	for (int i = 0; i < 2; i++)
	{
		const int ItemID = BotData.m_aItemGives[i];
		const int CountItem = BotData.m_aItemGivesCount[i];
		if (ItemID <= 0 || CountItem <= 0)
			continue;

		str_format(aBuf, sizeof(aBuf), "Receive %s", pPlayer->GetItem(ItemID).Info().GetName(pPlayer));
		GS()->Mmo()->Quest()->QuestTableAddItem(ClientID, aBuf, CountItem, ItemID, true);
	}*/
}

void QuestJob::QuestTableAddItem(int ClientID, const char* pText, int Requires, int ItemID, bool GivingTable)
{
	CPlayer* pPlayer = GS()->GetPlayer(ClientID, true);
	if (!pPlayer || ItemID < itGold || !GS()->IsMmoClient(ClientID))
		return;

	const InventoryItem PlayerSelectedItem = pPlayer->GetItem(ItemID);

	CNetMsg_Sv_AddQuestingProcessing Msg;
	Msg.m_pText = pText;
	Msg.m_pRequiresNum = Requires;
	Msg.m_pHaveNum = clamp(PlayerSelectedItem.m_Count, 0, Requires);
	Msg.m_pGivingTable = GivingTable;
	StrToInts(Msg.m_pIcon, 4, PlayerSelectedItem.Info().GetIcon());
	GS()->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void QuestJob::QuestTableAddInfo(int ClientID, const char *pText, int Requires, int Have)
{
	if (ClientID < 0 || ClientID >= MAX_PLAYERS || !GS()->IsMmoClient(ClientID))
		return;

	CNetMsg_Sv_AddQuestingProcessing Msg;
	Msg.m_pText = pText;
	Msg.m_pRequiresNum = Requires;
	Msg.m_pHaveNum = clamp(Have, 0, Requires);
	Msg.m_pGivingTable = false;
	StrToInts(Msg.m_pIcon, 4, "hammer");
	GS()->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void QuestJob::QuestTableClear(int ClientID)
{
	if (ClientID < 0 || ClientID >= MAX_PLAYERS || !GS()->IsMmoClient(ClientID))
		return;
	
	CNetMsg_Sv_ClearQuestingProcessing Msg;
	GS()->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}
/*
int QuestJob::QuestingAllowedItemsCount(CPlayer *pPlayer, int ItemID)
{
	const int ClientID = pPlayer->GetCID();
	const InventoryItem PlayerSearchItem = pPlayer->GetItem(ItemID);
	for (const auto& qq : ms_aPlayerQuests[ClientID])
	{
		if (qq.second.m_State != QuestState::QUEST_ACCEPT)
			continue;

		BotJob::QuestBotInfo *BotInfo = GetQuestBot(qq.first, qq.second.m_Step);
		if (!BotInfo)
			continue;

		for (int i = 0; i < 2; i++)
		{
			const int needItemID = BotInfo->m_aItemSearch[i];
			const int numNeed = BotInfo->m_aItemSearchCount[i];
			if (needItemID <= 0 || numNeed <= 0 || ItemID != needItemID)
				continue;

			const int AvailableCount = clamp(PlayerSearchItem.m_Count - numNeed, 0, PlayerSearchItem.m_Count);
			return AvailableCount;
		}
	}
	return PlayerSearchItem.m_Count;
}
*/
void QuestJob::OnInit()
{
	std::shared_ptr<ResultSet> RES(SJK.SD("*", "tw_quests_list"));
	while (RES->next())
	{
		const int QUID = RES->getInt("ID");
		str_copy(ms_aDataQuests[QUID].m_aName, RES->getString("Name").c_str(), sizeof(ms_aDataQuests[QUID].m_aName));
		str_copy(ms_aDataQuests[QUID].m_aStoryLine, RES->getString("StoryLine").c_str(), sizeof(ms_aDataQuests[QUID].m_aStoryLine));
		ms_aDataQuests[QUID].m_Gold = (int)RES->getInt("Money");
		ms_aDataQuests[QUID].m_Exp = (int)RES->getInt("Exp");
		// init steps bots run on BotJob::Init
	}
}

void QuestJob::OnInitAccount(CPlayer* pPlayer)
{
	const int ClientID = pPlayer->GetCID();
	std::shared_ptr<ResultSet> RES(SJK.SD("*", "tw_accounts_quests", "WHERE OwnerID = '%d'", pPlayer->Acc().m_AuthID));
	while (RES->next())
	{
		const int QuestID = RES->getInt("QuestID");
		ms_aPlayerQuests[ClientID][QuestID].m_State = (int)RES->getInt("Type");
		ms_aPlayerQuests[ClientID][QuestID].m_Step = (int)RES->getInt("Step");
		ms_aPlayerQuests[ClientID][QuestID].m_StepsQuestBot = ms_aDataQuests[QuestID].CopySteps();
	}

	// init data steps players
	std::shared_ptr<ResultSet> PlayerData(SJK.SD("*", "tw_accounts_quests_bots_step", "WHERE OwnerID = '%d' ", pPlayer->Acc().m_AuthID));
	while(PlayerData->next())
	{
		const int SubBotID = PlayerData->getInt("SubBotID"); // is a unique value
		const int QuestID = BotJob::ms_aQuestBot[SubBotID].m_QuestID;
		ms_aPlayerQuests[ClientID][QuestID].m_StepsQuestBot[SubBotID].m_Bot = &BotJob::ms_aQuestBot[SubBotID];
		ms_aPlayerQuests[ClientID][QuestID].m_StepsQuestBot[SubBotID].m_MobProgress[0] = PlayerData->getInt("Mob1Progress");
		ms_aPlayerQuests[ClientID][QuestID].m_StepsQuestBot[SubBotID].m_MobProgress[1] = PlayerData->getInt("Mob1Progress");
		ms_aPlayerQuests[ClientID][QuestID].m_StepsQuestBot[SubBotID].m_StepComplete = PlayerData->getBoolean("Completed");
		ms_aPlayerQuests[ClientID][QuestID].m_StepsQuestBot[SubBotID].m_ClientQuitting = false;
		ms_aPlayerQuests[ClientID][QuestID].m_StepsQuestBot[SubBotID].UpdateBot(GS());
	}
}

void QuestJob::OnResetClient(int ClientID)
{
	for(auto& qp : ms_aPlayerQuests[ClientID])
	{
		for(auto& pStepBot : qp.second.m_StepsQuestBot)
		{
			pStepBot.second.m_ClientQuitting = true;
			pStepBot.second.UpdateBot(GS());
		}
	}
}

void QuestJob::OnMessage(int MsgID, void* pRawMsg, int ClientID)
{
	CPlayer* pPlayer = GS()->m_apPlayers[ClientID];
	if (MsgID == NETMSGTYPE_CL_TALKINTERACTIVE)
	{
		if (pPlayer->m_aPlayerTick[TickState::LastDialog] && pPlayer->m_aPlayerTick[TickState::LastDialog] > GS()->Server()->Tick())
			return;

		pPlayer->m_aPlayerTick[TickState::LastDialog] = GS()->Server()->Tick() + (GS()->Server()->TickSpeed() / 4);
		pPlayer->SetTalking(pPlayer->GetTalkedID(), true);
	}
}

bool QuestJob::OnHandleMenulist(CPlayer* pPlayer, int Menulist, bool ReplaceMenu)
{
	const int ClientID = pPlayer->GetCID();
	if (ReplaceMenu)
	{
		CCharacter* pChr = pPlayer->GetCharacter();
		if (!pChr || !pChr->IsAlive())
			return false;

		return false;
	}

	if (Menulist == MenuList::MENU_JOURNAL_FINISHED)
	{
		pPlayer->m_LastVoteMenu = MenuList::MENU_JOURNAL_MAIN;
		ShowQuestsTabList(pPlayer, QuestState::QUEST_FINISHED);
		GS()->AddBackpage(ClientID);
		return true;
	}

	return false;
}

bool QuestJob::OnParsingVoteCommands(CPlayer* pPlayer, const char* CMD, const int VoteID, const int VoteID2, int Get, const char* GetText)
{
	return false;
}
