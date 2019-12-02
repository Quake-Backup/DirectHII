
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"


//=============================================================================
/* SLIST MENU */

// yuck, this bullshit
extern int m_multiplayer_cursor;
extern int m_net_cursor;
extern qboolean	m_return_onerror;
extern char		m_return_reason [32];
extern m_state_t m_return_state;


int		slist_cursor;
qboolean slist_sorted;

void M_Menu_ServerList_f (void)
{
	key_dest = key_menu;
	m_state = m_slist;
	m_soundlevel = m_sound_menu2;
	slist_cursor = 0;
	m_return_onerror = false;
	m_return_reason[0] = 0;
	slist_sorted = false;
}


void M_ServerList_Draw (void)
{
	int		n;
	char	string [64], *name;

	if (!slist_sorted)
	{
		if (hostCacheCount > 1)
		{
			int	i, j;
			hostcache_t temp;

			for (i = 0; i < hostCacheCount; i++)
				for (j = i + 1; j < hostCacheCount; j++)
					if (strcmp (hostcache[j].name, hostcache[i].name) < 0)
					{
						memcpy (&temp, &hostcache[j], sizeof (hostcache_t));
						memcpy (&hostcache[j], &hostcache[i], sizeof (hostcache_t));
						memcpy (&hostcache[i], &temp, sizeof (hostcache_t));
					}
		}

		slist_sorted = true;
	}

	ScrollTitle ("gfx/menu/title4.lmp");

	for (n = 0; n < hostCacheCount; n++)
	{
		if (hostcache[n].driver == 0)
			name = net_drivers[hostcache[n].driver].name;
		else
			name = net_landrivers[hostcache[n].ldriver].name;

		if (hostcache[n].maxusers)
			sprintf (string, "%-11.11s %-8.8s %-10.10s %2u/%2u\n", hostcache[n].name, name, hostcache[n].map, hostcache[n].users, hostcache[n].maxusers);
		else
			sprintf (string, "%-11.11s %-8.8s %-10.10s\n", hostcache[n].name, name, hostcache[n].map);

		M_Print (16, 60 + 8 * n, string);
	}

	M_DrawCharacter (0, 60 + slist_cursor * 8, 12 + ((int) (realtime * 4) & 1));

	if (*m_return_reason)
		M_PrintWhite (16, 176, m_return_reason);
}


void M_ServerList_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_LanConfig_f ();
		break;

	case K_SPACE:
		M_Menu_Search_f ();
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		m_soundlevel = m_sound_menu1;
		slist_cursor--;

		if (slist_cursor < 0)
			slist_cursor = hostCacheCount - 1;

		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		m_soundlevel = m_sound_menu1;
		slist_cursor++;

		if (slist_cursor >= hostCacheCount)
			slist_cursor = 0;

		break;

	case K_ENTER:
		m_soundlevel = m_sound_menu2;
		m_return_state = m_state;
		m_return_onerror = true;
		slist_sorted = false;
		key_dest = key_game;
		m_state = m_none;
		Cbuf_AddText (va ("connect \"%s\"\n", hostcache[slist_cursor].cname));
		break;

	default:
		break;
	}
}

