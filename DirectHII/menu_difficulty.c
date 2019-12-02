

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"


//=============================================================================
/* DIFFICULTY MENU */

char *DiffNames[NUM_CLASSES][4] =
{
	{
		// Paladin
		"APPRENTICE",
		"SQUIRE",
		"ADEPT",
		"LORD"
	},
	{
		// Crusader
		"GALLANT",
		"HOLY AVENGER",
		"DIVINE HERO",
		"LEGEND"
	},
	{
		// Necromancer
		"SORCERER",
		"DARK SERVANT",
		"WARLOCK",
		"LICH KING"
	},
	{
		// Assassin
		"ROGUE",
		"CUTTHROAT",
		"EXECUTIONER",
		"WIDOW MAKER"
	}
};

int		setup_class;


void M_Menu_Difficulty_f (void)
{
	key_dest = key_menu;
	m_state = m_difficulty;
}

int	m_diff_cursor;
#define	DIFF_ITEMS	NUM_DIFFLEVELS

void M_Difficulty_Draw (void)
{
	int		f, i;

	ScrollTitle ("gfx/menu/title5.lmp");

	setup_class = cl_playerclass.value;

	if (setup_class < 1 || setup_class > NUM_CLASSES)
		setup_class = NUM_CLASSES;

	setup_class--;

	for (i = 0; i < NUM_DIFFLEVELS; ++i)
		M_DrawBigString (72, 60 + (i * 20), DiffNames[setup_class][i]);

	f = (int) (realtime * 10) % 8;
	M_DrawTransPic (43, 54 + m_diff_cursor * 20, Draw_CachePic (va ("gfx/menu/menudot%i.lmp", f + 1)));
}

void M_Difficulty_Key (int key)
{
	switch (key)
	{
	case K_LEFTARROW:
	case K_RIGHTARROW:
		break;
	case K_ESCAPE:
		M_Menu_Class_f ();
		break;

	case K_DOWNARROW:
		m_soundlevel = m_sound_menu1;

		if (++m_diff_cursor >= DIFF_ITEMS)
			m_diff_cursor = 0;

		break;

	case K_UPARROW:
		m_soundlevel = m_sound_menu1;

		if (--m_diff_cursor < 0)
			m_diff_cursor = DIFF_ITEMS - 1;

		break;
	case K_ENTER:
		Cvar_SetValue ("skill", m_diff_cursor);
		m_soundlevel = m_sound_menu2;
		Cbuf_AddText ("map demo1\n");
		break;
	default:
		key_dest = key_game;
		m_state = m_none;
		break;
	}
}

