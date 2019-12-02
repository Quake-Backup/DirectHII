
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"


//=============================================================================
/* SEARCH MENU */

qboolean	searchComplete = false;
double		searchCompleteTime;

void M_Menu_Search_f (void)
{
	key_dest = key_menu;
	m_state = m_search;
	m_soundlevel = m_sound_none;
	slistSilent = true;
	slistLocal = false;
	searchComplete = false;
	NET_Slist_f ();

}


void M_Search_Draw (void)
{
	int x;

	ScrollTitle ("gfx/menu/title4.lmp");

	x = (320 / 2) - ((12 * 8) / 2) + 4;
	M_DrawTextBox (x - 8, 60, 12, 1);
	M_Print (x, 68, "Searching...");

	if (slistInProgress)
	{
		NET_Poll ();
		return;
	}

	if (!searchComplete)
	{
		searchComplete = true;
		searchCompleteTime = realtime;
	}

	if (hostCacheCount)
	{
		M_Menu_ServerList_f ();
		return;
	}

	M_PrintWhite ((320 / 2) - ((22 * 8) / 2), 92, "No Hexen II servers found");

	if ((realtime - searchCompleteTime) < 3.0)
		return;

	M_Menu_LanConfig_f ();
}


void M_Search_Key (int key)
{
}

