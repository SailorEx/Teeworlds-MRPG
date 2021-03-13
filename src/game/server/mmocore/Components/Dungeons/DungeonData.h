/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_COMPONENT_DUNGEON_DATA_H
#define GAME_SERVER_COMPONENT_DUNGEON_DATA_H

struct CDungeonData
{
	char m_aName[64];
	int m_Level;
	int m_OpenQuestID;
	int m_DoorX;
	int m_DoorY;
	int m_WorldID;
	int m_Players;
	int m_Progress;
	int m_State;
	bool m_IsStory;

	bool IsDungeonPlaying() const { return m_State > 1; }

	static std::map < int, CDungeonData > ms_aDungeon;
};

#endif