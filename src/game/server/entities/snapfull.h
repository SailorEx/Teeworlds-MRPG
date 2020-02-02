/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_SNAP_FULL_H
#define GAME_SERVER_ENTITIES_SNAP_FULL_H

#include <game/server/entity.h>
#include <base/tl/array.h>

class CSnapFull : public CEntity
{
public:
	CSnapFull(CGameWorld *pGameWorld, vec2 Pos, int SnapID, int Owner, int Num, int Type, bool Changing, bool Projectile);
	~CSnapFull();

	virtual void Snap(int SnappingClient);
	virtual void Tick();
	virtual void Reset();

	int GetOwner() const { return m_Owner; }
	void AddItem(int Count, int Type, bool Projectile, bool Dynamic, int SnapID);
	void RemoveItem(int Count, int SnapID, bool Effect);

private: 
	struct SnapItem
	{
		int m_SnapID;
		int m_ID;
		int m_Type;
		bool m_Changing;
		bool m_Projectile;
	};
	std::list < struct SnapItem > m_SnapItem;

	int m_Owner;
	int m_LoadingTick;
	bool m_boolreback;
};

#endif