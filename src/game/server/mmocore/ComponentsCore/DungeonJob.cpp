/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include "DungeonJob.h"

using namespace sqlstr;

std::map < int , DungeonJob::StructDungeon > DungeonJob::Dungeon;

DungeonJob::DungeonJob()
{
	boost::scoped_ptr<ResultSet> RES(SJK.SD("*", "tw_dungeons"));
	while (RES->next())
	{
		int ID = RES->getInt("ID");
		str_copy(Dungeon[ID].Name, RES->getString("Name").c_str(), sizeof(Dungeon[ID].Name));

		Dungeon[ID].Level = RES->getInt("Level");
		Dungeon[ID].DoorX = RES->getInt("DoorX");
		Dungeon[ID].DoorY = RES->getInt("DoorY");
		Dungeon[ID].WorldID = RES->getInt("WorldID");
	}
}

void DungeonJob::SaveDungeonRecord(CPlayer* pPlayer, int DungeonID, int Seconds)
{
	boost::scoped_ptr<ResultSet> RES(SJK.SD("*", "tw_dungeons_records", "WHERE OwnerID = '%d' AND DungeonID = '%d'", pPlayer->Acc().AuthID, DungeonID));
	if (RES->next())
	{
		if (RES->getInt("Seconds") > Seconds)
			SJK.UD("tw_dungeons_records", "Seconds = '%d' WHERE OwnerID = '%d' AND DungeonID = '%d'", Seconds, pPlayer->Acc().AuthID, DungeonID);
		return;
	}
	SJK.ID("tw_dungeons_records", "(OwnerID, DungeonID, Seconds) VALUES ('%d', '%d', '%d')", pPlayer->Acc().AuthID, DungeonID, Seconds);
}

void DungeonJob::ShowDungeonTop(CPlayer* pPlayer, int DungeonID, int HideID)
{
	int ClientID = pPlayer->GetCID();
	boost::scoped_ptr<ResultSet> RES(SJK.SD("*", "tw_dungeons_records", "WHERE DungeonID = '%d' ORDER BY Seconds ASC LIMIT 5", DungeonID));
	while (RES->next())
	{
		int Rank = RES->getRow();
		int OwnerID = RES->getInt("OwnerID");
		int Seconds = RES->getDouble("Seconds");

		char aTimeFormat[64];
		str_format(aTimeFormat, sizeof(aTimeFormat), "Time: %d minute(s) %d second(s)", Seconds / 60, Seconds - (Seconds / 60 * 60));
		GS()->AVM(ClientID, "null", NOPE, HideID, "{INT}. {STR} : {STR}", &Rank, Job()->PlayerName(OwnerID), aTimeFormat);
	}
}

void DungeonJob::ShowDungeonsList(CPlayer* pPlayer)
{
	int ClientID = pPlayer->GetCID();
	for (const auto& dungeon : Dungeon)
	{
		int HideID = 7500 + dungeon.first;
		GS()->AVH(ClientID, HideID, GOLDEN_COLOR, "Lvl{INT} {STR} : Players {INT} : {STR} [{INT}%]",
			&dungeon.second.Level, dungeon.second.Name, &dungeon.second.Players, (dungeon.second.State > 1 ? "Active dungeon" : "Waiting players"), &dungeon.second.Progress);

		ShowDungeonTop(pPlayer, dungeon.first, HideID);
		GS()->AVM(ClientID, "DUNGEONJOIN", dungeon.first, HideID, "Join dungeon {STR}", dungeon.second.Name);
	}

	if (GS()->IsDungeon())
	{
		GS()->AV(ClientID, "null", "");
		pPlayer->m_Colored = { 30, 8, 8 };
		GS()->AVL(ClientID, "DUNGEONEXIT", "Exit dungeon {STR} !!", Dungeon[GS()->DungeonID()].Name);
	}
}

bool DungeonJob::OnHandleMenulist(CPlayer* pPlayer, int Menulist, bool ReplaceMenu)
{
	const int ClientID = pPlayer->GetCID();
	if (ReplaceMenu)
	{
		return false;
	}


	if (Menulist == MenuList::MENU_DUNGEONS)
	{
		pPlayer->m_LastVoteMenu = MenuList::MAIN_MENU;
		GS()->AVH(ClientID, TAB_INFO_DUNGEON, GREEN_COLOR, "Dungeons Information");
		GS()->AVM(ClientID, "null", NOPE, TAB_INFO_DUNGEON, "In this section you can choose a dungeon");
		GS()->AVM(ClientID, "null", NOPE, TAB_INFO_DUNGEON, "View the fastest players on the passage");

		GS()->AV(ClientID, "null", "");
		ShowDungeonsList(pPlayer);
		GS()->AddBack(ClientID);
		return true;
	}
	return false;
}

bool DungeonJob::OnVotingMenu(CPlayer* pPlayer, const char* CMD, const int VoteID, const int VoteID2, int Get, const char* GetText)
{
	const int ClientID = pPlayer->GetCID();
	if (!pPlayer->GetCharacter() || !pPlayer->GetCharacter()->IsAlive())
		return false;

	if (PPSTR(CMD, "DUNGEONJOIN") == 0)
	{
		if (Dungeon[VoteID].State > 1)
		{
			GS()->Chat(ClientID, "At the moment players are passing this dungeon!");
			GS()->VResetVotes(ClientID, MenuList::MENU_DUNGEONS);
			return true;
		}

		if (pPlayer->Acc().Level < Dungeon[VoteID].Level)
		{
			GS()->Chat(ClientID, "Your level is low to pass this dungeon!");
			GS()->VResetVotes(ClientID, MenuList::MENU_DUNGEONS);
			return true;
		}

		GS()->Chat(-1, "{STR} joined to Dungeon {STR}!", GS()->Server()->ClientName(ClientID), Dungeon[VoteID].Name);

		if (!GS()->IsDungeon())
			pPlayer->Acc().LastWorldID = GS()->GetWorldID();

		pPlayer->Acc().TeleportX = pPlayer->GetCharacter()->m_Core.m_Pos.x;
		pPlayer->Acc().TeleportY = pPlayer->GetCharacter()->m_Core.m_Pos.y;
		GS()->Server()->ChangeWorld(ClientID, Dungeon[VoteID].WorldID);
		return true;
	}
	else if (PPSTR(CMD, "DUNGEONEXIT") == 0)
	{
		GS()->Server()->ChangeWorld(ClientID, pPlayer->Acc().LastWorldID);
		return true;
	}

	return false;
}

int DungeonJob::SyncFactor()
{
	if (!GS()->IsDungeon())
		return 0;

	int Factor = 0;
	for (int i = MAX_PLAYERS; i < MAX_CLIENTS; i++)
	{
		CPlayerBot* BotPlayer = static_cast<CPlayerBot*>(GS()->m_apPlayers[i]);
		if (BotPlayer && BotPlayer->GetSpawnBot() == SpawnBot::SPAWN_MOBS && GS()->CheckPlayerMessageWorldID(i) == GS()->GetWorldID())
			Factor += BotPlayer->GetStartHealth();
	}
	return Factor;
}