
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"


//=============================================================================
/* HELP MENU */

int		help_page;
#define	NUM_HELP_PAGES	5


void M_Menu_Help_f (void)
{
	key_dest = key_menu;
	m_state = m_help;
	m_soundlevel = m_sound_menu2;
	help_page = 0;
}



void M_Help_Draw (void)
{
	M_DrawPic (0, 0, Draw_CachePic (va ("gfx/menu/help%02i.lmp", help_page + 1)));
}


void M_Help_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_UPARROW:
	case K_RIGHTARROW:
		m_soundlevel = m_sound_menu2;

		if (++help_page >= NUM_HELP_PAGES)
			help_page = 0;

		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		m_soundlevel = m_sound_menu2;

		if (--help_page < 0)
			help_page = NUM_HELP_PAGES - 1;

		break;
	}

}
