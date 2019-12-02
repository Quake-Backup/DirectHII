
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"

//=============================================================================
/* MODEM CONFIG MENU */

int		modemConfig_cursor;
int		modemConfig_cursor_table [] = {40, 56, 88, 120, 156};
#define NUM_MODEMCONFIG_CMDS	5

char	modemConfig_dialing;
char	modemConfig_clear [16];
char	modemConfig_init [32];
char	modemConfig_hangup [16];

void M_Menu_ModemConfig_f (void)
{
	key_dest = key_menu;
	m_state = m_modemconfig;
	m_soundlevel = m_sound_menu2;
	(*GetModemConfig) (0, &modemConfig_dialing, modemConfig_clear, modemConfig_init, modemConfig_hangup);
}


void M_ModemConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;

	ScrollTitle ("gfx/menu/title4.lmp");
	p = Draw_CachePic ("gfx/menu/title4.lmp");
	basex = (320 - p->width) / 2;
	basex += 8;

	if (modemConfig_dialing == 'P')
		M_Print (basex, modemConfig_cursor_table[0], "Pulse Dialing");
	else
		M_Print (basex, modemConfig_cursor_table[0], "Touch Tone Dialing");

	M_Print (basex, modemConfig_cursor_table[1], "Clear");
	M_DrawTextBox (basex, modemConfig_cursor_table[1] + 4, 16, 1);
	M_Print (basex + 8, modemConfig_cursor_table[1] + 12, modemConfig_clear);

	if (modemConfig_cursor == 1)
		M_DrawCharacter (basex + 8 + 8 * strlen (modemConfig_clear), modemConfig_cursor_table[1] + 12, 10 + ((int) (realtime * 4) & 1));

	M_Print (basex, modemConfig_cursor_table[2], "Init");
	M_DrawTextBox (basex, modemConfig_cursor_table[2] + 4, 30, 1);
	M_Print (basex + 8, modemConfig_cursor_table[2] + 12, modemConfig_init);

	if (modemConfig_cursor == 2)
		M_DrawCharacter (basex + 8 + 8 * strlen (modemConfig_init), modemConfig_cursor_table[2] + 12, 10 + ((int) (realtime * 4) & 1));

	M_Print (basex, modemConfig_cursor_table[3], "Hangup");
	M_DrawTextBox (basex, modemConfig_cursor_table[3] + 4, 16, 1);
	M_Print (basex + 8, modemConfig_cursor_table[3] + 12, modemConfig_hangup);

	if (modemConfig_cursor == 3)
		M_DrawCharacter (basex + 8 + 8 * strlen (modemConfig_hangup), modemConfig_cursor_table[3] + 12, 10 + ((int) (realtime * 4) & 1));

	M_DrawTextBox (basex, modemConfig_cursor_table[4] - 8, 2, 1);
	M_Print (basex + 8, modemConfig_cursor_table[4], "OK");

	M_DrawCharacter (basex - 8, modemConfig_cursor_table [modemConfig_cursor], 12 + ((int) (realtime * 4) & 1));
}


void M_ModemConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_SerialConfig_f ();
		break;

	case K_UPARROW:
		m_soundlevel = m_sound_menu1;
		modemConfig_cursor--;

		if (modemConfig_cursor < 0)
			modemConfig_cursor = NUM_MODEMCONFIG_CMDS - 1;

		break;

	case K_DOWNARROW:
		m_soundlevel = m_sound_menu1;
		modemConfig_cursor++;

		if (modemConfig_cursor >= NUM_MODEMCONFIG_CMDS)
			modemConfig_cursor = 0;

		break;

	case K_LEFTARROW:
	case K_RIGHTARROW:

		if (modemConfig_cursor == 0)
		{
			if (modemConfig_dialing == 'P')
				modemConfig_dialing = 'T';
			else
				modemConfig_dialing = 'P';

			m_soundlevel = m_sound_menu1;
		}

		break;

	case K_ENTER:

		if (modemConfig_cursor == 0)
		{
			if (modemConfig_dialing == 'P')
				modemConfig_dialing = 'T';
			else
				modemConfig_dialing = 'P';

			m_soundlevel = m_sound_menu2;
		}

		if (modemConfig_cursor == 4)
		{
			(*SetModemConfig) (0, va ("%c", modemConfig_dialing), modemConfig_clear, modemConfig_init, modemConfig_hangup);
			m_soundlevel = m_sound_menu2;
			M_Menu_SerialConfig_f ();
		}

		break;

	case K_BACKSPACE:

		if (modemConfig_cursor == 1)
		{
			if (strlen (modemConfig_clear))
				modemConfig_clear[strlen (modemConfig_clear)-1] = 0;
		}

		if (modemConfig_cursor == 2)
		{
			if (strlen (modemConfig_init))
				modemConfig_init[strlen (modemConfig_init)-1] = 0;
		}

		if (modemConfig_cursor == 3)
		{
			if (strlen (modemConfig_hangup))
				modemConfig_hangup[strlen (modemConfig_hangup)-1] = 0;
		}

		break;

	default:

		if (key < 32 || key > 127)
			break;

		if (modemConfig_cursor == 1)
		{
			l = strlen (modemConfig_clear);

			if (l < 15)
			{
				modemConfig_clear[l+1] = 0;
				modemConfig_clear[l] = key;
			}
		}

		if (modemConfig_cursor == 2)
		{
			l = strlen (modemConfig_init);

			if (l < 29)
			{
				modemConfig_init[l+1] = 0;
				modemConfig_init[l] = key;
			}
		}

		if (modemConfig_cursor == 3)
		{
			l = strlen (modemConfig_hangup);

			if (l < 15)
			{
				modemConfig_hangup[l+1] = 0;
				modemConfig_hangup[l] = key;
			}
		}
	}
}
