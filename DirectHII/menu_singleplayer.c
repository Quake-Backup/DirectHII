
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"


//=============================================================================
/* SINGLE PLAYER MENU */

int	m_singleplayer_cursor;
#define	SINGLEPLAYER_ITEMS	3


void M_Menu_SinglePlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_singleplayer;
	m_soundlevel = m_sound_menu2;
	Cvar_SetValue ("timelimit", 0);		//put this here to help play single after dm
}

void M_SinglePlayer_Draw (void)
{
	int		f;

	ScrollTitle ("gfx/menu/title1.lmp");

	M_DrawBigString (72, 60, "NEW MISSION");
	M_DrawBigString (72, 80, "LOAD");
	M_DrawBigString (72, 100, "SAVE");

	f = (int) (realtime * 10) % 8;
	M_DrawTransPic (43, 54 + m_singleplayer_cursor * 20, Draw_CachePic (va ("gfx/menu/menudot%i.lmp", f + 1)));
}


void M_SinglePlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_DOWNARROW:
		m_soundlevel = m_sound_menu1;

		if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
			m_singleplayer_cursor = 0;

		break;

	case K_UPARROW:
		m_soundlevel = m_sound_menu1;

		if (--m_singleplayer_cursor < 0)
			m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;

		break;

	case K_ENTER:
		m_soundlevel = m_sound_menu2;

		switch (m_singleplayer_cursor)
		{
		case 0:
			if (sv.active)
				if (!SCR_ModalMessage ("Are you sure you want to\nstart a new game?\n"))
					break;

			key_dest = key_game;

			if (sv.active)
				Cbuf_AddText ("disconnect\n");

			CL_RemoveGIPFiles (NULL);
			Cbuf_AddText ("maxplayers 1\n");
			M_Menu_Class_f ();
			break;

		case 1:
			M_Menu_Load_f ();
			break;

		case 2:
			M_Menu_Save_f ();
			break;
		}
	}
}

