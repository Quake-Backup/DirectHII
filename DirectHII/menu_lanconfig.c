
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"


//=============================================================================
/* LAN CONFIG MENU */

// yuck, this bullshit
extern int m_multiplayer_cursor;
extern int m_net_cursor;
extern qboolean	m_return_onerror;
extern char		m_return_reason[32];
extern m_state_t m_return_state;


int		lanConfig_cursor = -1;
int		lanConfig_cursor_table[] = {100, 120, 140, 172};
#define NUM_LANCONFIG_CMDS	4

int 	lanConfig_port;
char	lanConfig_portname[6];
char	lanConfig_joinname[30];

void M_Menu_LanConfig_f (void)
{
	key_dest = key_menu;
	m_state = m_lanconfig;
	m_soundlevel = m_sound_menu2;

	if (lanConfig_cursor == -1)
	{
		if (JoiningGame && TCPIPConfig)
			lanConfig_cursor = 2;
		else
			lanConfig_cursor = 1;
	}

	if (StartingGame && lanConfig_cursor >= 2)
		lanConfig_cursor = 1;

	lanConfig_port = DEFAULTnet_hostport;
	sprintf (lanConfig_portname, "%u", lanConfig_port);

	m_return_onerror = false;
	m_return_reason[0] = 0;

	setup_class = cl_playerclass.value;

	if (setup_class < 1 || setup_class > NUM_CLASSES)
		setup_class = NUM_CLASSES;

	setup_class--;
}


void M_LanConfig_Draw (void)
{
	int		basex;
	char	*startJoin;
	char	*protocol;

	ScrollTitle ("gfx/menu/title4.lmp");
	basex = 48;

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";

	if (IPXConfig)
		protocol = "IPX";
	else
		protocol = "TCP/IP";

	M_Print (basex, 60, va ("%s - %s", startJoin, protocol));
	basex += 8;

	M_Print (basex, 80, "Address:");

	if (IPXConfig)
		M_Print (basex + 9 * 8, 80, my_ipx_address);
	else
		M_Print (basex + 9 * 8, 80, my_tcpip_address);

	M_Print (basex, lanConfig_cursor_table[0], "Port");
	M_DrawTextBox (basex + 8 * 8, lanConfig_cursor_table[0] - 8, 6, 1);
	M_Print (basex + 9 * 8, lanConfig_cursor_table[0], lanConfig_portname);

	if (JoiningGame)
	{
		M_Print (basex, lanConfig_cursor_table[1], "Class:");
		M_Print (basex + 8 * 7, lanConfig_cursor_table[1], ClassNames[setup_class]);

		M_Print (basex, lanConfig_cursor_table[2], "Search for local games...");
		M_Print (basex, 156, "Join game at:");
		M_DrawTextBox (basex, lanConfig_cursor_table[3] - 8, 30, 1);
		M_Print (basex + 8, lanConfig_cursor_table[3], lanConfig_joinname);
	}
	else
	{
		M_DrawTextBox (basex, lanConfig_cursor_table[1] - 8, 2, 1);
		M_Print (basex + 8, lanConfig_cursor_table[1], "OK");
	}

	M_DrawCharacter (basex - 8, lanConfig_cursor_table[lanConfig_cursor], 12 + ((int) (realtime * 4) & 1));

	if (lanConfig_cursor == 0)
		M_DrawCharacter (basex + 9 * 8 + 8 * strlen (lanConfig_portname), lanConfig_cursor_table[0], 10 + ((int) (realtime * 4) & 1));

	if (lanConfig_cursor == 3)
		M_DrawCharacter (basex + 8 + 8 * strlen (lanConfig_joinname), lanConfig_cursor_table[3], 10 + ((int) (realtime * 4) & 1));

	if (*m_return_reason)
		M_PrintWhite (basex, 192, m_return_reason);
}


void M_LanConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		m_soundlevel = m_sound_menu1;
		lanConfig_cursor--;

		if (JoiningGame)
		{
			if (lanConfig_cursor < 0)
				lanConfig_cursor = NUM_LANCONFIG_CMDS - 1;
		}
		else
		{
			if (lanConfig_cursor < 0)
				lanConfig_cursor = NUM_LANCONFIG_CMDS - 2;
		}

		break;

	case K_DOWNARROW:
		m_soundlevel = m_sound_menu1;
		lanConfig_cursor++;

		if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
			lanConfig_cursor = 0;

		break;

	case K_ENTER:

		if ((JoiningGame && lanConfig_cursor <= 1) ||
			(!JoiningGame && lanConfig_cursor == 0))
			break;

		m_soundlevel = m_sound_menu2;

		if (JoiningGame)
			Cbuf_AddText (va ("playerclass %d\n", setup_class + 1));

		M_ConfigureNetSubsystem ();

		if ((JoiningGame && lanConfig_cursor == 2) ||
			(!JoiningGame && lanConfig_cursor == 1))
		{
			if (StartingGame)
			{
				M_Menu_GameOptions_f ();
				break;
			}

			M_Menu_Search_f ();
			break;
		}

		if (lanConfig_cursor == 3)
		{
			m_return_state = m_state;
			m_return_onerror = true;
			key_dest = key_game;
			m_state = m_none;
			Cbuf_AddText (va ("connect \"%s\"\n", lanConfig_joinname));
			break;
		}

		break;

	case K_BACKSPACE:

		if (lanConfig_cursor == 0)
		{
			if (strlen (lanConfig_portname))
				lanConfig_portname[strlen (lanConfig_portname) - 1] = 0;
		}

		if (lanConfig_cursor == 3)
		{
			if (strlen (lanConfig_joinname))
				lanConfig_joinname[strlen (lanConfig_joinname) - 1] = 0;
		}

		break;

	case K_LEFTARROW:

		if (lanConfig_cursor != 1 || !JoiningGame)
			break;

		m_soundlevel = m_sound_menu3;
		setup_class--;

		if (setup_class < 0)
			setup_class = NUM_CLASSES - 1;

		break;

	case K_RIGHTARROW:

		if (lanConfig_cursor != 1 || !JoiningGame)
			break;

		m_soundlevel = m_sound_menu3;
		setup_class++;

		if (setup_class > NUM_CLASSES - 1)
			setup_class = 0;

		break;

	default:

		if (key < 32 || key > 127)
			break;

		if (lanConfig_cursor == 3)
		{
			l = strlen (lanConfig_joinname);

			if (l < 29)
			{
				lanConfig_joinname[l + 1] = 0;
				lanConfig_joinname[l] = key;
			}
		}

		if (key < '0' || key > '9')
			break;

		if (lanConfig_cursor == 0)
		{
			l = strlen (lanConfig_portname);

			if (l < 5)
			{
				lanConfig_portname[l + 1] = 0;
				lanConfig_portname[l] = key;
			}
		}
	}

	if (StartingGame && lanConfig_cursor == 2)
		if (key == K_UPARROW)
			lanConfig_cursor = 1;
		else
			lanConfig_cursor = 0;

	l = atoi (lanConfig_portname);

	if (l > 65535)
		l = lanConfig_port;
	else
		lanConfig_port = l;

	sprintf (lanConfig_portname, "%u", lanConfig_port);
}

