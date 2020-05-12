/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_DROPINGITEMS_H
#define GAME_SERVER_ENTITIES_DROPINGITEMS_H

class CDropItem : public CEntity
{
	enum
	{
		PERSPECT = 1,
		BODY,
		NUM_IDS,
	};

	vec2 m_Vel;
	float m_Angle;
	float m_AngleForce;

	int m_StartTick;
	bool m_Flashing;
	int m_LifeSpan;
	int m_FlashTimer;

	ItemJob::InventoryItem m_DropItem;
	int m_OwnerID;
	int m_IDs[NUM_IDS];

public:
	CDropItem(CGameWorld *pGameWorld, vec2 Pos, vec2 Vel, float AngleForce, ItemJob::InventoryItem DropItem, int OwnerID);
	~CDropItem();

	virtual void Tick();
	virtual void TickPaused(); 
	virtual void Snap(int SnappingClient);

	bool TakeItem(int ClientID);
};

#endif