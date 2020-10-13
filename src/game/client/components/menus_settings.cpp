/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/color.h>
#include <base/math.h>

#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/external/json-parser/json.h>
#include <engine/shared/config.h>

#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/components/maplayers.h>
#include <game/client/components/sounds.h>
#include <game/client/components/stats.h>
#include <game/client/ui.h>
#include <game/client/render.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>

#include "binds.h"
#include "countryflags.h"
#include "menus.h"

CMenusKeyBinder::CMenusKeyBinder()
{
	m_TakeKey = false;
	m_GotKey = false;
}

bool CMenusKeyBinder::OnInput(IInput::CEvent Event)
{
	if(m_TakeKey)
	{
		int TriggeringEvent = (Event.m_Key == KEY_MOUSE_1) ? IInput::FLAG_PRESS : IInput::FLAG_RELEASE;
		if(Event.m_Flags&TriggeringEvent) // delay to RELEASE to support composed binds
		{
			m_Key = Event;
			m_GotKey = true;
			m_TakeKey = false;

			int Mask = CBinds::GetModifierMask(Input()); // always > 0
			m_Modifier = 0;
			while(!(Mask&1)) // this computes a log2, we take the first modifier flag in mask.
			{
				Mask >>= 1;
				m_Modifier++;
			}
			// prevent from adding e.g. a control modifier to lctrl
			if(CBinds::ModifierMatchesKey(m_Modifier, Event.m_Key))
				m_Modifier = 0;
		}
		return true;
	}

	return false;
}

void CMenus::RenderHSLPicker(CUIRect MainView)
{
	CUIRect Label, Button, Picker, Sliders;

	// background
	float Spacing = 2.0f;

	if(!(*CSkins::ms_apUCCVariables[m_TeePartSelected]))
		return;

	MainView.HSplitTop(Spacing, 0, &MainView);

	bool Modified = false;
	bool UseAlpha = m_TeePartSelected == SKINPART_MARKING;
	int Color = *CSkins::ms_apColorVariables[m_TeePartSelected];

	int Hue, Sat, Lgt, Alp;
	Hue = (Color>>16)&0xff;
	Sat = (Color>>8)&0xff;
	Lgt = Color&0xff;
	if(UseAlpha)
		Alp = (Color>>24)&0xff;

	MainView.VSplitMid(&Picker, &Sliders, 5.0f);
	float PickerSize = min(Picker.w, Picker.h) - 20.0f;
	RenderTools()->DrawUIRect(&Picker, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	float Dark = CSkins::DARKEST_COLOR_LGT/255.0f;
	IGraphics::CColorVertex ColorArray[4];

	// Hue/Lgt picker :
	{
		Picker.VMargin((Picker.w - PickerSize) / 2.0f, &Picker);
		Picker.HMargin((Picker.h - PickerSize) / 2.0f, &Picker);

		// picker
		Graphics()->TextureClear();
		Graphics()->QuadsBegin();

		// base: grey - hue
		vec3 c = HslToRgb(vec3(Hue/255.0f, 0.0f, 0.5f));
		ColorArray[0] = IGraphics::CColorVertex(0, c.r, c.g, c.b, 1.0f);
		c = HslToRgb(vec3(Hue/255.0f, 1.0f, 0.5f));
		ColorArray[1] = IGraphics::CColorVertex(1, c.r, c.g, c.b, 1.0f);
		c = HslToRgb(vec3(Hue/255.0f, 1.0f, 0.5f));
		ColorArray[2] = IGraphics::CColorVertex(2, c.r, c.g, c.b, 1.0f);
		c = HslToRgb(vec3(Hue/255.0f, 0.0f, 0.5f));
		ColorArray[3] = IGraphics::CColorVertex(3, c.r, c.g, c.b, 1.0f);
		Graphics()->SetColorVertex(ColorArray, 4);
		IGraphics::CQuadItem QuadItem(Picker.x, Picker.y, Picker.w, Picker.h);
		Graphics()->QuadsDrawTL(&QuadItem, 1);

		// white blending
		ColorArray[0] = IGraphics::CColorVertex(0, 0.0f, 0.0f, 0.0f, 0.0f);
		ColorArray[1] = IGraphics::CColorVertex(1, 0.0f, 0.0f, 0.0f, 0.0f);
		ColorArray[2] = IGraphics::CColorVertex(2, 1.0f, 1.0f, 1.0f, 1.0f);
		ColorArray[3] = IGraphics::CColorVertex(3, 1.0f, 1.0f, 1.0f, 1.0f);
		Graphics()->SetColorVertex(ColorArray, 4);
		IGraphics::CQuadItem WhiteGradient(Picker.x, Picker.y + Picker.h*(1-2*Dark)/((1-Dark)*2), Picker.w, Picker.h/((1-Dark)*2));
		Graphics()->QuadsDrawTL(&WhiteGradient, 1);

		// black blending
		ColorArray[0] = IGraphics::CColorVertex(0, 0.0f, 0.0f, 0.0f, 1.0f-2*Dark);
		ColorArray[1] = IGraphics::CColorVertex(1, 0.0f, 0.0f, 0.0f, 1.0f-2*Dark);
		ColorArray[2] = IGraphics::CColorVertex(2, 0.0f, 0.0f, 0.0f, 0.0f);
		ColorArray[3] = IGraphics::CColorVertex(3, 0.0f, 0.0f, 0.0f, 0.0f);
		Graphics()->SetColorVertex(ColorArray, 4);
		IGraphics::CQuadItem BlackGradient(Picker.x, Picker.y, Picker.w, Picker.h*(1-2*Dark)/((1-Dark)*2));
		Graphics()->QuadsDrawTL(&BlackGradient, 1);

		Graphics()->QuadsEnd();

		// marker
		vec2 Marker = vec2(Sat / 255.0f * PickerSize, Lgt / 255.0f * PickerSize);
		Graphics()->TextureClear();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
		IGraphics::CQuadItem aMarker[2];
		aMarker[0] = IGraphics::CQuadItem(Picker.x+Marker.x, Picker.y+Marker.y - 5.0f*UI()->PixelSize(), UI()->PixelSize(), 11.0f*UI()->PixelSize());
		aMarker[1] = IGraphics::CQuadItem(Picker.x+Marker.x - 5.0f*UI()->PixelSize(), Picker.y+Marker.y, 11.0f*UI()->PixelSize(), UI()->PixelSize());
		Graphics()->QuadsDrawTL(aMarker, 2);
		Graphics()->QuadsEnd();

		// logic
		float X, Y;
		static int s_HLPicker;
		if(UI()->DoPickerLogic(&s_HLPicker, &Picker, &X, &Y))
		{
			Sat = (int)(255.0f*X/Picker.w);
			Lgt = (int)(255.0f*Y/Picker.h);
			Modified = true;
		}
	}

	// H/S/L/A sliders :
	{
		int NumBars = UseAlpha ? 4 : 3;
		const char *const apNames[4] = {Localize("Hue:"), Localize("Sat:"), Localize("Lgt:"), Localize("Alp:")};
		int *const apVars[4] = {&Hue, &Sat, &Lgt, &Alp};
		static CButtonContainer s_aButtons[12];
		float SectionHeight = 40.0f;
		float SliderHeight = 16.0f;
		static const float s_aColorIndices[7][3] = {{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
													{0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};

		for(int i = 0; i < NumBars; i++)
		{
			CUIRect Bar, Section;

			Sliders.HSplitTop(SectionHeight, &Section, &Sliders);
			Sliders.HSplitTop(Spacing, 0, &Sliders);
			RenderTools()->DrawUIRect(&Section, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

			Section.HSplitTop(SectionHeight - SliderHeight, &Label, &Section);

			// label
			Label.VSplitMid(&Label, &Button, 0.0f);
			Label.y += 4.0f;
			UI()->DoLabel(&Label, apNames[i], SliderHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

			// value label
			char aBuf[16];
			str_format(aBuf, sizeof(aBuf), "%d", *apVars[i]);
			Button.y += 4.0f;
			UI()->DoLabel(&Button, aBuf, SliderHeight * ms_FontmodHeight * 0.8f, CUI::ALIGN_CENTER);

			// button <
			Section.VSplitLeft(SliderHeight, &Button, &Bar);
			if(DoButton_Menu(&s_aButtons[i*3], "<", 0, &Button, 0, CUI::CORNER_TL|CUI::CORNER_BL))
			{
				*apVars[i] = max(0, *apVars[i]-1);
				Modified = true;
			}

			// bar
			Bar.VSplitLeft(Section.w - SliderHeight * 2, &Bar, &Button);
			int NumQuads = 1;
			if(i == 0)
				NumQuads = 6;
			else if(i == 2)
				NumQuads = 2;
			else
				NumQuads = 1;

			float Length = Bar.w/NumQuads;
			float Offset = Length;
			vec4 ColorL, ColorR;

			Graphics()->TextureClear();
			Graphics()->QuadsBegin();
			for(int j = 0; j < NumQuads; ++j)
			{
				switch(i)
				{
				case 0: // Hue
					{
						ColorL = vec4(s_aColorIndices[j][0], s_aColorIndices[j][1], s_aColorIndices[j][2], 1.0f);
						ColorR = vec4(s_aColorIndices[j+1][0], s_aColorIndices[j+1][1], s_aColorIndices[j+1][2], 1.0f);
					}
					break;
				case 1: // Sat
					{
						vec3 c = HslToRgb(vec3(Hue/255.0f, 0.0f, Dark+Lgt/255.0f*(1.0f-Dark)));
						ColorL = vec4(c.r, c.g, c.b, 1.0f);
						c = HslToRgb(vec3(Hue/255.0f, 1.0f, Dark+Lgt/255.0f*(1.0f-Dark)));
						ColorR = vec4(c.r, c.g, c.b, 1.0f);
					}
					break;
				case 2: // Lgt
					{
						if(j == 0)
						{
							// Dark - 0.5f
							vec3 c = HslToRgb(vec3(Hue/255.0f, Sat/255.0f, Dark));
							ColorL = vec4(c.r, c.g, c.b, 1.0f);
							c = HslToRgb(vec3(Hue/255.0f, Sat/255.0f, 0.5f));
							ColorR = vec4(c.r, c.g, c.b, 1.0f);
							Length = Offset = Bar.w - Bar.w/((1-Dark)*2);
						}
						else
						{
							// 0.5f - 0.0f
							vec3 c = HslToRgb(vec3(Hue/255.0f, Sat/255.0f, 0.5f));
							ColorL = vec4(c.r, c.g, c.b, 1.0f);
							c = HslToRgb(vec3(Hue/255.0f, Sat/255.0f, 1.0f));
							ColorR = vec4(c.r, c.g, c.b, 1.0f);
							Length = Bar.w/((1-Dark)*2);
						}
					}
					break;
				default: // Alpha
					{
						vec3 c = HslToRgb(vec3(Hue/255.0f, Sat/255.0f, Dark+Lgt/255.0f*(1.0f-Dark)));
						ColorL = vec4(c.r, c.g, c.b, 0.0f);
						ColorR = vec4(c.r, c.g, c.b, 1.0f);
					}
				}

				ColorArray[0] = IGraphics::CColorVertex(0, ColorL.r, ColorL.g, ColorL.b, ColorL.a);
				ColorArray[1] = IGraphics::CColorVertex(1, ColorR.r, ColorR.g, ColorR.b, ColorR.a);
				ColorArray[2] = IGraphics::CColorVertex(2, ColorR.r, ColorR.g, ColorR.b, ColorR.a);
				ColorArray[3] = IGraphics::CColorVertex(3, ColorL.r, ColorL.g, ColorL.b, ColorL.a);
				Graphics()->SetColorVertex(ColorArray, 4);
				IGraphics::CQuadItem QuadItem(Bar.x+Offset*j, Bar.y, Length, Bar.h);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
			}

			// bar marker
			Graphics()->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
			IGraphics::CQuadItem QuadItem(Bar.x + min(127.0f, *apVars[i]/2.0f), Bar.y, UI()->PixelSize(), Bar.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();

			// button >
			Button.VSplitLeft(SliderHeight, &Button, &Label);
			if(DoButton_Menu(&s_aButtons[i*3+1], ">", 0, &Button, 0, CUI::CORNER_TR|CUI::CORNER_BR))
			{
				*apVars[i] = min(255, *apVars[i]+1);
				Modified = true;
			}

			// logic
			float X;
			if(UI()->DoPickerLogic(&s_aButtons[i*3+2], &Bar, &X, 0))
			{
				*apVars[i] = X*255.0f/Bar.w;
				Modified = true;
			}
		}
	}

	if(Modified)
	{
		int NewVal = (Hue << 16) + (Sat << 8) + Lgt;
		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			if(m_TeePartSelected == p)
				*CSkins::ms_apColorVariables[p] = NewVal;
		}
		if(UseAlpha)
			g_Config.m_PlayerColorMarking = (Alp << 24) + NewVal;
		m_SkinModified = true;
	}
}

void CMenus::RenderSkinSelection(CUIRect MainView)
{
	static float s_LastSelectionTime = -10.0f;
	static sorted_array<const CSkins::CSkin*> s_paSkinList;
	static CListBox s_ListBox;
	if(m_RefreshSkinSelector)
	{
		s_paSkinList.clear();
		for(int i = 0; i < m_pClient->m_pSkins->Num(); ++i)
		{
			const CSkins::CSkin *s = m_pClient->m_pSkins->Get(i);
			// no special skins
			if((s->m_Flags & CSkins::SKINFLAG_SPECIAL) == 0 && s_ListBox.FilterMatches(s->m_aName))
			{
				s_paSkinList.add(s);
			}
		}
		m_RefreshSkinSelector = false;
	}

	m_pSelectedSkin = 0;
	int OldSelected = -1;
	s_ListBox.DoHeader(&MainView, Localize("Skins"), GetListHeaderHeight());
	m_RefreshSkinSelector = s_ListBox.DoFilter();
	s_ListBox.DoStart(60.0f, s_paSkinList.size(), 10, 1, OldSelected);

	for(int i = 0; i < s_paSkinList.size(); ++i)
	{
		const CSkins::CSkin* s = s_paSkinList[i];
		if(s == 0)
			continue;
		if(!str_comp(s->m_aName, g_Config.m_PlayerSkin))
		{
			m_pSelectedSkin = s;
			OldSelected = i;
		}

		CListboxItem Item = s_ListBox.DoNextItem(&s_paSkinList[i], OldSelected == i);
		if(Item.m_Visible)
		{
			CTeeRenderInfo Info;
			for(int p = 0; p < NUM_SKINPARTS; p++)
			{
				if(s->m_aUseCustomColors[p])
				{
					Info.m_aTextures[p] = s->m_apParts[p]->m_ColorTexture;
					Info.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(s->m_aPartColors[p], p == SKINPART_MARKING);
				}
				else
				{
					Info.m_aTextures[p] = s->m_apParts[p]->m_OrgTexture;
					Info.m_aColors[p] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
				}
			}

			Info.m_Size = 50.0f;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			{
				// interactive tee: tee is happy to be selected
				int TeeEmote = (Item.m_Selected && s_LastSelectionTime + 0.75f > Client()->LocalTime()) ? EMOTE_HAPPY : EMOTE_NORMAL;
				RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, TeeEmote, vec2(1.0f, 0.0f), vec2(Item.m_Rect.x + Item.m_Rect.w / 2, Item.m_Rect.y + Item.m_Rect.h / 2));
			}

			CUIRect Label;
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitBottom(10.0f, &Item.m_Rect, &Label);

			if(Item.m_Selected)
			{
				TextRender()->TextColor(CUI::ms_HighlightTextColor);
				TextRender()->TextOutlineColor(CUI::ms_HighlightTextOutlineColor);
			}
			UI()->DoLabel(&Label, s->m_aName, 10.0f, CUI::ALIGN_CENTER);
			if(Item.m_Selected)
			{
				TextRender()->TextColor(CUI::ms_DefaultTextColor);
				TextRender()->TextOutlineColor(CUI::ms_DefaultTextOutlineColor);
			}
		}
	}

	const int NewSelected = s_ListBox.DoEnd();
	if(NewSelected != -1 && NewSelected != OldSelected)
	{
		s_LastSelectionTime = Client()->LocalTime();
		m_pSelectedSkin = s_paSkinList[NewSelected];
		mem_copy(g_Config.m_PlayerSkin, m_pSelectedSkin->m_aName, sizeof(g_Config.m_PlayerSkin));
		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			mem_copy(CSkins::ms_apSkinVariables[p], m_pSelectedSkin->m_apParts[p]->m_aName, MAX_SKIN_LENGTH);
			*CSkins::ms_apUCCVariables[p] = m_pSelectedSkin->m_aUseCustomColors[p];
			*CSkins::ms_apColorVariables[p] = m_pSelectedSkin->m_aPartColors[p];
		}
		m_SkinModified = true;
	}
	OldSelected = NewSelected;
}

void CMenus::RenderSkinPartSelection(CUIRect MainView)
{
	static bool s_InitSkinPartList = true;
	static sorted_array<const CSkins::CSkinPart*> s_paList[6];
	static CListBox s_ListBox;
	if(s_InitSkinPartList)
	{
		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			s_paList[p].clear();
			for(int i = 0; i < m_pClient->m_pSkins->NumSkinPart(p); ++i)
			{
				const CSkins::CSkinPart *s = m_pClient->m_pSkins->GetSkinPart(p, i);
				// no special skins
				if((s->m_Flags & CSkins::SKINFLAG_SPECIAL) == 0 && s_ListBox.FilterMatches(s->m_aName))
				{
					s_paList[p].add(s);
				}
			}
		}
		s_InitSkinPartList = false;
	}

	static int OldSelected = -1;
	s_ListBox.DoHeader(&MainView, Localize(CSkins::ms_apSkinPartNames[m_TeePartSelected]), GetListHeaderHeight());
	s_InitSkinPartList = s_ListBox.DoFilter();
	s_ListBox.DoStart(50.0f, s_paList[m_TeePartSelected].size(), 5, 1, OldSelected);

	for(int i = 0; i < s_paList[m_TeePartSelected].size(); ++i)
	{
		const CSkins::CSkinPart* s = s_paList[m_TeePartSelected][i];
		if(s == 0)
			continue;
		if(!str_comp(s->m_aName, CSkins::ms_apSkinVariables[m_TeePartSelected]))
			OldSelected = i;

		CListboxItem Item = s_ListBox.DoNextItem(&s_paList[m_TeePartSelected][i], OldSelected == i);
		if(Item.m_Visible)
		{
			CTeeRenderInfo Info;
			for(int j = 0; j < NUM_SKINPARTS; j++)
			{
				int SkinPart = m_pClient->m_pSkins->FindSkinPart(j, CSkins::ms_apSkinVariables[j], false);
				const CSkins::CSkinPart* pSkinPart = m_pClient->m_pSkins->GetSkinPart(j, SkinPart);
				if(*CSkins::ms_apUCCVariables[j])
				{
					if(m_TeePartSelected == j)
						Info.m_aTextures[j] = s->m_ColorTexture;
					else
						Info.m_aTextures[j] = pSkinPart->m_ColorTexture;
					Info.m_aColors[j] = m_pClient->m_pSkins->GetColorV4(*CSkins::ms_apColorVariables[j], j == SKINPART_MARKING);
				}
				else
				{
					if(m_TeePartSelected == j)
						Info.m_aTextures[j] = s->m_OrgTexture;
					else
						Info.m_aTextures[j] = pSkinPart->m_OrgTexture;
					Info.m_aColors[j] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
				}
			}
			Info.m_Size = 50.0f;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			const vec2 TeePos(Item.m_Rect.x + Item.m_Rect.w / 2, Item.m_Rect.y + Item.m_Rect.h / 2);

			if(m_TeePartSelected == SKINPART_HANDS)
			{
				RenderTools()->RenderTeeHand(&Info, TeePos, vec2(1.0f, 0.0f), -pi * 0.5f, vec2(18, 0));
			}
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, 0, vec2(1.0f, 0.0f), TeePos);
		}
	}

	const int NewSelected = s_ListBox.DoEnd();
	if(NewSelected != -1 && NewSelected != OldSelected)
	{
		const CSkins::CSkinPart* s = s_paList[m_TeePartSelected][NewSelected];
		mem_copy(CSkins::ms_apSkinVariables[m_TeePartSelected], s->m_aName, MAX_SKIN_LENGTH);
		g_Config.m_PlayerSkin[0] = 0;
		m_SkinModified = true;
	}
	OldSelected = NewSelected;
}

void CMenus::RenderSkinPartPalette(CUIRect MainView)
{
	if (!*CSkins::ms_apUCCVariables[m_TeePartSelected])
		return; // color selection not open

	float ButtonHeight = 20.0f;
	float Width = MainView.w;
	float Margin = 5.0f;
	CUIRect Button;

	// palette
	MainView.HSplitBottom(ButtonHeight / 2.0f, &MainView, 0);
	MainView.HSplitBottom(ButtonHeight + 2 * Margin, &MainView, &MainView);
	for (int p = 0; p < NUM_SKINPARTS; p++)
	{
		MainView.VSplitLeft(Width / NUM_SKINPARTS, &Button, &MainView);

		// no palette if color is unused for this skin parts
		static int s_aColorPalettes[NUM_SKINPARTS];
		if (*CSkins::ms_apUCCVariables[p])
		{
			float HMargin = (Button.w - (ButtonHeight + 2 * Margin)) / 2.0f;
			Button.VSplitLeft(HMargin, 0, &Button);
			Button.VSplitRight(HMargin, &Button, 0);

			vec4 PartColor = m_pClient->m_pSkins->GetColorV4(*CSkins::ms_apColorVariables[p], p == SKINPART_MARKING);

			bool Hovered = UI()->HotItem() == &s_aColorPalettes[p];
			bool Clicked = UI()->DoButtonLogic(&s_aColorPalettes[p], &Button);
			bool Selected = m_TeePartSelected == p;
			if (Selected)
			{
				CUIRect Underline = { Button.x, Button.y + ButtonHeight + 2 * Margin + 2.0f, Button.w, 1.0f };
				RenderTools()->DrawUIRect(&Underline, vec4(1.0f, 1.0f, 1.0f, 1.5f), 0, 0);
			}
			RenderTools()->DrawUIRect(&Button, (Hovered ? vec4(1.0f, 1.0f, 1.0f, 0.25f) : vec4(0.0f, 0.0f, 0.0f, 0.25f)), CUI::CORNER_ALL, 5.0f);
			Button.Margin(Margin, &Button);
			RenderTools()->DrawUIRect(&Button, PartColor, CUI::CORNER_ALL, 3.0f);
			if (Clicked && p != m_TeePartSelected)
			{
				int& TeePartSelectedColor = *CSkins::ms_apColorVariables[m_TeePartSelected];
				TeePartSelectedColor = *CSkins::ms_apColorVariables[p];
				TeePartSelectedColor -= ((TeePartSelectedColor >> 24) & 0xff) << 24; // remove any alpha
				if (m_TeePartSelected == SKINPART_MARKING)
					TeePartSelectedColor += 0xff << 24; // force full alpha
			}
		}
	}
}

class CLanguage
{
public:
	CLanguage() {}
	CLanguage(const char *n, const char *f, int Code) : m_Name(n), m_FileName(f), m_CountryCode(Code) {}

	string m_Name;
	string m_FileName;
	int m_CountryCode;

	bool operator<(const CLanguage &Other) { return m_Name < Other.m_Name; }
};


int CMenus::ThemeScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	CMenus *pSelf = (CMenus *)pUser;
	const char *pSuffix = str_endswith(pName, ".map");
	if(IsDir || !pSuffix)
		return 0;
	char aFullName[128];
	char aThemeName[128];
	str_truncate(aFullName, sizeof(aFullName), pName, pSuffix - pName);

	bool IsDay = false;
	bool IsNight = false;
	if((pSuffix = str_endswith(aFullName, "_day")))
	{
		str_truncate(aThemeName, sizeof(aThemeName), pName, pSuffix - aFullName);
		IsDay = true;
	}
	else if((pSuffix = str_endswith(aFullName, "_night")))
	{
		str_truncate(aThemeName, sizeof(aThemeName), pName, pSuffix - aFullName);
		IsNight = true;
	}
	else
		str_copy(aThemeName, aFullName, sizeof(aThemeName));

	// "none" and "auto" are reserved, disallowed for maps
	if(str_comp(aThemeName, "none") == 0 || str_comp(aThemeName, "auto") == 0)
		return 0;

	// try to edit an existing theme
	for(int i = 0; i < pSelf->m_lThemes.size(); i++)
	{
		if(str_comp(pSelf->m_lThemes[i].m_Name, aThemeName) == 0)
		{
			if(IsDay)
				pSelf->m_lThemes[i].m_HasDay = true;
			if(IsNight)
				pSelf->m_lThemes[i].m_HasNight = true;
			return 0;
		}
	}

	// make new theme
	CTheme Theme(aThemeName, IsDay, IsNight);
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "added theme %s from ui/themes/%s", aThemeName, pName);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
	pSelf->m_lThemes.add(Theme);
	return 0;
}

int CMenus::ThemeIconScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	CMenus *pSelf = (CMenus *)pUser;
	const char *pSuffix = str_endswith(pName, ".png");
	if(IsDir || !pSuffix)
		return 0;

	char aThemeName[128];
	str_truncate(aThemeName, sizeof(aThemeName), pName, pSuffix - pName);

	// save icon for an existing theme
	for(sorted_array<CTheme>::range r = pSelf->m_lThemes.all(); !r.empty(); r.pop_front()) // bit slow but whatever
	{
		if(str_comp(r.front().m_Name, aThemeName) == 0 || (!r.front().m_Name[0] && str_comp(aThemeName, "none") == 0))
		{
			char aBuf[512];
			str_format(aBuf, sizeof(aBuf), "ui/themes/%s", pName);
			CImageInfo Info;
			if(!pSelf->Graphics()->LoadPNG(&Info, aBuf, DirType))
			{
				str_format(aBuf, sizeof(aBuf), "failed to load theme icon from %s", pName);
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
				return 0;
			}
			str_format(aBuf, sizeof(aBuf), "loaded theme icon %s", pName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);

			r.front().m_IconTexture = pSelf->Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);
			return 0;
		}
	}
	return 0; // no existing theme
}

void LoadLanguageIndexfile(IStorage *pStorage, IConsole *pConsole, sorted_array<CLanguage> *pLanguages)
{
	// read file data into buffer
	const char *pFilename = "languages/index.json";
	IOHANDLE File = pStorage->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", "couldn't open index file");
		return;
	}
	int FileSize = (int)io_length(File);
	char *pFileData = (char *)mem_alloc(FileSize, 1);
	io_read(File, pFileData, FileSize);
	io_close(File);

	// parse json data
	json_settings JsonSettings;
	mem_zero(&JsonSettings, sizeof(JsonSettings));
	char aError[256];
	json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, FileSize, aError);
	mem_free(pFileData);

	if(pJsonData == 0)
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, pFilename, aError);
		return;
	}

	// extract data
	const json_value &rStart = (*pJsonData)["language indices"];
	if(rStart.type == json_array)
	{
		for(unsigned i = 0; i < rStart.u.array.length; ++i)
		{
			char aFileName[128];
			str_format(aFileName, sizeof(aFileName), "languages/%s.json", (const char *)rStart[i]["file"]);
			pLanguages->add(CLanguage((const char *)rStart[i]["name"], aFileName, (json_int_t)rStart[i]["code"]));
		}
	}

	// clean up
	json_value_free(pJsonData);
}

void CMenus::RenderLanguageSelection(CUIRect MainView, bool Header)
{
	static int s_SelectedLanguage = -1;
	static sorted_array<CLanguage> s_Languages;
	static CListBox s_ListBox;

	if(s_Languages.size() == 0)
	{
		s_Languages.add(CLanguage("English", "", 826));
		LoadLanguageIndexfile(Storage(), Console(), &s_Languages);
		for(int i = 0; i < s_Languages.size(); i++)
		{
			if(str_comp(s_Languages[i].m_FileName, g_Config.m_ClLanguagefile) == 0)
			{
				s_SelectedLanguage = i;
				break;
			}
		}
	}

	int OldSelected = s_SelectedLanguage;

	if(Header)
		s_ListBox.DoHeader(&MainView, Localize("Language"), GetListHeaderHeight());

	bool IsActive = m_ActiveListBox == ACTLB_LANG;
	s_ListBox.DoStart(20.0f, s_Languages.size(), 1, 3, s_SelectedLanguage, Header ? 0 : &MainView, Header, &IsActive);

	for(sorted_array<CLanguage>::range r = s_Languages.all(); !r.empty(); r.pop_front())
	{
		CListboxItem Item = s_ListBox.DoNextItem(&r.front(), s_SelectedLanguage != -1 && !str_comp(s_Languages[s_SelectedLanguage].m_Name, r.front().m_Name), &IsActive);
		if(IsActive)
			m_ActiveListBox = ACTLB_LANG;

		if(Item.m_Visible)
		{
			CUIRect Rect;
			Item.m_Rect.VSplitLeft(Item.m_Rect.h*2.0f, &Rect, &Item.m_Rect);
			Rect.VMargin(6.0f, &Rect);
			Rect.HMargin(3.0f, &Rect);
			vec4 Color(1.0f, 1.0f, 1.0f, 1.0f);
			m_pClient->m_pCountryFlags->Render(r.front().m_CountryCode, &Color, Rect.x, Rect.y, Rect.w, Rect.h, true);
			if(Item.m_Selected)
			{
				TextRender()->TextColor(CUI::ms_HighlightTextColor);
				TextRender()->TextOutlineColor(CUI::ms_HighlightTextOutlineColor);
			}
			Item.m_Rect.y += 2.0f;
			UI()->DoLabel(&Item.m_Rect, r.front().m_Name, Item.m_Rect.h * ms_FontmodHeight * 0.8f, CUI::ALIGN_LEFT);
			if(Item.m_Selected)
			{
				TextRender()->TextColor(CUI::ms_DefaultTextColor);
				TextRender()->TextOutlineColor(CUI::ms_DefaultTextOutlineColor);
			}
		}
	}

	s_SelectedLanguage = s_ListBox.DoEnd();

	if(OldSelected != s_SelectedLanguage)
	{
		m_ActiveListBox = ACTLB_LANG;
		str_copy(g_Config.m_ClLanguagefile, s_Languages[s_SelectedLanguage].m_FileName, sizeof(g_Config.m_ClLanguagefile));
		g_Localization.Load(s_Languages[s_SelectedLanguage].m_FileName, Storage(), Console());
	}
}

void CMenus::RenderThemeSelection(CUIRect MainView, bool Header)
{
	static CListBox s_ListBox;

	if(m_lThemes.size() == 0) // not loaded yet
	{
		if(!g_Config.m_ClShowMenuMap)
			str_copy(g_Config.m_ClMenuMap, "", sizeof(g_Config.m_ClMenuMap)); // cl_menu_map otherwise resets to default on loading
		m_lThemes.add(CTheme("", false, false)); // no theme
		m_lThemes.add(CTheme("auto", false, false)); // auto theme
		Storage()->ListDirectory(IStorage::TYPE_ALL, "ui/themes", ThemeScan, (CMenus*)this);
		Storage()->ListDirectory(IStorage::TYPE_ALL, "ui/themes", ThemeIconScan, (CMenus*)this);
	}

	int SelectedTheme = -1;
	for(int i = 0; i < m_lThemes.size(); i++)
		if(str_comp(m_lThemes[i].m_Name, g_Config.m_ClMenuMap) == 0)
		{
			SelectedTheme = i;
			break;
		}
	const int OldSelected = SelectedTheme;

	if(Header)
		s_ListBox.DoHeader(&MainView, Localize("Theme"), GetListHeaderHeight());

	bool IsActive = m_ActiveListBox == ACTLB_THEME;
	s_ListBox.DoStart(20.0f, m_lThemes.size(), 1, 3, SelectedTheme, Header ? 0 : &MainView, Header, &IsActive);

	for(sorted_array<CTheme>::range r = m_lThemes.all(); !r.empty(); r.pop_front())
	{
		const CTheme& Theme = r.front();
		CListboxItem Item = s_ListBox.DoNextItem(&Theme, SelectedTheme == (&Theme - m_lThemes.base_ptr()), &IsActive);
		if(IsActive)
			m_ActiveListBox = ACTLB_THEME;

		if(!Item.m_Visible)
			continue;

		CUIRect Icon;
		Item.m_Rect.VSplitLeft(Item.m_Rect.h * 2.0f, &Icon, &Item.m_Rect);
		
		// draw icon if it exists
		if(Theme.m_IconTexture.IsValid())
		{
			Icon.VMargin(6.0f, &Icon);
			Icon.HMargin(3.0f, &Icon);
			Graphics()->TextureSet(Theme.m_IconTexture);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			IGraphics::CQuadItem QuadItem(Icon.x, Icon.y, Icon.w, Icon.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		char aName[128];
		if(!Theme.m_Name[0])
			str_copy(aName, "(none)", sizeof(aName));
		else if(str_comp(Theme.m_Name, "auto") == 0)
			str_copy(aName, "(automatic)", sizeof(aName));
		else if(Theme.m_HasDay && Theme.m_HasNight)
			str_format(aName, sizeof(aName), "%s", Theme.m_Name.cstr());
		else if(Theme.m_HasDay && !Theme.m_HasNight)
			str_format(aName, sizeof(aName), "%s (day)", Theme.m_Name.cstr());
		else if(!Theme.m_HasDay && Theme.m_HasNight)
			str_format(aName, sizeof(aName), "%s (night)", Theme.m_Name.cstr());
		else // generic
			str_format(aName, sizeof(aName), "%s", Theme.m_Name.cstr());

		if(Item.m_Selected)
		{
			TextRender()->TextColor(CUI::ms_HighlightTextColor);
			TextRender()->TextOutlineColor(CUI::ms_HighlightTextOutlineColor);
		}
		Item.m_Rect.y += 2.0f;
		UI()->DoLabel(&Item.m_Rect, aName, Item.m_Rect.h * ms_FontmodHeight * 0.8f, CUI::ALIGN_LEFT);
		if(Item.m_Selected)
		{
			TextRender()->TextColor(CUI::ms_DefaultTextColor);
			TextRender()->TextOutlineColor(CUI::ms_DefaultTextOutlineColor);
		}
	}

	SelectedTheme = s_ListBox.DoEnd();

	if(OldSelected != SelectedTheme)
	{
		m_ActiveListBox = ACTLB_THEME;
		str_copy(g_Config.m_ClMenuMap, m_lThemes[SelectedTheme].m_Name, sizeof(g_Config.m_ClMenuMap));
		g_Config.m_ClShowMenuMap = m_lThemes[SelectedTheme].m_Name[0] ? 1 : 0;
		m_pClient->m_pMapLayersBackGround->BackgroundMapUpdate();
	}
}

void CMenus::RenderSettingsGeneral(CUIRect MainView)
{
	CUIRect Label, Button, Game, Client, BottomView, Background;

	// cut view
	MainView.HSplitBottom(80.0f, &MainView, &BottomView);
	BottomView.HSplitTop(20.f, 0, &BottomView);

	// render game menu backgrounds
	int NumOptions = max(g_Config.m_ClNameplates ? 6 : 3, g_Config.m_ClShowsocial ? 6 : 5);
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;

	if(this->Client()->State() == IClient::STATE_ONLINE)
		Background = MainView;
	else
		MainView.HSplitTop(20.0f, 0, &Background);
	RenderTools()->DrawUIRect(&Background, vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), this->Client()->State() == IClient::STATE_OFFLINE ? CUI::CORNER_ALL : CUI::CORNER_B, 5.0f);
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &Game, &MainView);
	RenderTools()->DrawUIRect(&Game, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// render client menu background
	NumOptions = 4;
	if(g_Config.m_ClAutoDemoRecord) NumOptions += 1;
	if(g_Config.m_ClAutoScreenshot) NumOptions += 1;
	BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;

	MainView.HSplitTop(10.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &Client, &MainView);
	RenderTools()->DrawUIRect(&Client, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	CUIRect GameLeft, GameRight;
	// render game menu
	Game.HSplitTop(ButtonHeight, &Label, &Game);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Game"), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

	Game.VSplitMid(&GameLeft, &GameRight, Spacing);

	// left side
	GameLeft.HSplitTop(Spacing, 0, &GameLeft);
	GameLeft.HSplitTop(ButtonHeight, &Button, &GameLeft);
	static int s_DynamicCameraButton = 0;
	if(DoButton_CheckBox(&s_DynamicCameraButton, Localize("Dynamic Camera"), g_Config.m_ClDynamicCamera, &Button))
	{
		if(g_Config.m_ClDynamicCamera)
		{
			g_Config.m_ClDynamicCamera = 0;
			// force to defaults when using the GUI
			g_Config.m_ClMouseMaxDistanceStatic = 400;
			// g_Config.m_ClMouseFollowfactor = 0;
			// g_Config.m_ClMouseDeadzone = 0;
		}
		else
		{
			g_Config.m_ClDynamicCamera = 1;
			// force to defaults when using the GUI
			g_Config.m_ClMouseMaxDistanceDynamic = 1000;
			g_Config.m_ClMouseFollowfactor = 60;
			g_Config.m_ClMouseDeadzone = 300;
		}
	}

	GameLeft.HSplitTop(Spacing, 0, &GameLeft);
	GameLeft.HSplitTop(ButtonHeight, &Button, &GameLeft);
	static int s_AutoswitchWeapons = 0;
	if(DoButton_CheckBox(&s_AutoswitchWeapons, Localize("Switch weapon on pickup"), g_Config.m_ClAutoswitchWeapons, &Button))
		g_Config.m_ClAutoswitchWeapons ^= 1;

	GameLeft.HSplitTop(Spacing, 0, &GameLeft);
	GameLeft.HSplitTop(ButtonHeight, &Button, &GameLeft);
	static int s_Nameplates = 0;
	if(DoButton_CheckBox(&s_Nameplates, Localize("Show name plates"), g_Config.m_ClNameplates, &Button))
		g_Config.m_ClNameplates ^= 1;

	if(g_Config.m_ClNameplates)
	{
		GameLeft.HSplitTop(Spacing, 0, &GameLeft);
		GameLeft.HSplitTop(ButtonHeight, &Button, &GameLeft);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		static int s_NameplatesAlways = 0;
		if(DoButton_CheckBox(&s_NameplatesAlways, Localize("Always show name plates"), g_Config.m_ClNameplatesAlways, &Button))
			g_Config.m_ClNameplatesAlways ^= 1;

		GameLeft.HSplitTop(Spacing, 0, &GameLeft);
		GameLeft.HSplitTop(ButtonHeight, &Button, &GameLeft);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		DoScrollbarOption(&g_Config.m_ClNameplatesSize, &g_Config.m_ClNameplatesSize, &Button, Localize("Size"), 0, 100);

		GameLeft.HSplitTop(Spacing, 0, &GameLeft);
		GameLeft.HSplitTop(ButtonHeight, &Button, &GameLeft);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		static int s_NameplatesTeamcolors = 0;
		if(DoButton_CheckBox(&s_NameplatesTeamcolors, Localize("Use team colors for name plates"), g_Config.m_ClNameplatesTeamcolors, &Button))
			g_Config.m_ClNameplatesTeamcolors ^= 1;
	}

	// right side
	GameRight.HSplitTop(Spacing, 0, &GameRight);
	GameRight.HSplitTop(ButtonHeight, &Button, &GameRight);
	static int s_Showhud = 0;
	if(DoButton_CheckBox(&s_Showhud, Localize("Show ingame HUD"), g_Config.m_ClShowhud, &Button))
		g_Config.m_ClShowhud ^= 1;

	GameRight.HSplitTop(Spacing, 0, &GameRight);
	GameRight.HSplitTop(ButtonHeight, &Button, &GameRight);
	static int s_ShowUserId = 0;
	if(DoButton_CheckBox(&s_ShowUserId, Localize("Show user IDs"), g_Config.m_ClShowUserId, &Button))
		g_Config.m_ClShowUserId ^= 1;

	GameRight.HSplitTop(Spacing, 0, &GameRight);
	GameRight.HSplitTop(ButtonHeight, &Button, &GameRight);
	static int s_Showsocial = 0;
	if(DoButton_CheckBox(&s_Showsocial, Localize("Show social"), g_Config.m_ClShowsocial, &Button))
		g_Config.m_ClShowsocial ^= 1;

	// show chat messages button
	if(g_Config.m_ClShowsocial)
	{
		GameRight.HSplitTop(Spacing, 0, &GameRight);
		GameRight.HSplitTop(ButtonHeight, &Button, &GameRight);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		/*RenderTools()->DrawUIRect(&Button, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
		CUIRect Text;
		Button.VSplitLeft(ButtonHeight+5.0f, 0, &Button);
		Button.VSplitLeft(200.0f, &Text, &Button);

		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), Localize("Show chat messages from:"));
		Text.y += 2.0f;
		UI()->DoLabel(&Text, aBuf, Text.h*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

		Button.VSplitLeft(119.0f, &Button, 0);
		if(g_Config.m_ClFilterchat == 0)
			str_format(aBuf, sizeof(aBuf), Localize("everyone", "Show chat messages from:"));
		else if(g_Config.m_ClFilterchat == 1)
			str_format(aBuf, sizeof(aBuf), Localize("friends only", "Show chat messages from:"));
		else if(g_Config.m_ClFilterchat == 2)
			str_format(aBuf, sizeof(aBuf), Localize("no one", "Show chat messages from:"));
		static CButtonContainer s_ButtonFilterchat;
		if(DoButton_Menu(&s_ButtonFilterchat, aBuf, 0, &Button))
			g_Config.m_ClFilterchat = (g_Config.m_ClFilterchat + 1) % 3;*/

		const int NumLabels = 3;
		const char* aLabels[NumLabels] = { Localize("everyone", "Show chat messages from"), Localize("friends only", "Show chat messages from"), Localize("no one", "Show chat messages from") };
		DoScrollbarOptionLabeled(&g_Config.m_ClFilterchat, &g_Config.m_ClFilterchat, &Button, Localize("Show chat messages from"), aLabels, NumLabels);
	}

	GameRight.HSplitTop(Spacing, 0, &GameRight);
	GameRight.HSplitTop(ButtonHeight, &Button, &GameRight);
	static int s_EnableColoredBroadcasts = 0;
	if(DoButton_CheckBox(&s_EnableColoredBroadcasts, Localize("Enable colored server broadcasts"),
						 g_Config.m_ClColoredBroadcast, &Button))
		g_Config.m_ClColoredBroadcast ^= 1;

	// render client menu
	Client.HSplitTop(ButtonHeight, &Label, &Client);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Client"), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

	Client.HSplitTop(Spacing, 0, &Client);
	Client.HSplitTop(ButtonHeight, &Button, &Client);
	static int s_SkipMainMenu = 0;
	if(DoButton_CheckBox(&s_SkipMainMenu, Localize("Skip the main menu"), g_Config.m_ClSkipStartMenu, &Button))
		g_Config.m_ClSkipStartMenu ^= 1;

	Client.HSplitTop(Spacing, 0, &Client);
	Client.HSplitTop(ButtonHeight, &Button, &Client);
	DoScrollbarOption(&g_Config.m_ClMenuAlpha, &g_Config.m_ClMenuAlpha, &Button, Localize("Menu background opacity"), 0, 75);

	Client.HSplitTop(Spacing, 0, &Client);
	Client.HSplitTop(ButtonHeight, &Button, &Client);
	static int s_AutoDemoRecord = 0;
	if(DoButton_CheckBox(&s_AutoDemoRecord, Localize("Automatically record demos"), g_Config.m_ClAutoDemoRecord, &Button))
		g_Config.m_ClAutoDemoRecord ^= 1;

	if(g_Config.m_ClAutoDemoRecord)
	{
		Client.HSplitTop(Spacing, 0, &Client);
		Client.HSplitTop(ButtonHeight, &Button, &Client);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		DoScrollbarOption(&g_Config.m_ClAutoDemoMax, &g_Config.m_ClAutoDemoMax, &Button, Localize("Max"), 0, 1000, &LogarithmicScrollbarScale, true);
	}

	Client.HSplitTop(Spacing, 0, &Client);
	Client.HSplitTop(ButtonHeight, &Button, &Client);
	static int s_AutoScreenshot = 0;
	if(DoButton_CheckBox(&s_AutoScreenshot, Localize("Automatically take game over screenshot"), g_Config.m_ClAutoScreenshot, &Button))
		g_Config.m_ClAutoScreenshot ^= 1;

	if(g_Config.m_ClAutoScreenshot)
	{
		Client.HSplitTop(Spacing, 0, &Client);
		Client.HSplitTop(ButtonHeight, &Button, &Client);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		DoScrollbarOption(&g_Config.m_ClAutoScreenshotMax, &g_Config.m_ClAutoScreenshotMax, &Button, Localize("Max"), 0, 1000, &LogarithmicScrollbarScale, true);
	}

	MainView.HSplitTop(10.0f, 0, &MainView);

	// render language and theme selection
	CUIRect LanguageView, ThemeView;
	MainView.VSplitMid(&LanguageView, &ThemeView, 2.0f);
	RenderLanguageSelection(LanguageView);
	RenderThemeSelection(ThemeView);

	// reset button
	Spacing = 3.0f;
	float ButtonWidth = (BottomView.w/6.0f)-(Spacing*5.0)/6.0f;

	BottomView.VSplitRight(ButtonWidth, 0, &BottomView);
	RenderBackgroundShadow(&BottomView, true);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	Button = BottomView;
	static CButtonContainer s_ResetButton;
	if(DoButton_Menu(&s_ResetButton, Localize("Reset"), 0, &Button))
	{
		PopupConfirm(Localize("Reset general settings"), Localize("Are you sure that you want to reset the general settings to their defaults?"),
			Localize("Reset"), Localize("Cancel"), &CMenus::ResetSettingsGeneral);
	}
}

void CMenus::RenderSettingsTeeBasic(CUIRect MainView)
{
	RenderSkinSelection(MainView); // yes thats all here ^^
}

void CMenus::RenderSettingsTeeCustom(CUIRect MainView)
{
	CUIRect Label, Patterns, Button, Left, Right, Picker, Palette;

	// render skin preview background
	float SpacingH = 2.0f;
	float SpacingW = 3.0f;
	float ButtonHeight = 20.0f;

	RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	MainView.HSplitTop(ButtonHeight, &Label, &MainView);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Customize"), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

	// skin part selection
	MainView.HSplitTop(SpacingH, 0, &MainView);
	MainView.HSplitTop(ButtonHeight, &Patterns, &MainView);
	RenderTools()->DrawUIRect(&Patterns, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	float ButtonWidth = (Patterns.w/6.0f)-(SpacingW*5.0)/6.0f;

	static CButtonContainer s_aPatternButtons[6];
	for(int i = 0; i < NUM_SKINPARTS; i++)
	{
		Patterns.VSplitLeft(ButtonWidth, &Button, &Patterns);
		if(DoButton_MenuTabTop(&s_aPatternButtons[i], Localize(CSkins::ms_apSkinPartNames[i]), m_TeePartSelected==i, &Button))
		{
			m_TeePartSelected = i;
		}
		Patterns.VSplitLeft(SpacingW, 0, &Patterns);
	}

	MainView.HSplitTop(SpacingH, 0, &MainView);
	MainView.VSplitMid(&Left, &Right, SpacingW);

	// part selection
	RenderSkinPartSelection(Left);

	// use custom color checkbox
	Right.HSplitTop(ButtonHeight, &Button, &Right);
	static bool s_CustomColors;
	s_CustomColors = *CSkins::ms_apUCCVariables[m_TeePartSelected] == 1;
	if(DoButton_CheckBox(&s_CustomColors, Localize("Custom colors"), s_CustomColors, &Button))
	{
		*CSkins::ms_apUCCVariables[m_TeePartSelected] = s_CustomColors ? 0 : 1;
		m_SkinModified = true;
	}

	// HSL picker
	Right.HSplitBottom(45.0f, &Picker, &Palette);
	if(s_CustomColors)
	{
		RenderTools()->DrawUIRect(&Right, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
		RenderHSLPicker(Picker);
		RenderSkinPartPalette(Palette);
	}
}

void CMenus::RenderSettingsPlayer(CUIRect MainView)
{
	static bool s_CustomSkinMenu = false;
	static int s_PlayerCountry = 0;
	static char s_aPlayerName[256] = { 0 };
	static char s_aPlayerClan[256] = { 0 };
	static char s_aPlayerSkin[256] = { 0 };
	if(m_pClient->m_IdentityState < 0)
	{
		s_PlayerCountry = g_Config.m_PlayerCountry;
		m_pClient->m_IdentityState = 0;
	}

	CUIRect Button, Label, TopView, BottomView, Background, Left, Right;

	// cut view
	MainView.HSplitBottom(80.0f, &MainView, &BottomView);
	BottomView.HSplitTop(20.f, 0, &BottomView);

	// render skin preview background
	const float SpacingH = 2.0f;
	const float SpacingW = 3.0f;
	const float ButtonHeight = 20.0f;
	const float SkinHeight = 50.0f;
	const float BackgroundHeight = (ButtonHeight + SpacingH) + SkinHeight * 2;
	const vec2 MousePosition = vec2(UI()->MouseX(), UI()->MouseY());

	if(this->Client()->State() == IClient::STATE_ONLINE)
		Background = MainView;
	else
		MainView.HSplitTop(20.0f, 0, &Background);
	RenderTools()->DrawUIRect(&Background, vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), Client()->State() == IClient::STATE_OFFLINE ? CUI::CORNER_ALL : CUI::CORNER_B, 5.0f);
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &TopView, &MainView);
	TopView.VSplitMid(&Left, &Right, 3.0f);
	RenderTools()->DrawUIRect(&Left, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
	RenderTools()->DrawUIRect(&Right, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	Left.HSplitTop(ButtonHeight, &Label, &Left);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Tee"), ButtonHeight * ms_FontmodHeight * 0.8f, CUI::ALIGN_CENTER);

	// Preview
	{
		CUIRect Top, Bottom, TeeLeft, TeeRight;
		Left.HSplitTop(SpacingH, 0, &Left);
		Left.HSplitTop(SkinHeight * 2, &Top, &Left);

		// split the menu in 2 parts
		Top.HSplitMid(&Top, &Bottom, SpacingH);

		// handle left

		// validate skin parts for solo mode
		CTeeRenderInfo OwnSkinInfo;
		OwnSkinInfo.m_Size = 50.0f;

		char aSkinParts[NUM_SKINPARTS][MAX_SKIN_LENGTH];
		char* apSkinPartsPtr[NUM_SKINPARTS];
		int aUCCVars[NUM_SKINPARTS];
		int aColorVars[NUM_SKINPARTS];

		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			str_copy(aSkinParts[p], CSkins::ms_apSkinVariables[p], MAX_SKIN_LENGTH);
			apSkinPartsPtr[p] = aSkinParts[p];
			aUCCVars[p] = *CSkins::ms_apUCCVariables[p];
			aColorVars[p] = *CSkins::ms_apColorVariables[p];
		}

		m_pClient->m_pSkins->ValidateSkinParts(apSkinPartsPtr, aUCCVars, aColorVars, 0);

		for (int p = 0; p < NUM_SKINPARTS; p++)
		{
			int SkinPart = m_pClient->m_pSkins->FindSkinPart(p, apSkinPartsPtr[p], false);
			const CSkins::CSkinPart *pSkinPart = m_pClient->m_pSkins->GetSkinPart(p, SkinPart);
			if (aUCCVars[p])
			{
				OwnSkinInfo.m_aTextures[p] = pSkinPart->m_ColorTexture;
				OwnSkinInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(aColorVars[p], p == SKINPART_MARKING);
			}
			else
			{
				OwnSkinInfo.m_aTextures[p] = pSkinPart->m_OrgTexture;
				OwnSkinInfo.m_aColors[p] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}

		// draw preview
		RenderTools()->DrawUIRect(&Top, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		Top.VSplitLeft(Top.w / 3.0f + SpacingW / 2.0f, &Label, &Top);
		Label.y += 17.0f;
		UI()->DoLabel(&Label, Localize("Normal:"), ButtonHeight * ms_FontmodHeight * 0.8f, CUI::ALIGN_CENTER);

		RenderTools()->DrawUIRect(&Top, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		{
			// interactive tee: tee looking towards cursor, and it is happy when you touch it
			vec2 TeePosition = vec2(Top.x + Top.w / 2.0f, Top.y + Top.h / 2.0f + 6.0f);
			vec2 DeltaPosition = MousePosition - TeePosition;
			float Distance = length(DeltaPosition);
			vec2 TeeDirection = Distance < 20.0f ? normalize(vec2(DeltaPosition.x, max(DeltaPosition.y, 0.5f))) : normalize(DeltaPosition);
			int TeeEmote = Distance < 20.0f ? EMOTE_HAPPY : EMOTE_NORMAL;
			RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, TeeEmote, TeeDirection, TeePosition);
			if(Distance < 20.0f && UI()->MouseButtonClicked(0))
				m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_PLAYER_SPAWN, 0);
		}

		// handle right (team skins)
		// validate skin parts for team game mode
		CTeeRenderInfo TeamSkinInfo = OwnSkinInfo;

		for (int p = 0; p < NUM_SKINPARTS; p++)
		{
			str_copy(aSkinParts[p], CSkins::ms_apSkinVariables[p], MAX_SKIN_LENGTH);
			apSkinPartsPtr[p] = aSkinParts[p];
			aUCCVars[p] = *CSkins::ms_apUCCVariables[p];
			aColorVars[p] = *CSkins::ms_apColorVariables[p];
		}

		m_pClient->m_pSkins->ValidateSkinParts(apSkinPartsPtr, aUCCVars, aColorVars, GAMEFLAG_TEAMS);

		for (int p = 0; p < NUM_SKINPARTS; p++)
		{
			int SkinPart = m_pClient->m_pSkins->FindSkinPart(p, apSkinPartsPtr[p], false);
			const CSkins::CSkinPart* pSkinPart = m_pClient->m_pSkins->GetSkinPart(p, SkinPart);
			if (aUCCVars[p])
			{
				TeamSkinInfo.m_aTextures[p] = pSkinPart->m_ColorTexture;
				TeamSkinInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(aColorVars[p], p == SKINPART_MARKING);
			}
			else
			{
				TeamSkinInfo.m_aTextures[p] = pSkinPart->m_OrgTexture;
				TeamSkinInfo.m_aColors[p] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}

		// draw preview
		RenderTools()->DrawUIRect(&Bottom, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		Bottom.VSplitLeft(Bottom.w / 3.0f + SpacingW / 2.0f, &Label, &Bottom);
		Label.y += 17.0f;
		UI()->DoLabel(&Label, Localize("Team:"), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

		Bottom.VSplitMid(&TeeLeft, &TeeRight, SpacingW);

		RenderTools()->DrawUIRect(&TeeLeft, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			int TeamColor = m_pClient->m_pSkins->GetTeamColor(aUCCVars[p], aColorVars[p], TEAM_RED, p);
			TeamSkinInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(TeamColor, p == SKINPART_MARKING);
		}
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeamSkinInfo, 0, vec2(1, 0), vec2(TeeLeft.x + TeeLeft.w / 2.0f, TeeLeft.y + TeeLeft.h / 2.0f + 6.0f));

		RenderTools()->DrawUIRect(&TeeRight, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			int TeamColor = m_pClient->m_pSkins->GetTeamColor(aUCCVars[p], aColorVars[p], TEAM_BLUE, p);
			TeamSkinInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(TeamColor, p == SKINPART_MARKING);
		}
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeamSkinInfo, 0, vec2(-1, 0), vec2(TeeRight.x + TeeRight.w / 2.0f, TeeRight.y + TeeRight.h / 2.0f + 6.0f));
	}

	Right.HSplitTop(ButtonHeight, &Label, &Right);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Personal"), ButtonHeight* ms_FontmodHeight * 0.8f, CUI::ALIGN_CENTER);

	// Personal
	{
		CUIRect Top, Bottom, Name, Clan, Flag;
		Right.HSplitTop(SpacingH, 0, &Right);
		Right.HSplitMid(&Top, &Bottom, SpacingH);
		Top.HSplitMid(&Name, &Clan, SpacingH);

		// player name
		Name.HSplitTop(ButtonHeight, &Button, &Name);
		static float s_OffsetName = 0.0f;
		DoEditBoxOption(g_Config.m_PlayerName, g_Config.m_PlayerName, sizeof(g_Config.m_PlayerName), &Button, Localize("Name"), 100.0f, &s_OffsetName);

		// player clan
		Clan.HSplitTop(ButtonHeight, &Button, &Clan);
		static float s_OffsetClan = 0.0f;
		DoEditBoxOption(g_Config.m_PlayerClan, g_Config.m_PlayerClan, sizeof(g_Config.m_PlayerClan), &Button, Localize("Clan"), 100.0f, &s_OffsetClan);

		// country selector
		RenderTools()->DrawUIRect(&Bottom, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		Bottom.VSplitLeft(100.0f, &Label, &Button);
		Label.y += 17.0f;
		UI()->DoLabel(&Label, Localize("Country:"), ButtonHeight* ms_FontmodHeight * 0.8f, CUI::ALIGN_CENTER);

		Button.w = (SkinHeight - 20.0f) * 2 + 20.0f;
		RenderTools()->DrawUIRect(&Button, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
		if(UI()->MouseHovered(&Button))
			RenderTools()->DrawUIRect(&Button, vec4(0.5f, 0.5f, 0.5f, 0.25f), CUI::CORNER_ALL, 5.0f);

		Button.Margin(10.0f, &Flag);
		vec4 Color = vec4(1, 1, 1, 1);
		m_pClient->m_pCountryFlags->Render(g_Config.m_PlayerCountry, &Color, Flag.x, Flag.y, Flag.w, Flag.h);

		if(UI()->DoButtonLogic(&g_Config.m_PlayerCountry, &Button))
			PopupCountry(g_Config.m_PlayerCountry, &CMenus::PopupConfirmPlayerCountry);
	}

	MainView.HSplitTop(10.0f, 0, &MainView);

	if(s_CustomSkinMenu)
		RenderSettingsTeeCustom(MainView);
	else
		RenderSettingsTeeBasic(MainView);

	// bottom button
	float ButtonWidth = (BottomView.w/6.0f)-(SpacingW*5.0)/6.0f;
	float BackgroundWidth = s_CustomSkinMenu || (m_pSelectedSkin && (m_pSelectedSkin->m_Flags & CSkins::SKINFLAG_STANDARD) == 0) ? ButtonWidth * 3.0f + SpacingW : ButtonWidth;

	BottomView.VSplitRight(BackgroundWidth, 0, &BottomView);
	RenderBackgroundShadow(&BottomView, true);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	if(s_CustomSkinMenu)
	{
		BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
		static CButtonContainer s_RandomSkinButton;
		if(DoButton_Menu(&s_RandomSkinButton, "Random", 0, &Button))
		{
			for(int p = 0; p < NUM_SKINPARTS; p++)
			{
				int Hue = random_int() % 255;
				int Sat = random_int() % 255;
				int Lgt = random_int() % 255;
				int Alp = 0;
				if(p == 1) // 1 == SKINPART_MARKING
					Alp = random_int() % 255;
				int NewVal = (Alp << 24) + (Hue << 16) + (Sat << 8) + Lgt;
				*CSkins::ms_apColorVariables[p] = NewVal;
			}

			static sorted_array<const CSkins::CSkinPart*> s_paList[6];
			static CListBox s_ListBox;
			for(int p = 0; p < NUM_SKINPARTS; p++)
			{
				s_paList[p].clear();
				for(int i = 0; i < m_pClient->m_pSkins->NumSkinPart(p); ++i)
				{
					const CSkins::CSkinPart* s = m_pClient->m_pSkins->GetSkinPart(p, i);
					// no special skins
					if((s->m_Flags & CSkins::SKINFLAG_SPECIAL) == 0 && s_ListBox.FilterMatches(s->m_aName))
					{
						s_paList[p].add(s);
					}
				}
			}

			for(int p = 0; p < NUM_SKINPARTS; p++)
			{
				int rand_choice = random_int() % (m_pClient->m_pSkins->NumSkinPart(p));
				if((rand_choice <= m_pClient->m_pSkins->NumSkinPart(p)) && (rand_choice != 0))
					rand_choice -= 1;
				const CSkins::CSkinPart* s = s_paList[p][rand_choice];
				mem_copy(CSkins::ms_apSkinVariables[p], s->m_aName, 24);
				g_Config.m_PlayerSkin[0] = 0;
				m_SkinModified = true;
			}
		}

		BottomView.VSplitLeft(SpacingW, 0, &BottomView);

		BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
		static CButtonContainer s_CustomSkinSaveButton;
		if(DoButton_Menu(&s_CustomSkinSaveButton, Localize("Save"), 0, &Button))
			m_Popup = POPUP_SAVE_SKIN;
		BottomView.VSplitLeft(SpacingW, 0, &BottomView);
	}
	else if(m_pSelectedSkin && (m_pSelectedSkin->m_Flags&CSkins::SKINFLAG_STANDARD) == 0)
	{
		BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
		static CButtonContainer s_CustomSkinDeleteButton;
		if(DoButton_Menu(&s_CustomSkinDeleteButton, Localize("Delete"), 0, &Button))
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), Localize("Are you sure that you want to delete the skin '%s'?"), m_pSelectedSkin->m_aName);
			PopupConfirm(Localize("Delete skin"), aBuf, Localize("Yes"), Localize("No"), &CMenus::PopupConfirmDeleteSkin);
		}
		BottomView.VSplitLeft(SpacingW, 0, &BottomView);
	}

	BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
	static CButtonContainer s_CustomSwitchButton;
	if(DoButton_Menu(&s_CustomSwitchButton, s_CustomSkinMenu ? Localize("Basic") : Localize("Custom"), 0, &Button))
	{
		s_CustomSkinMenu = !s_CustomSkinMenu;
	}

	// check if the new settings require a server reload
	m_NeedRestartPlayer = !(
		s_PlayerCountry == g_Config.m_PlayerCountry &&
		!str_comp(s_aPlayerClan, g_Config.m_PlayerClan) &&
		!str_comp(s_aPlayerName, g_Config.m_PlayerName)
		);
	m_pClient->m_IdentityState = m_NeedRestartPlayer ? 1 : 0;
}

void CMenus::PopupConfirmDeleteSkin()
{
	if(m_pSelectedSkin)
	{
		char aBuf[IO_MAX_PATH_LENGTH];
		str_format(aBuf, sizeof(aBuf), "skins/%s.json", m_pSelectedSkin->m_aName);
		if(Storage()->RemoveFile(aBuf, IStorage::TYPE_SAVE))
		{
			m_pClient->m_pSkins->RemoveSkin(m_pSelectedSkin);
			m_RefreshSkinSelector = true;
			m_pSelectedSkin = 0;
		}
		else
			PopupMessage(Localize("Error"), Localize("Unable to delete the skin"), Localize("Ok"));
	}
}

//typedef void (*pfnAssignFuncCallback)(CConfiguration *pConfig, int Value);

void CMenus::RenderSettingsControls(CUIRect MainView)
{
	// cut view
	CUIRect BottomView, Button, Background;
	MainView.HSplitBottom(80.0f, &MainView, &BottomView);
	if(this->Client()->State() == IClient::STATE_ONLINE)
		Background = MainView;
	else
		MainView.HSplitTop(20.0f, 0, &Background);
	RenderTools()->DrawUIRect(&Background, vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), Client()->State() == IClient::STATE_OFFLINE ? CUI::CORNER_ALL : CUI::CORNER_B, 5.0f);
	MainView.HSplitTop(20.0f, 0, &MainView);
	BottomView.HSplitTop(20.f, 0, &BottomView);

	const float HeaderHeight = 20.0f;

	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0, 0);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ClipBgColor = vec4(0, 0, 0, 0);
	ScrollParams.m_ScrollUnit = 60.0f; // inconsistent margin, 3 category header per scroll
	s_ScrollRegion.Begin(&MainView, &ScrollOffset, &ScrollParams);
	MainView.y += ScrollOffset.y;

	CUIRect LastExpandRect;
	static int s_MouseDropdown = 0;
	static bool s_MouseActive = true;
	float Split = DoIndependentDropdownMenu(&s_MouseDropdown, &MainView, Localize("Mouse"), HeaderHeight, &CMenus::RenderSettingsControlsMouse, &s_MouseActive);

	MainView.HSplitTop(Split + 10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);
	static int s_JoystickDropdown = 0;
	static bool s_JoystickActive = m_pClient->Input()->NumJoysticks() > 0; // hide by default if no joystick found
	Split = DoIndependentDropdownMenu(&s_JoystickDropdown, &MainView, Localize("Joystick"), HeaderHeight, &CMenus::RenderSettingsControlsJoystick, &s_JoystickActive);

	MainView.HSplitTop(Split + 10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);

	static int s_MovementDropdown = 0;
	static bool s_MovementActive = true;
	Split = DoIndependentDropdownMenu(&s_MovementDropdown, &MainView, Localize("Movement"), HeaderHeight, &CMenus::RenderSettingsControlsMovement, &s_MovementActive);

	MainView.HSplitTop(Split + 10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);
	static int s_WeaponDropdown = 0;
	static bool s_WeaponActive = true;
	Split = DoIndependentDropdownMenu(&s_WeaponDropdown, &MainView, Localize("Weapon"), HeaderHeight, &CMenus::RenderSettingsControlsWeapon, &s_WeaponActive);

	MainView.HSplitTop(Split + 10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);
	static int s_VotingDropdown = 0;
	static bool s_VotingActive = true;
	Split = DoIndependentDropdownMenu(&s_VotingDropdown, &MainView, Localize("Voting"), HeaderHeight, &CMenus::RenderSettingsControlsVoting, &s_VotingActive);

	MainView.HSplitTop(Split + 10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);
	static int s_ChatDropdown = 0;
	static bool s_ChatActive = true;
	Split = DoIndependentDropdownMenu(&s_ChatDropdown, &MainView, Localize("Chat"), HeaderHeight, &CMenus::RenderSettingsControlsChat, &s_ChatActive);

	MainView.HSplitTop(Split + 10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);
	static int s_ScoreboardDropdown = 0;
	static bool s_ScoreboardActive = true;
	Split = DoIndependentDropdownMenu(&s_ScoreboardDropdown, &MainView, Localize("Scoreboard"), HeaderHeight, &CMenus::RenderSettingsControlsScoreboard, &s_ScoreboardActive);

	MainView.HSplitTop(Split + 10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);
	static int s_MiscDropdown = 0;
	static bool s_MiscActive = true;
	Split = DoIndependentDropdownMenu(&s_MiscDropdown, &MainView, Localize("Misc"), HeaderHeight, &CMenus::RenderSettingsControlsMisc, &s_MiscActive);

	MainView.HSplitTop(Split + 10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);

	s_ScrollRegion.End();

	// reset button
	float Spacing = 3.0f;
	float ButtonWidth = (BottomView.w/6.0f)-(Spacing*5.0)/6.0f;

	BottomView.VSplitRight(ButtonWidth, 0, &BottomView);
	RenderBackgroundShadow(&BottomView, true);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	Button = BottomView;
	static CButtonContainer s_ResetButton;
	if(DoButton_Menu(&s_ResetButton, Localize("Reset"), 0, &Button))
	{
		PopupConfirm(Localize("Reset controls"), Localize("Are you sure that you want to reset the controls to their defaults?"),
			Localize("Reset"), Localize("Cancel"), &CMenus::ResetSettingsControls);
	}
}

float CMenus::RenderSettingsControlsStats(CUIRect View)
{
	const float RowHeight = 20.0f;
	CUIRect Button;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_ClStatboardInfos, Localize("Frags"), g_Config.m_ClStatboardInfos & TC_STATS_FRAGS, &Button))
		g_Config.m_ClStatboardInfos ^= TC_STATS_FRAGS;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_ClStatboardInfos + 1, Localize("Deaths"), g_Config.m_ClStatboardInfos & TC_STATS_DEATHS, &Button))
		g_Config.m_ClStatboardInfos ^= TC_STATS_DEATHS;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_ClStatboardInfos + 2, Localize("Suicides"), g_Config.m_ClStatboardInfos & TC_STATS_SUICIDES, &Button))
		g_Config.m_ClStatboardInfos ^= TC_STATS_SUICIDES;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_ClStatboardInfos + 3, Localize("Ratio"), g_Config.m_ClStatboardInfos & TC_STATS_RATIO, &Button))
		g_Config.m_ClStatboardInfos ^= TC_STATS_RATIO;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_ClStatboardInfos + 4, Localize("Net score"), g_Config.m_ClStatboardInfos & TC_STATS_NET, &Button))
		g_Config.m_ClStatboardInfos ^= TC_STATS_NET;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_ClStatboardInfos + 5, Localize("Frags per minute"), g_Config.m_ClStatboardInfos & TC_STATS_FPM, &Button))
		g_Config.m_ClStatboardInfos ^= TC_STATS_FPM;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_ClStatboardInfos + 6, Localize("Current spree"), g_Config.m_ClStatboardInfos & TC_STATS_SPREE, &Button))
		g_Config.m_ClStatboardInfos ^= TC_STATS_SPREE;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_ClStatboardInfos + 7, Localize("Best spree"), g_Config.m_ClStatboardInfos & TC_STATS_BESTSPREE, &Button))
		g_Config.m_ClStatboardInfos ^= TC_STATS_BESTSPREE;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_ClStatboardInfos + 9, Localize("Weapons stats"), g_Config.m_ClStatboardInfos & TC_STATS_WEAPS, &Button))
		g_Config.m_ClStatboardInfos ^= TC_STATS_WEAPS;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_ClStatboardInfos + 8, Localize("Flag grabs"), g_Config.m_ClStatboardInfos & TC_STATS_FLAGGRABS, &Button))
		g_Config.m_ClStatboardInfos ^= TC_STATS_FLAGGRABS;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_ClStatboardInfos + 10, Localize("Flag captures"), g_Config.m_ClStatboardInfos & TC_STATS_FLAGCAPTURES, &Button))
		g_Config.m_ClStatboardInfos ^= TC_STATS_FLAGCAPTURES;

	return 11 * RowHeight;
}

bool CMenus::DoResolutionList(CUIRect* pRect, CListBox* pListBox,
	const sorted_array<CVideoMode>& lModes)
{
	int OldSelected = -1;
	char aBuf[32];

	pListBox->DoStart(20.0f, lModes.size(), 1, 3, OldSelected, pRect);

	for(int i = 0; i < lModes.size(); ++i)
	{
		if(g_Config.m_GfxScreenWidth == lModes[i].m_Width &&
			g_Config.m_GfxScreenHeight == lModes[i].m_Height)
		{
			OldSelected = i;
		}

		CListboxItem Item = pListBox->DoNextItem(&lModes[i], OldSelected == i);
		if(Item.m_Visible)
		{
			int G = gcd(lModes[i].m_Width, lModes[i].m_Height);

			str_format(aBuf, sizeof(aBuf), "%dx%d (%d:%d)",
					   lModes[i].m_Width,
					   lModes[i].m_Height,
					   lModes[i].m_Width/G,
					   lModes[i].m_Height/G);

			if(Item.m_Selected)
			{
				TextRender()->TextColor(CUI::ms_HighlightTextColor);
				TextRender()->TextOutlineColor(CUI::ms_HighlightTextOutlineColor);
			}
			Item.m_Rect.y += 2.0f;
			UI()->DoLabel(&Item.m_Rect, aBuf, Item.m_Rect.h * ms_FontmodHeight * 0.8f, CUI::ALIGN_CENTER);
			if(Item.m_Selected)
			{
				TextRender()->TextColor(CUI::ms_DefaultTextColor);
				TextRender()->TextOutlineColor(CUI::ms_DefaultTextOutlineColor);
			}
		}
	}

	const int NewSelected = pListBox->DoEnd();
	if(OldSelected != NewSelected)
	{
		g_Config.m_GfxScreenWidth = lModes[NewSelected].m_Width;
		g_Config.m_GfxScreenHeight = lModes[NewSelected].m_Height;
		return true;
	}
	return false;
}

void CMenus::RenderSettingsGraphics(CUIRect MainView)
{
	bool CheckFullscreen = false;
#ifdef CONF_PLATFORM_MACOSX
	CheckFullscreen = true;
#endif
	
	static const int s_GfxFullscreen = g_Config.m_GfxFullscreen;
	static const int s_GfxScreenWidth = g_Config.m_GfxScreenWidth;
	static const int s_GfxScreenHeight = g_Config.m_GfxScreenHeight;
	static const int s_GfxFsaaSamples = g_Config.m_GfxFsaaSamples;
	static const int s_GfxTextureQuality = g_Config.m_GfxTextureQuality;
	static const int s_GfxTextureCompression = g_Config.m_GfxTextureCompression;

	CUIRect Label, Button, ScreenLeft, ScreenRight, Texture, BottomView, Background;

	// cut view
	MainView.HSplitBottom(80.0f, &MainView, &BottomView);
	BottomView.HSplitTop(20.f, 0, &BottomView);

	// render screen menu background
	int NumOptions = 3;
	if(Graphics()->GetNumScreens() > 1 && !g_Config.m_GfxFullscreen)
		++NumOptions;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;

	if(this->Client()->State() == IClient::STATE_ONLINE)
		Background = MainView;
	else
		MainView.HSplitTop(20.0f, 0, &Background);
	RenderTools()->DrawUIRect(&Background, vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), Client()->State() == IClient::STATE_OFFLINE ? CUI::CORNER_ALL : CUI::CORNER_B, 5.0f);
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &ScreenLeft, &MainView);
	RenderTools()->DrawUIRect(&ScreenLeft, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// render textures menu background
	NumOptions = 3;
	BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;

	MainView.HSplitTop(10.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &Texture, &MainView);
	RenderTools()->DrawUIRect(&Texture, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// render screen menu
	ScreenLeft.HSplitTop(ButtonHeight, &Label, &ScreenLeft);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Screen"), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

	ScreenLeft.VSplitMid(&ScreenLeft, &ScreenRight, Spacing);

	ScreenLeft.HSplitTop(Spacing, 0, &ScreenLeft);
	ScreenLeft.HSplitTop(ButtonHeight, &Button, &ScreenLeft);
	static int s_ButtonGfxFullscreen = 0;
	if(DoButton_CheckBox(&s_ButtonGfxFullscreen, Localize("Fullscreen"), g_Config.m_GfxFullscreen, &Button))
		Client()->ToggleFullscreen();

	if(!g_Config.m_GfxFullscreen)
	{
		ScreenLeft.HSplitTop(Spacing, 0, &ScreenLeft);
		ScreenLeft.HSplitTop(ButtonHeight, &Button, &ScreenLeft);
		static int s_ButtonGfxBorderless = 0;
		if(DoButton_CheckBox(&s_ButtonGfxBorderless, Localize("Borderless window"), g_Config.m_GfxBorderless, &Button))
			Client()->ToggleWindowBordered();
	}

	if(Graphics()->GetNumScreens() > 1)
	{
		char aBuf[64];
		ScreenLeft.HSplitTop(Spacing, 0, &ScreenLeft);
		ScreenLeft.HSplitTop(ButtonHeight, &Button, &ScreenLeft);
		RenderTools()->DrawUIRect(&Button, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
		CUIRect Text;
		Button.VSplitLeft(ButtonHeight+5.0f, 0, &Button);
		Button.VSplitLeft(100.0f-25.0f, &Text, &Button); // make button appear centered with FSAA
		str_format(aBuf, sizeof(aBuf), Localize("Screen:"));
		Text.y += 2.0f;
		UI()->DoLabel(&Text, aBuf, Text.h*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

		Button.VSplitLeft(120.0f, &Button, 0);
		str_format(aBuf, sizeof(aBuf), "#%d  (%dx%d)", g_Config.m_GfxScreen+1, Graphics()->DesktopWidth(), Graphics()->DesktopHeight());
		static CButtonContainer s_ButtonScreenId;
		if(DoButton_Menu(&s_ButtonScreenId, aBuf, 0, &Button))
		{
			g_Config.m_GfxScreen = (g_Config.m_GfxScreen + 1) % Graphics()->GetNumScreens();
			Client()->SwitchWindowScreen(g_Config.m_GfxScreen);
		}
	}

	// FSAA button
	{
		ScreenLeft.HSplitTop(Spacing, 0, &ScreenLeft);
		ScreenLeft.HSplitTop(ButtonHeight, &Button, &ScreenLeft);
		RenderTools()->DrawUIRect(&Button, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
		CUIRect Text;
		Button.VSplitLeft(ButtonHeight+5.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Text, &Button);

		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%s:", Localize("Anti Aliasing"));
		Text.y += 2.0f;
		UI()->DoLabel(&Text, aBuf, Text.h*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

		Button.VSplitLeft(70.0f, &Button, 0);
		str_format(aBuf, sizeof(aBuf), "%dx", g_Config.m_GfxFsaaSamples);
		static CButtonContainer s_ButtonGfxFsaaSamples;
		if(DoButton_Menu(&s_ButtonGfxFsaaSamples, aBuf, 0, &Button))
		{
			if(!g_Config.m_GfxFsaaSamples)
				g_Config.m_GfxFsaaSamples = 2;
			else if(g_Config.m_GfxFsaaSamples == 16)
				g_Config.m_GfxFsaaSamples = 0;
			else
				g_Config.m_GfxFsaaSamples *= 2;
			m_CheckVideoSettings = true;
		}
	}

	ScreenRight.HSplitTop(Spacing, 0, &ScreenRight);
	ScreenRight.HSplitTop(ButtonHeight, &Button, &ScreenRight);
	static int s_ButtonGfxVsync = 0;
	if(DoButton_CheckBox(&s_ButtonGfxVsync, Localize("V-Sync"), g_Config.m_GfxVsync, &Button))
		Client()->ToggleWindowVSync();

	ScreenRight.HSplitTop(Spacing, 0, &ScreenRight);
	ScreenRight.HSplitTop(ButtonHeight, &Button, &ScreenRight);

	// TODO: greyed out checkbox (not clickable)
	if(!g_Config.m_GfxVsync)
	{
		static int s_ButtonGfxCapFps = 0;
		if(DoButton_CheckBox(&s_ButtonGfxCapFps, Localize("Limit Fps"), g_Config.m_GfxLimitFps, &Button))
		{
			g_Config.m_GfxLimitFps ^= 1;
		}

		if(g_Config.m_GfxLimitFps > 0)
		{
			ScreenRight.HSplitTop(Spacing, 0, &ScreenRight);
			ScreenRight.HSplitTop(ButtonHeight, &Button, &ScreenRight);
			DoScrollbarOption(&g_Config.m_GfxMaxFps, &g_Config.m_GfxMaxFps,
							  &Button, Localize("Max fps"), 30, 300);
		}
	}

	// render texture menu
	Texture.HSplitTop(ButtonHeight, &Label, &Texture);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Texture"), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

	Texture.HSplitTop(Spacing, 0, &Texture);
	Texture.HSplitTop(ButtonHeight, &Button, &Texture);
	static int s_ButtonGfxTextureQuality = 0;
	if(DoButton_CheckBox(&s_ButtonGfxTextureQuality, Localize("Quality Textures"), g_Config.m_GfxTextureQuality, &Button))
	{
		g_Config.m_GfxTextureQuality ^= 1;
		m_CheckVideoSettings = true;
	}

	Texture.HSplitTop(Spacing, 0, &Texture);
	Texture.HSplitTop(ButtonHeight, &Button, &Texture);
	static int s_ButtonGfxTextureCompression = 0;
	if(DoButton_CheckBox(&s_ButtonGfxTextureCompression, Localize("Texture Compression"), g_Config.m_GfxTextureCompression, &Button))
	{
		g_Config.m_GfxTextureCompression ^= 1;
		m_CheckVideoSettings = true;
	}

	Texture.HSplitTop(Spacing, 0, &Texture);
	Texture.HSplitTop(ButtonHeight, &Button, &Texture);
	static int s_ButtonGfxHighDetail = 0;
	if(DoButton_CheckBox(&s_ButtonGfxHighDetail, Localize("High Detail"), g_Config.m_GfxHighDetail, &Button))
		g_Config.m_GfxHighDetail ^= 1;

	// render screen modes
	MainView.HSplitTop(10.0f, 0, &MainView);

	// display mode list
	{
		// custom list header
		NumOptions = 1;

		CUIRect Header, Button;
		MainView.HSplitTop(ButtonHeight*(float)(NumOptions+1)+Spacing*(float)(NumOptions+1), &Header, 0);
		RenderTools()->DrawUIRect(&Header, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_T, 5.0f);

		// draw header
		MainView.HSplitTop(ButtonHeight, &Header, &MainView);
		Header.y += 2.0f;
		UI()->DoLabel(&Header, Localize("Resolution"), Header.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

		MainView.HSplitTop(Spacing, 0, &MainView);
		MainView.HSplitTop(ButtonHeight, &Button, &MainView);
		CUIRect HeaderLeft, HeaderRight;
		Button.VSplitMid(&HeaderLeft, &HeaderRight, 3.0f);

		RenderTools()->DrawUIRect(&HeaderLeft, vec4(0.30f, 0.4f, 1.0f, 0.5f), CUI::CORNER_T, 5.0f);
		RenderTools()->DrawUIRect(&HeaderRight, vec4(0.0f, 0.0f, 0.0f, 0.5f), CUI::CORNER_T, 5.0f);

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s", Localize("Recommended"));
		HeaderLeft.y += 2;
		UI()->DoLabel(&HeaderLeft, aBuf, HeaderLeft.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

		str_format(aBuf, sizeof(aBuf), "%s", Localize("Other"));
		HeaderRight.y += 2;
		UI()->DoLabel(&HeaderRight, aBuf, HeaderRight.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);


		MainView.HSplitTop(Spacing, 0, &MainView);
		CUIRect ListRec, ListOth;
		MainView.VSplitMid(&ListRec, &ListOth, 3.0f);

		ListRec.HSplitBottom(ButtonHeight, &ListRec, &Button);
		ListRec.HSplitBottom(Spacing, &ListRec, 0);
		RenderTools()->DrawUIRect(&Button, vec4(0.0f, 0.0f, 0.0f, 0.5f), CUI::CORNER_B, 5.0f);
		int g = gcd(s_GfxScreenWidth, s_GfxScreenHeight);
		str_format(aBuf, sizeof(aBuf), Localize("Current: %dx%d (%d:%d)"), s_GfxScreenWidth, s_GfxScreenHeight, s_GfxScreenWidth/g, s_GfxScreenHeight/g);
		Button.y += 2;
		UI()->DoLabel(&Button, aBuf, Button.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

		static CListBox s_RecListBox;
		static CListBox s_OthListBox;
		m_CheckVideoSettings |= DoResolutionList(&ListRec, &s_RecListBox, m_lRecommendedVideoModes);
		m_CheckVideoSettings |= DoResolutionList(&ListOth, &s_OthListBox, m_lOtherVideoModes);
	}

	// reset button
	Spacing = 3.0f;
	float ButtonWidth = (BottomView.w/6.0f)-(Spacing*5.0)/6.0f;

	BottomView.VSplitRight(ButtonWidth, 0, &BottomView);
	RenderBackgroundShadow(&BottomView, true);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	Button = BottomView;
	static CButtonContainer s_ResetButton;
	if(DoButton_Menu(&s_ResetButton, Localize("Reset"), 0, &Button))
	{
		PopupConfirm(Localize("Reset graphics settings"), Localize("Are you sure that you want to reset the graphics settings to their defaults?"),
			Localize("Reset"), Localize("Cancel"), &CMenus::ResetSettingsGraphics);
	}

	// check if the new settings require a restart
	if(m_CheckVideoSettings)
	{
		m_NeedRestartGraphics =
			s_GfxScreenWidth != g_Config.m_GfxScreenWidth ||
			s_GfxScreenHeight != g_Config.m_GfxScreenHeight ||
			s_GfxFsaaSamples != g_Config.m_GfxFsaaSamples ||
			s_GfxTextureQuality != g_Config.m_GfxTextureQuality ||
			s_GfxTextureCompression != g_Config.m_GfxTextureCompression ||
			(CheckFullscreen && s_GfxFullscreen != g_Config.m_GfxFullscreen);
		m_CheckVideoSettings = false;
	}
}

void CMenus::RenderSettingsSound(CUIRect MainView)
{
	CUIRect Label, Button, Sound, Detail, BottomView, Background;

	// render sound menu background
	int NumOptions = g_Config.m_SndEnable ? 6 : 2;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;
	float TotalHeight = BackgroundHeight;
	if(g_Config.m_SndEnable)
		TotalHeight += 10.0f+2.0f*ButtonHeight+Spacing;

	MainView.HSplitBottom(MainView.h-TotalHeight-20.0f, &MainView, &BottomView);
	if(this->Client()->State() == IClient::STATE_ONLINE)
		Background = MainView;
	else
		MainView.HSplitTop(20.0f, 0, &Background);
	RenderTools()->DrawUIRect(&Background, vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), Client()->State() == IClient::STATE_OFFLINE ? CUI::CORNER_ALL : CUI::CORNER_B, 5.0f);
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &Sound, &MainView);
	RenderTools()->DrawUIRect(&Sound, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// render detail menu background
	if(g_Config.m_SndEnable)
	{
		BackgroundHeight = 2.0f*ButtonHeight+Spacing;

		MainView.HSplitTop(10.0f, 0, &MainView);
		MainView.HSplitTop(BackgroundHeight, &Detail, &MainView);
		RenderTools()->DrawUIRect(&Detail, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
	}

	static int s_SndInit = g_Config.m_SndInit;
	static int s_SndRate = g_Config.m_SndRate;

	// render sound menu
	Sound.HSplitTop(ButtonHeight, &Label, &Sound);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Sound"), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

	Sound.HSplitTop(Spacing, 0, &Sound);
	CUIRect UseSoundButton;
	Sound.HSplitTop(ButtonHeight, &UseSoundButton, &Sound);

	if(g_Config.m_SndEnable)
	{
		Sound.HSplitTop(Spacing, 0, &Sound);
		Sound.HSplitTop(ButtonHeight, &Button, &Sound);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		static int s_ButtonSndEnableUI = 0;
		if(DoButton_CheckBox(&s_ButtonSndEnableUI, Localize("Enable interface sounds"), g_Config.m_SndEnableUI, &Button))
			g_Config.m_SndEnableUI ^= 1;

		Sound.HSplitTop(Spacing, 0, &Sound);
		Sound.HSplitTop(ButtonHeight, &Button, &Sound);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		static int s_ButtonSndEnableRace = 0;
		if(DoButton_CheckBox(&s_ButtonSndEnableRace, Localize("Enable race sounds (checkpoints)"), g_Config.m_SndEnableRace, &Button))
			g_Config.m_SndEnableRace ^= 1;

		Sound.HSplitTop(Spacing, 0, &Sound);
		Sound.HSplitTop(ButtonHeight, &Button, &Sound);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		static int s_ButtonSndMusic = 0;
		if(DoButton_CheckBox(&s_ButtonSndMusic, Localize("Play background music"), g_Config.m_SndMusic, &Button))
		{
			g_Config.m_SndMusic ^= 1;
			UpdateMusicState();
		}

		Sound.HSplitTop(Spacing, 0, &Sound);
		Sound.HSplitTop(ButtonHeight, &Button, &Sound);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		static int s_ButtonSndMusicMRPG = 0;
		if(DoButton_CheckBox(&s_ButtonSndMusicMRPG, Localize("Enable atmosphere music (MRPG)"), g_Config.m_SndMusicMRPG, &Button))
		{
			g_Config.m_SndMusicMRPG ^= 1;
			m_pClient->UpdateStateMmoMusic();
		}
		
		Sound.HSplitTop(Spacing, 0, &Sound);
		Sound.HSplitTop(ButtonHeight, &Button, &Sound);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		static int s_ButtonSndNonactiveMute = 0;
		if(DoButton_CheckBox(&s_ButtonSndNonactiveMute, Localize("Mute when not active"), g_Config.m_SndNonactiveMute, &Button))
			g_Config.m_SndNonactiveMute ^= 1;

		// render detail menu
		Detail.HSplitTop(ButtonHeight, &Label, &Detail);
		Label.y += 2.0f;
		UI()->DoLabel(&Label, Localize("Detail"), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

		// split menu
		CUIRect Left, Right;
		Detail.HSplitTop(Spacing, 0, &Detail);
		Detail.VSplitMid(&Left, &Right, 3.0f);

		// sample rate thingy
		{
			Left.HSplitTop(ButtonHeight, &Button, &Left);

			RenderTools()->DrawUIRect(&Button, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
			CUIRect Text, Value, Unit;
			Button.VSplitLeft(Button.w/3.0f, &Text, &Button);
			Button.VSplitMid(&Value, &Unit);

			char aBuf[32];
			str_format(aBuf, sizeof(aBuf), "%s:", Localize("Sample rate"));
			Text.y += 2.0f;
			UI()->DoLabel(&Text, aBuf, Text.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

			Unit.y += 2.0f;
			UI()->DoLabel(&Unit, "kHz", Unit.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

			if(g_Config.m_SndRate != 48000 && g_Config.m_SndRate != 44100)
				g_Config.m_SndRate = 48000;
			if(g_Config.m_SndRate == 48000)
				str_copy(aBuf, "48.0", sizeof(aBuf));
			else
				str_copy(aBuf, "44.1", sizeof(aBuf));
			static CButtonContainer s_SampleRateButton;
			if(DoButton_Menu(&s_SampleRateButton, aBuf, 0, &Value))
			{
				if(g_Config.m_SndRate == 48000)
					g_Config.m_SndRate = 44100;
				else
					g_Config.m_SndRate = 48000;
			}

			m_NeedRestartSound = g_Config.m_SndInit && (!s_SndInit || s_SndRate != g_Config.m_SndRate);
		}

		Right.HSplitTop(ButtonHeight, &Button, &Right);
		DoScrollbarOption(&g_Config.m_SndVolume, &g_Config.m_SndVolume, &Button, Localize("Volume"), 0, 100, &LogarithmicScrollbarScale);
	}
	else
	{
		Sound.HSplitTop(Spacing, 0, &Sound);
		Sound.HSplitTop(ButtonHeight, &Button, &Sound);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		static int s_ButtonInitSounds = 0;
		if(DoButton_CheckBox(&s_ButtonInitSounds, Localize("Load the sound system"), g_Config.m_SndInit, &Button))
		{
			g_Config.m_SndInit ^= 1;
			m_NeedRestartSound = g_Config.m_SndInit && (!s_SndInit || s_SndRate != g_Config.m_SndRate);
		}
	}

	static int s_ButtonSndEnable = 0;
	if(DoButton_CheckBox(&s_ButtonSndEnable, Localize("Use sounds"), g_Config.m_SndEnable, &UseSoundButton))
	{
		g_Config.m_SndEnable ^= 1;
		if(g_Config.m_SndEnable)
		{
			g_Config.m_SndInit = 1;
		}
		UpdateMusicState();
		m_pClient->UpdateStateMmoMusic();
	}

	// reset button
	BottomView.HSplitBottom(60.0f, 0, &BottomView);

	Spacing = 3.0f;
	float ButtonWidth = (BottomView.w/6.0f)-(Spacing*5.0)/6.0f;

	BottomView.VSplitRight(ButtonWidth, 0, &BottomView);
	RenderBackgroundShadow(&BottomView, true);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	Button = BottomView;
	static CButtonContainer s_ResetButton;
	if(DoButton_Menu(&s_ResetButton, Localize("Reset"), 0, &Button))
	{
		PopupConfirm(Localize("Reset sound settings"), Localize("Are you sure that you want to reset the sound settings to their defaults?"),
			Localize("Reset"), Localize("Cancel"), &CMenus::ResetSettingsSound);
	}
}


void CMenus::ResetSettingsGeneral()
{
	g_Config.m_ClDynamicCamera = 0;
	g_Config.m_ClMouseMaxDistanceStatic = 400;
	g_Config.m_ClMouseMaxDistanceDynamic = 1000;
	g_Config.m_ClMouseFollowfactor = 60;
	g_Config.m_ClMouseDeadzone = 300;
	g_Config.m_ClAutoswitchWeapons = 1;
	g_Config.m_ClShowhud = 1;
	g_Config.m_ClFilterchat = 0;
	g_Config.m_ClNameplates = 1;
	g_Config.m_ClNameplatesAlways = 1;
	g_Config.m_ClNameplatesSize = 30;
	g_Config.m_ClNameplatesTeamcolors = 1;
	g_Config.m_ClAutoDemoRecord = 0;
	g_Config.m_ClAutoDemoMax = 10;
	g_Config.m_ClAutoScreenshot = 0;
	g_Config.m_ClAutoScreenshotMax = 10;
}

void CMenus::ResetSettingsControls()
{
	m_pClient->m_pBinds->SetDefaults();
}

void CMenus::ResetSettingsGraphics()
{
	g_Config.m_GfxScreenWidth = Graphics()->DesktopWidth();
	g_Config.m_GfxScreenHeight = Graphics()->DesktopHeight();
	g_Config.m_GfxBorderless = 0;
	g_Config.m_GfxFullscreen = 1;
	g_Config.m_GfxVsync = 0;
	g_Config.m_GfxFsaaSamples = 0;
	g_Config.m_GfxTextureQuality = 1;
	g_Config.m_GfxTextureCompression = 0;
	g_Config.m_GfxHighDetail = 1;

	if(g_Config.m_GfxDisplayAllModes)
	{
		g_Config.m_GfxDisplayAllModes = 0;
		UpdateVideoModeSettings();
	}

	m_CheckVideoSettings = true;
}

void CMenus::ResetSettingsSound()
{
	g_Config.m_SndEnable = 1;
	g_Config.m_SndInit = 1;
	g_Config.m_SndMusic = 1;
	g_Config.m_SndMusicMRPG = 1;
	g_Config.m_SndNonactiveMute = 0;
	g_Config.m_SndRate = 48000;
	g_Config.m_SndVolume = 100;
	UpdateMusicState();
	m_pClient->UpdateStateMmoMusic();
}

void CMenus::PopupConfirmPlayerCountry()
{
	if(m_PopupSelection != -2)
		g_Config.m_PlayerCountry = m_PopupSelection;
}

void CMenus::RenderSettings(CUIRect MainView)
{
	// handle which page should be rendered
	if(g_Config.m_UiSettingsPage == SETTINGS_GENERAL)
		RenderSettingsGeneral(MainView);
	else if(g_Config.m_UiSettingsPage == SETTINGS_PLAYER)
		RenderSettingsPlayer(MainView);
	else if(g_Config.m_UiSettingsPage == SETTINGS_TBD) // TODO: replace removed tee page to something else	
		g_Config.m_UiSettingsPage = SETTINGS_PLAYER; // TODO: remove this
	else if(g_Config.m_UiSettingsPage == SETTINGS_CONTROLS)
		RenderSettingsControls(MainView);
	else if(g_Config.m_UiSettingsPage == SETTINGS_GRAPHICS)
		RenderSettingsGraphics(MainView);
	else if(g_Config.m_UiSettingsPage == SETTINGS_SOUND)
		RenderSettingsSound(MainView);
	//mmotee
	else if (g_Config.m_UiSettingsPage == SETTINGS_MMO)
		RenderSettingsMmo(MainView);

	MainView.HSplitBottom(32.0f, 0, &MainView);

	// reset warning
	bool NeedReconnect = (m_NeedRestartPlayer || (m_SkinModified && m_pClient->m_LastSkinChangeTime + 6.0f > Client()->LocalTime())) && this->Client()->State() == IClient::STATE_ONLINE;
	// backwards compatibility
	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);
	const char *pVersion = str_startswith(CurrentServerInfo.m_aVersion, "0.7.");
	bool NeedRestartTee = pVersion && (pVersion[0] == '1' || pVersion[0] == '2') && pVersion[1] == 0;

	if(m_NeedRestartGraphics || m_NeedRestartSound || NeedReconnect || NeedRestartTee)
	{
		// background
		CUIRect RestartWarning;
		MainView.HSplitTop(25.0f, &RestartWarning, 0);
		RestartWarning.VMargin(140.0f, &RestartWarning);
		RenderTools()->DrawUIRect(&RestartWarning, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		// text
		TextRender()->TextColor(0.973f, 0.863f, 0.207f, 1.0f);
		RestartWarning.y += 2.0f;
		if(m_NeedRestartGraphics || m_NeedRestartSound)
			UI()->DoLabel(&RestartWarning, Localize("You must restart the game for all settings to take effect."), RestartWarning.h*ms_FontmodHeight*0.75f, CUI::ALIGN_CENTER);
		else if(Client()->State() == IClient::STATE_ONLINE)
		{
			if(m_NeedRestartPlayer || NeedRestartTee)
				UI()->DoLabel(&RestartWarning, Localize("You must reconnect to change identity."), RestartWarning.h*ms_FontmodHeight*0.75f, CUI::ALIGN_CENTER);
			else if(m_SkinModified)
			{
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), Localize("You have to wait %1.0f seconds to change identity."), m_pClient->m_LastSkinChangeTime+6.5f - Client()->LocalTime());
				UI()->DoLabel(&RestartWarning, aBuf, RestartWarning.h*ms_FontmodHeight*0.75f, CUI::ALIGN_CENTER);
			}
		}
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}

	RenderBackButton(MainView);
}
