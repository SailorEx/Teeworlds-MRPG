/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/hash_ctxt.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <teeother/components/localization.h>
#include "AccountMainJob.h"

using namespace sqlstr;
std::map < int, AccountMainJob::StructData > AccountMainJob::ms_aData;
std::map < int, AccountMainJob::StructTempPlayerData > AccountMainJob::ms_aPlayerTempData;

int AccountMainJob::GetHistoryLatestCorrectWorldID(CPlayer* pPlayer) const
{
	const auto pWorldIterator = std::find_if(pPlayer->Acc().m_aHistoryWorld.begin(), pPlayer->Acc().m_aHistoryWorld.end(), [=](int WorldID)
	{
		const int QuestToUnlock = Job()->WorldSwap()->GetNecessaryQuest(WorldID);
		const bool IsValidQuest = Job()->Quest()->IsValidQuest(QuestToUnlock);
		return !Job()->Dungeon()->IsDungeonWorld(WorldID) && ((IsValidQuest && Job()->Quest()->IsCompletedQuest(pPlayer->GetCID(), QuestToUnlock)) || !IsValidQuest);
	});
	return pWorldIterator != pPlayer->Acc().m_aHistoryWorld.end() ? *pWorldIterator : (int)MAIN_WORLD;
}

int AccountMainJob::SendAuthCode(int ClientID, int Code)
{
	if(GS()->IsMmoClient(ClientID))
	{
		CNetMsg_Sv_ClientProgressAuth ProgressMsg;
		ProgressMsg.m_Code = Code;
		GS()->Server()->SendPackMsg(&ProgressMsg, MSGFLAG_VITAL, ClientID);
	}
	return Code;
}

int AccountMainJob::CheckOnlineAccount(int AuthID) const
{
	for (const auto& dt : AccountMainJob::ms_aData)
	{
		if (dt.second.m_AuthID == AuthID && GS()->m_apPlayers[dt.first])
			return dt.first;
	}
	return -1;
}

int AccountMainJob::RegisterAccount(int ClientID, const char *Login, const char *Password)
{
	if(str_length(Login) > 12 || str_length(Login) < 4 || str_length(Password) > 12 || str_length(Password) < 4)
	{
		GS()->Chat(ClientID, "Username / Password must contain 4-12 characters");
		return SendAuthCode(ClientID, AUTH_ALL_MUSTCHAR);
	}
	CSqlString<32> clear_Nick = CSqlString<32>(GS()->Server()->ClientName(ClientID));
	std::shared_ptr<ResultSet> RES2(SJK.SD("ID", "tw_accounts_data", "WHERE Nick = '%s'", clear_Nick.cstr()));
	if(RES2->next())
	{
		GS()->Chat(ClientID, "- - - - [Your nickname is already registered] - - - -");
		GS()->Chat(ClientID, "Your nick is a unique identifier, and it has already been used!");
		GS()->Chat(ClientID, "You can restore access by contacting support, or change nick.");
		GS()->Chat(ClientID, "Discord group \"{STR}\".", g_Config.m_SvDiscordInviteGroup);
		return SendAuthCode(ClientID, AUTH_REGISTER_ERROR_NICK);
	}

	std::shared_ptr<ResultSet> RES4(SJK.SD("ID", "tw_accounts", "ORDER BY ID DESC LIMIT 1"));
	const int InitID = RES4->next() ? RES4->getInt("ID")+1 : 1; // thread save ? hm need for table all time auto increment = 1; NEED FIX IT

	CSqlString<32> clear_Login = CSqlString<32>(Login);
	CSqlString<32> clear_Pass = CSqlString<32>(Password);

	char aAddrStr[64];
	GS()->Server()->GetClientAddr(ClientID, aAddrStr, sizeof(aAddrStr));

	char aSalt[32] = { 0 };
	secure_random_password(aSalt, sizeof(aSalt), 24);

	SJK.ID("tw_accounts", "(ID, Username, Password, PasswordSalt, RegisterDate, RegisteredIP) VALUES ('%d', '%s', '%s', '%s', UTC_TIMESTAMP(), '%s')", InitID, clear_Login.cstr(), HashPassword(clear_Pass.cstr(), aSalt).c_str(), aSalt, aAddrStr);
	SJK.IDS(100, "tw_accounts_data", "(ID, Nick) VALUES ('%d', '%s')", InitID, clear_Nick.cstr());

	GS()->Chat(ClientID, "- - - - - - - [Successful registered] - - - - - - -");
	GS()->Chat(ClientID, "Don't forget your data, have a nice game!");
	GS()->Chat(ClientID, "# Your nickname is a unique identifier!");
	GS()->Chat(ClientID, "# Log in: \"/login {STR} {STR}\"", clear_Login.cstr(), clear_Pass.cstr());
	return SendAuthCode(ClientID, AUTH_REGISTER_GOOD);
}

int AccountMainJob::LoginAccount(int ClientID, const char *Login, const char *Password)
{
	CPlayer *pPlayer = GS()->GetPlayer(ClientID, false);
	if(!pPlayer) 
		return SendAuthCode(ClientID, AUTH_ALL_UNKNOWN);
	
	const int LengthLogin = str_length(Login);
	const int LengthPassword = str_length(Password);
	if(LengthLogin > 12 || LengthLogin < 4 || LengthPassword > 12 || LengthPassword < 4)
	{
		GS()->ChatFollow(ClientID, "Username / Password must contain 4-12 characters");
		return SendAuthCode(ClientID, AUTH_ALL_MUSTCHAR);
	}

	CSqlString<32> clear_Login = CSqlString<32>(Login);
	CSqlString<32> clear_Pass = CSqlString<32>(Password);
	CSqlString<32> clear_Nick = CSqlString<32>(GS()->Server()->ClientName(ClientID));
	std::shared_ptr<ResultSet> ACCOUNTDATA(SJK.SD("*", "tw_accounts_data", "WHERE Nick = '%s'", clear_Nick.cstr()));
	if(ACCOUNTDATA->next())
	{
		const int UserID = ACCOUNTDATA->getInt("ID");
		std::shared_ptr<ResultSet> CHECKACCESS(SJK.SD("ID, LoginDate, Language, Password, PasswordSalt", "tw_accounts", "WHERE Username = '%s' AND ID = '%d'", clear_Login.cstr(), UserID));

		bool LoginSuccess = false;
		if(CHECKACCESS->next())
		{
			if(!str_comp(CHECKACCESS->getString("Password").c_str(), HashPassword(clear_Pass.cstr(), CHECKACCESS->getString("PasswordSalt").c_str()).c_str()))
				LoginSuccess = true;
		}

		if(!LoginSuccess)
		{
			GS()->Chat(ClientID, "Wrong login or password!");
			return SendAuthCode(ClientID, AUTH_LOGIN_WRONG);
		}

		if (CheckOnlineAccount(UserID) >= 0)
		{
			GS()->Chat(ClientID, "The account is already in the game!");
			return SendAuthCode(ClientID, AUTH_LOGIN_ALREADY);
		}

		pPlayer->SetLanguage(CHECKACCESS->getString("Language").c_str());
		str_copy(pPlayer->Acc().m_aLogin, clear_Login.cstr(), sizeof(pPlayer->Acc().m_aLogin));
		str_copy(pPlayer->Acc().m_aLastLogin, CHECKACCESS->getString("LoginDate").c_str(), sizeof(pPlayer->Acc().m_aLastLogin));

		pPlayer->Acc().m_AuthID = UserID;
		pPlayer->Acc().m_Level = ACCOUNTDATA->getInt("Level");
		pPlayer->Acc().m_Exp = ACCOUNTDATA->getInt("Exp");
		pPlayer->Acc().m_GuildID = ACCOUNTDATA->getInt("GuildID");
		pPlayer->Acc().m_Upgrade = ACCOUNTDATA->getInt("Upgrade");
		pPlayer->Acc().m_GuildRank = ACCOUNTDATA->getInt("GuildRank");
		pPlayer->Acc().m_aHistoryWorld.push_front(ACCOUNTDATA->getInt("WorldID"));

		for (const auto& at : CGS::ms_aAttributsInfo)
		{
			if (str_comp_nocase(at.second.m_aFieldName, "unfield") != 0)
				pPlayer->Acc().m_aStats[at.first] = ACCOUNTDATA->getInt(at.second.m_aFieldName);
		}

		GS()->Chat(ClientID, "- - - - - - - [Successful login] - - - - - - -");
		GS()->Chat(ClientID, "Menu is available in call-votes!");
		GS()->m_pController->DoTeamChange(pPlayer, false);

		char aAddrStr[64];
		GS()->Server()->GetClientAddr(ClientID, aAddrStr, sizeof(aAddrStr));
		SJK.UD("tw_accounts", "LoginDate = CURRENT_TIMESTAMP, LoginIP = '%s' WHERE ID = '%d'", aAddrStr, UserID);
		return SendAuthCode(ClientID, AUTH_LOGIN_GOOD);
	}

	GS()->Chat(ClientID, "Your nickname was not found in the database!");
	return SendAuthCode(ClientID, AUTH_LOGIN_NICKNAME);
}

void AccountMainJob::LoadAccount(CPlayer *pPlayer, bool FirstInitilize)
{
	if(!pPlayer || !pPlayer->IsAuthed() || !GS()->IsPlayerEqualWorldID(pPlayer->GetCID()))
		return;

	const int ClientID = pPlayer->GetCID();
	GS()->Broadcast(ClientID, BroadcastPriority::BROADCAST_MAIN_INFORMATION, 200, "You are located {STR} ({STR})", 
		GS()->Server()->GetWorldName(GS()->GetWorldID()), (GS()->IsAllowedPVP() ? "Zone PVP" : "Safe zone"));

	GS()->SendWorldMusic(ClientID, (GS()->IsDungeon() ? -1 : 0));
	if(!FirstInitilize)
	{
		const int CountMessageInbox = Job()->Inbox()->GetActiveInbox(pPlayer);
		if (CountMessageInbox > 0)
			GS()->Chat(ClientID, "You have {INT} unread messages!", &CountMessageInbox);

		GS()->ResetVotes(ClientID, MenuList::MAIN_MENU);
		GS()->SendCompleteEquippingItems(ClientID);
		return;
	}

	Job()->OnInitAccount(ClientID);
	const int Rank = GetRank(pPlayer->Acc().m_AuthID);
	GS()->Chat(-1, "{STR} logged to account. Rank #{INT}", GS()->Server()->ClientName(ClientID), &Rank);
#ifdef CONF_DISCORD
	char pMsg[256], pLoggin[64];
	str_format(pLoggin, sizeof(pLoggin), "%s logged in Account ID %d", GS()->Server()->ClientName(ClientID), pPlayer->Acc().m_AuthID);
	str_format(pMsg, sizeof(pMsg), "?player=%s&rank=%d&dicid=%d",
		GS()->Server()->ClientName(ClientID), Rank, pPlayer->GetEquippedItemID(EQUIP_DISCORD));
	GS()->Server()->SendDiscordGenerateMessage("16757248", pLoggin, pMsg);
#endif

	if (!pPlayer->GetItem(itHammer).m_Count)
	{
		pPlayer->GetItem(itHammer).Add(1);
		GS()->Chat(ClientID, "Quest NPCs are marked with an aura Heart and Shield.");
		GS()->Chat(ClientID, "Shield around you indicates location of active quest.");
	}
	
	// settings
	if(!pPlayer->GetItem(itModePVP).m_Count)
		pPlayer->GetItem(itModePVP).Add(1, 1);

	pPlayer->GetTempData().m_TempSafeSpawn = true;

	int LatestCorrectWorldID = GetHistoryLatestCorrectWorldID(pPlayer);
	if(LatestCorrectWorldID != GS()->GetWorldID())
	{
		pPlayer->ChangeWorld(LatestCorrectWorldID);
		return;
	}
	GS()->SendCompleteEquippingItems(ClientID);
}

void AccountMainJob::DiscordConnect(int ClientID, const char *pDID)
{
#ifdef CONF_DISCORD
	CPlayer *pPlayer = GS()->GetPlayer(ClientID, true);
	if(!pPlayer) return;	

	CSqlString<64> cDID = CSqlString<64>(pDID);
	SJK.UD("tw_accounts_data", "DiscordID = '%s' WHERE ID = '%d'", cDID.cstr(), pPlayer->Acc().m_AuthID);

	GS()->Chat(ClientID, "Update DiscordID.");
	GS()->Chat(ClientID, "Check connect status in Discord \"!mconnect\".");
#endif
}

int AccountMainJob::GetRank(int AuthID)
{
	int Rank = 0;
	std::shared_ptr<ResultSet> RES(SJK.SD("ID", "tw_accounts_data", "ORDER BY Level DESC, Exp DESC"));
	while(RES->next())
	{
		Rank++;
		int SelectedAuthID = RES->getInt("ID");
		if(AuthID == SelectedAuthID) 
			return Rank;
	}
	return -1;
}

bool AccountMainJob::OnHandleMenulist(CPlayer* pPlayer, int Menulist, bool ReplaceMenu)
{
	const int ClientID = pPlayer->GetCID();
	if (ReplaceMenu)
	{
		return false;
	}

	// settings
	if (Menulist == MenuList::MENU_SETTINGS)
	{
		pPlayer->m_LastVoteMenu = MenuList::MAIN_MENU;

		// settings
		GS()->AVH(ClientID, TAB_SETTINGS, RED_COLOR, "Some of the settings becomes valid after death");
		GS()->AVM(ClientID, "MENU", MenuList::MENU_SELECT_LANGUAGE, TAB_SETTINGS, "Settings language");
		for (const auto& it : InventoryJob::ms_aItems[ClientID])
		{
			const InventoryItem ItemData = it.second;
			if (ItemData.Info().m_Type == ItemType::TYPE_SETTINGS && ItemData.m_Count > 0)
				GS()->AVM(ClientID, "ISETTINGS", it.first, TAB_SETTINGS, "[{STR}] {STR}", (ItemData.m_Settings ? "Enable" : "Disable"), ItemData.Info().GetName(pPlayer));
		}

		// equipment modules
		bool IsFoundModules = false;
		GS()->AV(ClientID, "null");
		GS()->AVH(ClientID, TAB_SETTINGS_MODULES, GREEN_COLOR, "Modules settings");
		for (const auto& it : InventoryJob::ms_aItems[ClientID])
		{
			const InventoryItem ItemData = it.second;
			if (ItemData.Info().m_Type == ItemType::TYPE_MODULE && ItemData.m_Count > 0)
			{
				char aAttributes[128];
				ItemData.FormatAttributes(aAttributes, sizeof(aAttributes));
				GS()->AVMI(ClientID, ItemData.Info().GetIcon(), "ISETTINGS", it.first, TAB_SETTINGS_MODULES, "{STR} {STR}{STR}",
					ItemData.Info().GetName(pPlayer), aAttributes, (ItemData.m_Settings ? "✔" : "\0"));
				IsFoundModules = true;
			}
		}

		// if no modules are found
		if (!IsFoundModules)
			GS()->AVM(ClientID, "null", NOPE, TAB_SETTINGS_MODULES, "The list of modules equipment is empty.");
	
		GS()->AddBackpage(ClientID);
		return true;
	}

	// language selection
	if (Menulist == MenuList::MENU_SELECT_LANGUAGE)
	{
		pPlayer->m_LastVoteMenu = MenuList::MENU_SETTINGS;
		GS()->AVH(ClientID, TAB_INFO_LANGUAGES, GREEN_COLOR, "Languages Information");
		GS()->AVM(ClientID, "null", NOPE, TAB_INFO_LANGUAGES, "Here you can choose the language.");
		GS()->AVM(ClientID, "null", NOPE, TAB_INFO_LANGUAGES, "Note: translation is not complete.");
		GS()->AV(ClientID, "null");

		const char* pPlayerLanguage = pPlayer->GetLanguage();
		GS()->AVH(ClientID, TAB_LANGUAGES, GRAY_COLOR, "Active language: [{STR}]", pPlayerLanguage);
		for(int i = 0; i < GS()->Server()->Localization()->m_pLanguages.size(); i++)
		{
			// do not show the language already selected by the player in the selection lists
			if(str_comp(pPlayerLanguage, GS()->Server()->Localization()->m_pLanguages[i]->GetFilename()) == 0)
				continue;

			// add language selection
			const char *pLanguageName = GS()->Server()->Localization()->m_pLanguages[i]->GetName();
			GS()->AVM(ClientID, "SELECTLANGUAGE", i, TAB_LANGUAGES, "Select language \"{STR}\"", pLanguageName);
		}
		GS()->AddBackpage(ClientID);
		return true;
	}
	return false;
}

bool AccountMainJob::OnParsingVoteCommands(CPlayer* pPlayer, const char* CMD, const int VoteID, const int VoteID2, int Get, const char* GetText)
{
	const int ClientID = pPlayer->GetCID();
	if (PPSTR(CMD, "SELECTLANGUAGE") == 0)
	{
		const char *pSelectedLanguage = GS()->Server()->Localization()->m_pLanguages[VoteID]->GetFilename();
		pPlayer->SetLanguage(pSelectedLanguage);
		GS()->Chat(ClientID, "You chosen a language \"{STR}\".", pSelectedLanguage);
		GS()->UpdateVotes(ClientID, MenuList::MENU_SELECT_LANGUAGE);
		Job()->SaveAccount(pPlayer, SaveType::SAVE_LANGUAGE);
		return true;
	}
	return false;
}

void AccountMainJob::OnResetClient(int ClientID)
{
	if(ms_aPlayerTempData.find(ClientID) != ms_aPlayerTempData.end())
		ms_aPlayerTempData.erase(ClientID);

	if (ms_aData.find(ClientID) != ms_aData.end())
		ms_aData.erase(ClientID);
}

std::string AccountMainJob::HashPassword(const char* pPassword, const char* pSalt)
{
	char aPlaintext[128] = { 0 };
	SHA256_CTX Sha256Ctx;
	sha256_init(&Sha256Ctx);
	str_format(aPlaintext, sizeof(aPlaintext), "%s%s%s", pSalt, pPassword, pSalt);
	sha256_update(&Sha256Ctx, aPlaintext, str_length(aPlaintext));
	SHA256_DIGEST Digest = sha256_finish(&Sha256Ctx);

	char aHash[SHA256_MAXSTRSIZE];
	sha256_str(Digest, aHash, sizeof(aHash));
	return std::string(aHash);
}