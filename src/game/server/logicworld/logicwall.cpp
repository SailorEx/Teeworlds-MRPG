/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>

#include "logicwall.h"

CLogicWall::CLogicWall(CGameWorld *pGameWorld, vec2 Pos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_EYES, Pos, 14)
{
	m_Pos = Pos;
	m_RespawnTick = Server()->TickSpeed()*10;
	pLogicWallLine = new CLogicWallLine(&GS()->m_World, m_Pos);
	GameWorld()->InsertEntity(this);
}

void CLogicWall::SetDestroy(int Sec)
{
	m_RespawnTick = Server()->TickSpeed()*Sec;
	if(pLogicWallLine) 
		pLogicWallLine->Respawn(false);

}

CPlayer *CLogicWall::FindPlayerAI(float Distance)
{
	for(int i = 0; i < MAX_PLAYERS; i++) {
		CPlayer *pPlayer = GS()->GetPlayer(i, true, true);
		if(!pPlayer || distance(pPlayer->GetCharacter()->m_Core.m_Pos, m_Pos) > Distance)
			continue;

		return pPlayer;
	}
	return NULL;
}

void CLogicWall::Tick()
{	
	if(m_RespawnTick)
	{
		m_RespawnTick--;
		if(!m_RespawnTick) {
			if(pLogicWallLine) 
				pLogicWallLine->Respawn(true);
		}
		return;
	}

	CPlayer *pPlayer = FindPlayerAI(250.0f);		
	if(Server()->Tick() % (Server()->TickSpeed()*5) == 0 && pPlayer) 
	{
		pLogicWallLine->SetClientID(pPlayer->GetCID());
		vec2 Dir = normalize(m_Pos - pPlayer->GetCharacter()->m_Core.m_Pos);
		new CLogicWallFire(&GS()->m_World, m_Pos, Dir, this);
	}
}

void CLogicWall::Snap(int SnappingClient)
{
	if(m_RespawnTick > 0 || NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, GetID(), sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = 1;
}


/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
CLogicWallFire::CLogicWallFire(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, CLogicWall *Eyes)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_EYES, Pos, 14)
{
	m_Pos = Pos;
	pLogicWall = Eyes;
	m_Dir = Direction;
	GameWorld()->InsertEntity(this);
}
void CLogicWallFire::Tick()
{	
	if(!pLogicWall || GS()->Collision()->CheckPoint(m_Pos.x, m_Pos.y))
	{
		GS()->m_World.DestroyEntity(this);		
		return;
	}

	for(CLogicWallWall *p = (CLogicWallWall*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_EYESWALL); p; p = (CLogicWallWall *)p->TypeNext())
	{
		vec2 IntersectPos = closest_point_on_line(p->GetPos(), p->GetTo(), m_Pos);
		if(distance(m_Pos, IntersectPos) < 15)
		{
			p->TakeDamage();
			GS()->CreateText(NULL, false, m_Pos, vec2(0, 0), 100, std::to_string(p->GetHealth()).c_str(), GS()->GetWorldID());
			if(p->GetHealth() <= 0) 
			{
				pLogicWall->SetDestroy(120);
				p->SetDestroy(120);
			}
			GS()->m_World.DestroyEntity(this);
			return;
		}
	}
	m_Pos += m_Dir*2.0f;
}

void CLogicWallFire::Snap(int SnappingClient)
{	
	if(NetworkClipped(SnappingClient))
		return;

	if(GS()->CheckClient(SnappingClient))
	{
		CNetObj_MmoProj *pProj = static_cast<CNetObj_MmoProj *>(Server()->SnapNewItem(NETOBJTYPE_MMOPROJ, GetID(), sizeof(CNetObj_MmoProj)));
		if(!pProj)
			return;
		pProj->m_X = (int)m_Pos.x;
		pProj->m_Y = (int)m_Pos.y;
		pProj->m_VelX = m_Dir.x;
		pProj->m_VelY = m_Dir.y;
		pProj->m_StartTick = Server()->Tick()-3;
		pProj->m_Type = 0;
		return;
	}

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, GetID(), sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = 1;
}

/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
CLogicWallWall::CLogicWallWall(CGameWorld *pGameWorld, vec2 Pos, int Mode, int Health)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_EYESWALL, Pos, 14)
{
	m_SaveHealth = m_Health = Health;
	m_Pos = Pos;
	m_To = Pos;

	if(Mode == 0)
	{
		m_Pos.y += 30;
		m_To = GS()->Collision()->FindDirCollision(100, m_To, 'y', '-');
	}
	else 
	{
		m_Pos.x += 30;
		m_To = GS()->Collision()->FindDirCollision(100, m_To, 'x', '+');
	}
	m_RespawnTick = Server()->TickSpeed()*10;
	GameWorld()->InsertEntity(this);
}

void CLogicWallWall::TakeDamage()
{
	m_Health -= 25;

}

void CLogicWallWall::SetDestroy(int Sec) { m_RespawnTick = Server()->TickSpeed()*Sec, m_Health = m_SaveHealth; }

void CLogicWallWall::Tick() 
{
	if(m_RespawnTick)
		m_RespawnTick--;

	if(!m_RespawnTick) 
	{
		for(CCharacter *pChar = (CCharacter*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChar; pChar = (CCharacter *)pChar->TypeNext())
		{
			vec2 IntersectPos = closest_point_on_line(m_Pos, m_To, pChar->m_Core.m_Pos);
			float Distance = distance(IntersectPos, pChar->m_Core.m_Pos);
		
			// снижаем скокрость
			if(Distance < 64.0f && length(pChar->m_Core.m_Vel) >= 64.0)
				pChar->m_Core.m_Vel = vec2(0,0);

			// проверяем дистанцию
			if(Distance < 30.0f) 
			{
				vec2 Dir = normalize(pChar->m_Core.m_Pos - IntersectPos);
				float a = (30.0f*1.45f - Distance);
				float Velocity = 0.5f;
				if (length(pChar->m_Core.m_Vel) > 0.0001)
					Velocity = 1-(dot(normalize(pChar->m_Core.m_Vel), Dir)+1)/4;
			
				pChar->m_Core.m_Vel += (Dir*a*(Velocity*0.75f))*0.85f;
				pChar->m_Core.m_Pos = pChar->m_OldPos + Dir;
			}
		}
	}
}

void CLogicWallWall::Snap(int SnappingClient)
{	
	if (m_RespawnTick > 0 || NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
	if (!pObj)
		return;

	pObj->m_X = int(m_Pos.x);
	pObj->m_Y = int(m_Pos.y);
	pObj->m_FromX = int(m_To.x);
	pObj->m_FromY = int(m_To.y);
	pObj->m_StartTick = Server()->Tick()-2;
}

/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
CLogicWallLine::CLogicWallLine(CGameWorld *pGameWorld, vec2 Pos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER, Pos, 14)
{
	m_Pos = Pos;
	m_To = Pos;
	m_ClientID = -1;
	m_Spawned = false;
	GameWorld()->InsertEntity(this);
}
void CLogicWallLine::Respawn(bool Spawn) { m_Spawned = Spawn; }
void CLogicWallLine::SetClientID(int ClientID) { m_ClientID = ClientID; }

void CLogicWallLine::Tick() 
{
	if(m_ClientID < 0 || m_ClientID > MAX_PLAYERS || !GS()->m_apPlayers[m_ClientID] || !GS()->m_apPlayers[m_ClientID]->GetCharacter())
	{
		m_To = m_Pos;
		return;
	}

	vec2 PositionPlayer = GS()->m_apPlayers[m_ClientID]->GetCharacter()->m_Core.m_Pos;
	if(m_Spawned && distance(m_Pos, PositionPlayer) < 300)
	{
		m_To = PositionPlayer;
		if(Server()->Tick() % Server()->TickSpeed() == 0)
			GS()->m_apPlayers[m_ClientID]->GetCharacter()->TakeDamage(vec2(0,0), vec2(0,0), 1, -1, WEAPON_WORLD);
		return;
	}
	m_To = m_Pos;
}

void CLogicWallLine::Snap(int SnappingClient)
{	
	if (!m_Spawned || NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
	if (!pObj)
		return;

	pObj->m_X = int(m_Pos.x);
	pObj->m_Y = int(m_Pos.y);
	pObj->m_FromX = int(m_To.x);
	pObj->m_FromY = int(m_To.y);
	pObj->m_StartTick = Server()->Tick()-5;
}


/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
CLogicDoorKey::CLogicDoorKey(CGameWorld *pGameWorld, vec2 Pos, int ItemID, int Mode)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER, Pos, 14)
{
	m_Pos = Pos;
	m_To = Pos;

	if(Mode == 0)
	{
		m_Pos.y += 30;
		m_To = GS()->Collision()->FindDirCollision(100, m_To, 'y', '-');

	}
	else 
	{
		m_Pos.x -= 30;
		m_To = GS()->Collision()->FindDirCollision(100, m_To, 'x', '+');
	}
	m_ItemID = ItemID;
	GameWorld()->InsertEntity(this);
}

void CLogicDoorKey::Tick() 
{
	for(CCharacter *pChar = (CCharacter*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChar; pChar = (CCharacter *)pChar->TypeNext())
	{
		if(pChar->GetPlayer()->GetItem(m_ItemID).Count)
			continue;
			
		vec2 IntersectPos = closest_point_on_line(m_Pos, m_To, pChar->m_Core.m_Pos);
		float Distance = distance(IntersectPos, pChar->m_Core.m_Pos);
		
		// снижаем скокрость
		if(Distance < 64.0f && length(pChar->m_Core.m_Vel) >= 64.0)
			pChar->m_Core.m_Vel = vec2(0,0);

		// проверяем дистанцию
		if(Distance < 30.0f) 
		{
			vec2 Dir = normalize(pChar->m_Core.m_Pos - IntersectPos);
			float a = (30.0f*1.45f - Distance);
			float Velocity = 0.5f;
			if (length(pChar->m_Core.m_Vel) > 0.0001)
				Velocity = 1-(dot(normalize(pChar->m_Core.m_Vel), Dir)+1)/4;
		
			pChar->m_Core.m_Vel += (Dir*a*(Velocity*0.75f))*0.85f;
			pChar->m_Core.m_Pos = pChar->m_OldPos + Dir;

			GS()->SBL(pChar->GetPlayer()->GetCID(), 100000, 100, _("You need {s:name}"), "name", GS()->GetItemInfo(m_ItemID).GetName(pChar->GetPlayer()));
		}
	}
}

void CLogicDoorKey::Snap(int SnappingClient)
{	
	if (NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
	if (!pObj)
		return;

	pObj->m_X = int(m_Pos.x);
	pObj->m_Y = int(m_Pos.y);
	pObj->m_FromX = int(m_To.x);
	pObj->m_FromY = int(m_To.y);
	pObj->m_StartTick = Server()->Tick()-3;
}

/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
CLogicDestroyDoorKey::CLogicDestroyDoorKey(CGameWorld *pGameWorld, vec2 Pos, int ItemID, int Mode)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER, Pos, 14)
{
	m_Pos = Pos;
	m_To = Pos;

	if(Mode == 0)
	{
		m_Pos.y += 30;
		m_To = GS()->Collision()->FindDirCollision(100, m_To, 'y', '-');
	}
	else 
	{
		m_Pos.x -= 30;
		m_To = GS()->Collision()->FindDirCollision(100, m_To, 'x', '+');
	}
	m_ItemID = ItemID;
	GameWorld()->InsertEntity(this);
}

void CLogicDestroyDoorKey::SetDestroyRespawn()
{
	GS()->CreateExplosion(m_To, -1, WEAPON_SELF, 0);
	GS()->CreateExplosion(m_Pos, -1, WEAPON_SELF, 0);
	m_RespawnTick = 60*Server()->TickSpeed();
}

void CLogicDestroyDoorKey::Tick() 
{
	// снимаем респавн тик
	if(m_RespawnTick)
	{
		m_RespawnTick--;
		return;
	}

	// ищим игроков для двери
	for(CCharacter *pChar = (CCharacter*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChar; pChar = (CCharacter *)pChar->TypeNext())
	{
		// проверяем кол-во предмет хватает ли
		ItemSql::ItemPlayer &KeyItem = pChar->GetPlayer()->GetItem(m_ItemID);
		if(KeyItem.Count)
		{
			SetDestroyRespawn();
			GS()->SBL(pChar->GetPlayer()->GetCID(), 100000, 100, _("The door is unlocked for 60 seconds"), NULL);
			KeyItem.Remove(1);
			break;
		}

		// если не залетаем во что либо
		vec2 IntersectPos = closest_point_on_line(m_Pos, m_To, pChar->m_Core.m_Pos);
		float Distance = distance(IntersectPos, pChar->m_Core.m_Pos);
		
		// снижаем скокрость
		if(Distance < 64.0f && length(pChar->m_Core.m_Vel) >= 64.0)
			pChar->m_Core.m_Vel = vec2(0,0);

		// проверяем дистанцию
		if(Distance < 30.0f) 
		{
			vec2 Dir = normalize(pChar->m_Core.m_Pos - IntersectPos);
			float a = (30.0f*1.45f - Distance);
			float Velocity = 0.5f;
			if (length(pChar->m_Core.m_Vel) > 0.0001)
				Velocity = 1-(dot(normalize(pChar->m_Core.m_Vel), Dir)+1)/4;
		
			pChar->m_Core.m_Vel += (Dir*a*(Velocity*0.75f))*0.85f;
			pChar->m_Core.m_Pos = pChar->m_OldPos + Dir;

			GS()->SBL(pChar->GetPlayer()->GetCID(), 100000, 100, _("You need {s:name}"), "name", KeyItem.Info().GetName(pChar->GetPlayer()));
		}
	}
}

void CLogicDestroyDoorKey::Snap(int SnappingClient)
{	
	if (m_RespawnTick || NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
	if (!pObj)
		return;

	pObj->m_X = int(m_Pos.x);
	pObj->m_Y = int(m_Pos.y);
	pObj->m_FromX = int(m_To.x);
	pObj->m_FromY = int(m_To.y);
	pObj->m_StartTick = Server()->Tick()-3;
}