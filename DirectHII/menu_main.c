

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"


//=============================================================================
/* MAIN MENU */

int	m_main_cursor;
int m_save_demonum;

#define	MAIN_ITEMS	5


void M_Menu_Main_f (void)
{
	if (key_dest != key_menu)
	{
		m_save_demonum = cls.demonum;
		cls.demonum = -1;
	}

	key_dest = key_menu;
	m_state = m_main;
	m_soundlevel = m_sound_menu2;
}


void M_Main_Draw (void)
{
	int		f;

	ScrollTitle ("gfx/menu/title0.lmp");

	M_DrawBigString (72, 60 + (0 * 20), "SINGLE PLAYER");
	M_DrawBigString (72, 60 + (1 * 20), "MULTIPLAYER");
	M_DrawBigString (72, 60 + (2 * 20), "OPTIONS");
	M_DrawBigString (72, 60 + (3 * 20), "HELP");
	M_DrawBigString (72, 60 + (4 * 20), "QUIT");

	f = (int) (realtime * 10) % 8;
	M_DrawTransPic (43, 54 + m_main_cursor * 20, Draw_CachePic (va ("gfx/menu/menudot%i.lmp", f + 1)));
}


void M_Main_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		key_dest = key_game;
		m_state = m_none;
		cls.demonum = m_save_demonum;

		if (cls.demonum != -1 && !cls.demoplayback && cls.state != ca_connected)
			CL_NextDemo ();

		break;

	case K_DOWNARROW:
		m_soundlevel = m_sound_menu1;

		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;

		break;

	case K_UPARROW:
		m_soundlevel = m_sound_menu1;

		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;

		break;

	case K_ENTER:
		m_soundlevel = m_sound_menu2;

		switch (m_main_cursor)
		{
		case 0:
			M_Menu_SinglePlayer_f ();
			break;

		case 1:
			M_Menu_MultiPlayer_f ();
			break;

		case 2:
			M_Menu_Options_f ();
			break;

		case 3:
			M_Menu_Help_f ();
			break;

		case 4:
			M_Menu_Quit_f ();
			break;
		}
	}
}

