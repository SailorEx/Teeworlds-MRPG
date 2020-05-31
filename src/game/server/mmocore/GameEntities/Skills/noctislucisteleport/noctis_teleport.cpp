/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>

#include "noctis_teleport.h"

CNoctisTeleport::CNoctisTeleport(CGameWorld *pGameWorld, CCharacter* pPlayerChar, int SkillBonus)
: CEntity(pGameWorld, CGameWorld::ENTYPE_NOCTIS_TELEPORT, pPlayerChar->GetPos(), 64.0f)
{
	// переданные аргументы
	m_pPlayerChar = pPlayerChar;
	m_Direction = vec2(m_pPlayerChar->m_Core.m_Input.m_TargetX, m_pPlayerChar->m_Core.m_Input.m_TargetY);
	m_SkillBonus = SkillBonus;
	m_LifeSpan = Server()->TickSpeed();
	GameWorld()->InsertEntity(this);
}

CNoctisTeleport::~CNoctisTeleport() {}

void CNoctisTeleport::Reset()
{
	if(m_pPlayerChar && m_pPlayerChar->IsAlive())
		GS()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);

	// уничтожаем обьект
	GS()->m_World.DestroyEntity(this);
	return;
}

void CNoctisTeleport::Tick()
{
	m_LifeSpan--;
	if(!m_pPlayerChar || !m_pPlayerChar->IsAlive())
	{
		Reset();
		return;
	}

	vec2 To = m_Pos + normalize(m_Direction) * GetProximityRadius();
	vec2 Size = vec2(GetProximityRadius()/2, GetProximityRadius()/2);
	CCharacter *pSearchChar = (CCharacter*)GS()->m_World.ClosestEntity(To, 64.0f, CGameWorld::ENTTYPE_CHARACTER, nullptr);
	if(!m_LifeSpan || GS()->Collision()->TestBox(To, Size) || GS()->m_World.IntersectClosestDoorEntity(To, GetProximityRadius()) 
		|| (pSearchChar && pSearchChar != m_pPlayerChar && pSearchChar->IsAllowedPVP(m_pPlayerChar->GetPlayer()->GetCID())))
	{
		GS()->CreateSound(m_pPlayerChar->GetPos(), SOUND_NINJA_FIRE);

		// change to new position
		vec2 OldPosition = m_Pos;
		m_pPlayerChar->ChangePosition(OldPosition);

		const int ClientID = m_pPlayerChar->GetPlayer()->GetCID();
		const int MaximalDamageSize = kurosio::translate_to_procent_rest(m_pPlayerChar->GetPlayer()->GetAttributeCount(Stats::StStrength, true), clamp(m_SkillBonus, 5, 50));
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			CPlayer *pSearchPlayer = GS()->GetPlayer(i, false, true);
			if(ClientID == i || !pSearchPlayer || !pSearchPlayer->GetCharacter()->IsAllowedPVP(ClientID) 
				|| distance(OldPosition, pSearchPlayer->GetCharacter()->GetPos()) > 320 
				|| GS()->Collision()->IntersectLineWithInvisible(OldPosition, pSearchPlayer->GetCharacter()->GetPos(), 0, 0))
				continue;

			// change position to player
			vec2 SearchPos = pSearchPlayer->GetCharacter()->GetPos();
			m_pPlayerChar->ChangePosition(SearchPos);

			// take damage
			vec2 Diff = SearchPos - OldPosition;
			vec2 Force = normalize(Diff) * 16.0f;
			pSearchPlayer->GetCharacter()->TakeDamage(Force * 24.0f, MaximalDamageSize, ClientID, WEAPON_NINJA);
			GS()->CreateExplosion(SearchPos, m_pPlayerChar->GetPlayer()->GetCID(), WEAPON_GRENADE, 0);
			GS()->CreateSound(SearchPos, SOUND_NINJA_HIT);
		}
		m_pPlayerChar->ChangePosition(OldPosition);

		// reset entity
		Reset();
		return;
	}
	m_PosTo = m_Pos;
	m_Pos += normalize(m_Direction) * 20.0f;
}

void CNoctisTeleport::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_PosTo.x;
	pObj->m_Y = (int)m_PosTo.y;
	pObj->m_FromX = (int)m_Pos.x;
	pObj->m_FromY = (int)m_Pos.y;
	pObj->m_StartTick = Server()->Tick();

}