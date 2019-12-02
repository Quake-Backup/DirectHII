
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"


//=============================================================================
/* MULTIPLAYER MENU */

int	m_multiplayer_cursor;
#define	MULTIPLAYER_ITEMS	5

void M_Menu_MultiPlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_multiplayer;
	m_soundlevel = m_sound_menu2;

	message = NULL;
}


void M_MultiPlayer_Draw (void)
{
	int		f;

	ScrollTitle ("gfx/menu/title4.lmp");
	//	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/mp_menu.lmp") );

	M_DrawBigString (72, 60 + (0 * 20), "JOIN A GAME");
	M_DrawBigString (72, 60 + (1 * 20), "NEW GAME");
	M_DrawBigString (72, 60 + (2 * 20), "SETUP");
	M_DrawBigString (72, 60 + (3 * 20), "LOAD");
	M_DrawBigString (72, 60 + (4 * 20), "SAVE");

	f = (int) (realtime * 10) % 8;
	M_DrawTransPic (43, 54 + m_multiplayer_cursor * 20, Draw_CachePic (va ("gfx/menu/menudot%i.lmp", f + 1)));

	if (message)
	{
		M_PrintWhite ((320 / 2) - ((27 * 8) / 2), 168, message);
		M_PrintWhite ((320 / 2) - ((27 * 8) / 2), 176, message2);

		if (realtime - 5 > message_time)
			message = NULL;
	}

	if (serialAvailable || ipxAvailable || tcpipAvailable)
		return;

	M_PrintWhite ((320 / 2) - ((27 * 8) / 2), 160, "No Communications Available");
}


void M_MultiPlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_DOWNARROW:
		m_soundlevel = m_sound_menu1;

		if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
			m_multiplayer_cursor = 0;

		break;

	case K_UPARROW:
		m_soundlevel = m_sound_menu1;

		if (--m_multiplayer_cursor < 0)
			m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;

		break;

	case K_ENTER:
		m_soundlevel = m_sound_menu2;

		switch (m_multiplayer_cursor)
		{
		case 0:

			if (serialAvailable || ipxAvailable || tcpipAvailable)
				M_Menu_Net_f ();

			break;

		case 1:

			if (serialAvailable || ipxAvailable || tcpipAvailable)
				M_Menu_Net_f ();

			break;

		case 2:
			M_Menu_Setup_f ();
			break;

		case 3:
			M_Menu_MLoad_f ();
			break;

		case 4:
			M_Menu_MSave_f ();
			break;
		}
	}
}


