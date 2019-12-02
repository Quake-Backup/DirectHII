
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"


//=============================================================================
/* CLASS CHOICE MENU */

char *ClassNames[NUM_CLASSES] =
{
	"Paladin",
	"Crusader",
	"Necromancer",
	"Assassin"
};

char *ClassNamesU[NUM_CLASSES] =
{
	"PALADIN",
	"CRUSADER",
	"NECROMANCER",
	"ASSASSIN"
};

int class_flag;

void M_Menu_Class_f (void)
{
	class_flag = 0;
	key_dest = key_menu;
	m_state = m_class;
}

void M_Menu_Class2_f (void)
{
	key_dest = key_menu;
	m_state = m_class;
	class_flag = 1;
}


int	m_class_cursor;
#define	CLASS_ITEMS	NUM_CLASSES

void M_Class_Draw (void)
{
	int		f, i;

	ScrollTitle ("gfx/menu/title2.lmp");

	for (i = 0; i < NUM_CLASSES; ++i)
		M_DrawBigString (72, 60 + (i * 20), ClassNamesU[i]);

	f = (int) (realtime * 10) % 8;
	M_DrawTransPic (43, 54 + m_class_cursor * 20, Draw_CachePic (va ("gfx/menu/menudot%i.lmp", f + 1)));

	M_DrawPic (251, 54 + 21, Draw_CachePic (va ("gfx/cport%d.lmp", m_class_cursor + 1)));
	M_DrawTransPic (242, 54, Draw_CachePic ("gfx/menu/frame.lmp"));
}

void M_Class_Key (int key)
{
	switch (key)
	{
	case K_LEFTARROW:
	case K_RIGHTARROW:
		break;
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_DOWNARROW:
		m_soundlevel = m_sound_menu1;

		if (++m_class_cursor >= CLASS_ITEMS)
			m_class_cursor = 0;

		break;

	case K_UPARROW:
		m_soundlevel = m_sound_menu1;

		if (--m_class_cursor < 0)
			m_class_cursor = CLASS_ITEMS - 1;

		break;

	case K_ENTER:
		//		sv_player->v.playerclass=m_class_cursor+1;
		Cbuf_AddText (va ("playerclass %d\n", m_class_cursor + 1));
		m_soundlevel = m_sound_menu2;

		if (!class_flag)
		{
			M_Menu_Difficulty_f ();
		}
		else
		{
			key_dest = key_game;
			m_state = m_none;
		}

		break;
	default:
		key_dest = key_game;
		m_state = m_none;
		break;
	}
}


