/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_NPCWALL_H
#define GAME_SERVER_ENTITIES_NPCWALL_H

#include <game/server/entity.h>

class CNPCWall : public CEntity
{
public:
	CNPCWall(CGameWorld *pGameWorld, vec2 Pos, bool Left);

	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	vec2 m_To;
	bool m_Active;
};

#endif