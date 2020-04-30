/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_HOUSESJOB_H
#define GAME_SERVER_HOUSESJOB_H

#include <game/server/mmocore/GameEntities/decorations_houses.h>
#include "../MmoComponent.h"

class HouseDoor;
class DecoHouse;
class HouseJob : public MmoComponent
{
	/* #########################################################################
		VAR AND OBJECTS HOUSES 
	######################################################################### */
	struct HouseList
	{
		int m_PosX;
		int m_PosY;
		int m_DoorX;
		int m_DoorY;
		int m_Price;
		char m_Class[32];
		int m_OwnerID;
		int m_Bank;
		int m_Farm;
		int m_FarmLevel;
		int m_PlantID;
		int m_PlantPosX;
		int m_PlantPosY;
		int m_WorldID;

		HouseDoor *m_Door;
	};
	typedef std::map < int , HouseList > HouseType;
	static HouseType Home;

	std::map < int , DecoHouse * > m_DecorationHouse;
public:
	virtual void OnInitWorld(const char* pWhereLocalWorld);
	virtual void OnPaymentTime();
	virtual bool OnHandleTile(CCharacter* pChr, int IndexCollision);
	
	/* #########################################################################
		FUNCTIONS HOUSES PLANTS
	######################################################################### */
	void ChangePlantsID(int HouseID, int PlantID);

	/* #########################################################################
		FUNCTIONS HOUSES DECORATION
	######################################################################### */
	bool AddDecorationHouse(int DecoID, int OwnerID, vec2 Position);
private:
	bool DeleteDecorationHouse(int ID);
	void ShowDecorationList(CPlayer *pPlayer);

	/* #########################################################################
		GET CHECK HOUSES 
	######################################################################### */
public:
	int GetWorldID(int HouseID) const;
	int GetHouse(vec2 Pos, bool Plants = false);
	int GetHousePrice(int HouseID) const;
	bool GetHouseDoor(int HouseID) const;
	vec2 GetPositionHouse(int HouseID) const;
	int PlayerHouseID(CPlayer *pPlayer) const;
	int OwnerHouseID(int AuthID) const;
	int GetPlantsID(int HouseID) const;

	const char *ClassName(int HouseID) const;
	const char *OwnerName(int HouseID);

	/* #########################################################################
		FUNCTIONS HOUSES 
	######################################################################### */
	bool BuyHouse(int HouseID, CPlayer *pPlayer);
	void SellToHouse(int SellerID, int BuyightID, int Price);
	void SellHouse(int HouseID);

	void TakeFarmMoney(CPlayer *pPlayer, int TakeCount);
	void AddBalance(CPlayer *pPlayer, int Balance);
	
	void CheckTimePayment();
	void ChangeStateDoor(int HouseID);

	/* #########################################################################
		MENUS HOUSES 
	######################################################################### */
	void ShowHouseMenu(CPlayer *pPlayer, int HouseID);
	void ShowPersonalHouse(CPlayer *pPlayer);

	/* #########################################################################
		PARSING HOUSES 
	######################################################################### */
	virtual bool OnVotingMenu(CPlayer *pPlayer, const char *CMD, const int VoteID, const int VoteID2, int Get, const char *GetText);
	virtual bool OnHandleMenulist(CPlayer* pPlayer, int Menulist, bool ReplaceMenu);
};


// - - - - - - - - - - - - - - DOOR HOUSES - - - - - - - - - - - - - - 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class HouseDoor : public CEntity
{
	vec2 m_To;
public:
	HouseDoor(CGameWorld *pGameWorld, vec2 Pos);
	~HouseDoor();

	bool HitCharacter();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};

#endif