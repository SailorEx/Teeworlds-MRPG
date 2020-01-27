/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_QUESTING_PROCESSING_H
#define GAME_CLIENT_COMPONENTS_QUESTING_PROCESSING_H
#include <game/client/component.h>

class CQuestingProcessing : public CComponent
{
	enum
	{
		MAX_TABLE = 4,
	};

	struct QuestingTable
	{
		int m_Have;
		int m_Requires;
		char m_aText[128];
		char m_aIcon[32];
	
		bool TableActive() const 
		{ 
			return m_aText[0] != '\0'; 
		}
	};
	QuestingTable QuestTable[MAX_TABLE];

	void Continue();

public:
	void Clear();
	bool IsActive();

	void ProcessingRenderTable(int TableID, CUIRect& Box);

	virtual void OnRender();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnInput(IInput::CEvent Event);

};

#endif