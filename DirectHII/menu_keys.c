
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"

//=============================================================================
/* KEYS MENU */

char *bindnames[][2] =
{
	{"+attack", 		"attack"},
	{"impulse 10", 		"change weapon"},
	{"+jump", 			"jump / swim up"},
	{"+forward", 		"walk forward"},
	{"+back", 			"backpedal"},
	{"+left", 			"turn left"},
	{"+right", 			"turn right"},
	{"+speed", 			"run"},
	{"+moveleft", 		"step left"},
	{"+moveright", 		"step right"},
	{"+strafe", 		"sidestep"},
	{"+crouch",			"crouch"},
	{"+lookup", 		"look up"},
	{"+lookdown", 		"look down"},
	{"centerview", 		"center view"},
	{"+mlook", 			"mouse look"},
	{"+klook", 			"keyboard look"},
	{"+moveup",			"swim up"},
	{"+movedown",		"swim down"},
	{"impulse 13", 		"lift object"},
	{"invuse",			"use inv item"},
	{"impulse 44",		"drop inv item"},
	{"+showinfo",		"full inventory"},
	{"+showdm",			"info / frags"},
	{"toggle_dm",		"toggle frags"},
	{"+infoplaque",		"objectives"},
	{"invleft",			"inv move left"},
	{"invright",		"inv move right"},
	{"impulse 100",		"inv:torch"},
	{"impulse 101",		"inv:qrtz flask"},
	{"impulse 102",		"inv:mystic urn"},
	{"impulse 103",		"inv:krater"},
	{"impulse 104",		"inv:chaos devc"},
	{"impulse 105",		"inv:tome power"},
	{"impulse 106",		"inv:summon stn"},
	{"impulse 107",		"inv:invisiblty"},
	{"impulse 108",		"inv:glyph"},
	{"impulse 109",		"inv:boots"},
	{"impulse 110",		"inv:repulsion"},
	{"impulse 111",		"inv:bo peep"},
	{"impulse 112",		"inv:flight"},
	{"impulse 113",		"inv:force cube"},
	{"impulse 114",		"inv:icon defn"}
};

#define	NUMCOMMANDS	(sizeof(bindnames)/sizeof(bindnames[0]))

#define KEYS_SIZE 14

int		keys_cursor;
int		bind_grab;
int		keys_top = 0;

void M_Menu_Keys_f (void)
{
	key_dest = key_menu;
	m_state = m_keys;
	m_soundlevel = m_sound_menu2;
}


void M_FindKeysForCommand (char *command, int *twokeys)
{
	int		count;
	int		j;
	int		l, l2;
	char	*b;

	twokeys[0] = twokeys[1] = -1;
	l = strlen (command);
	count = 0;

	for (j = 0; j < 256; j++)
	{
		b = keybindings[j];

		if (!b)
			continue;

		if (!strncmp (b, command, l))
		{
			l2 = strlen (b);

			if (l == l2)
			{
				twokeys[count] = j;
				count++;

				if (count == 2)
					break;
			}
		}
	}
}

void M_UnbindCommand (char *command)
{
	int		j;
	int		l;
	char	*b;

	l = strlen (command);

	for (j = 0; j < 256; j++)
	{
		b = keybindings[j];

		if (!b)
			continue;

		if (!strncmp (b, command, l))
			Key_SetBinding (j, "");
	}
}


void M_Keys_Draw (void)
{
	int		i, l;
	int		keys[2];
	char	*name;
	int		x, y;

	ScrollTitle ("gfx/menu/title6.lmp");

	//	M_DrawTextBox (6,56, 35,16);

	//	p = Draw_CachePic("gfx/menu/hback.lmp");
	//	M_DrawTransPicCropped(8, 62, p);

	if (bind_grab)
		M_Print (12, 64, "Press a key or button for this action");
	else
		M_Print (18, 64, "Enter to change, backspace to clear");

	if (keys_top)
		M_DrawCharacter (6, 80, 128);

	if (keys_top + KEYS_SIZE < NUMCOMMANDS)
		M_DrawCharacter (6, 80 + ((KEYS_SIZE - 1) * 8), 129);

	// search for known bindings
	for (i = 0; i < KEYS_SIZE; i++)
	{
		y = 80 + 8 * i;

		M_Print (16, y, bindnames[i+keys_top][1]);

		l = strlen (bindnames[i+keys_top][0]);

		M_FindKeysForCommand (bindnames[i+keys_top][0], keys);

		if (keys[0] == -1)
		{
			M_Print (140, y, "???");
		}
		else
		{
			name = Key_KeynumToString (keys[0]);
			M_Print (140, y, name);
			x = strlen (name) * 8;

			if (keys[1] != -1)
			{
				M_Print (140 + x + 8, y, "or");
				M_Print (140 + x + 32, y, Key_KeynumToString (keys[1]));
			}
		}
	}

	if (bind_grab)
		M_DrawCharacter (130, 80 + (keys_cursor - keys_top) * 8, '=');
	else
		M_DrawCharacter (130, 80 + (keys_cursor - keys_top) * 8, 12 + ((int) (realtime * 4) & 1));
}


void M_Keys_Key (int k)
{
	char	cmd[80];
	int		keys[2];

	if (bind_grab)
	{
		// defining a key
		m_soundlevel = m_sound_menu1;

		if (k == K_ESCAPE)
		{
			bind_grab = false;
		}
		else if (k != '`')
		{
			sprintf (cmd, "bind \"%s\" \"%s\"\n", Key_KeynumToString (k), bindnames[keys_cursor][0]);
			Cbuf_InsertText (cmd);
		}

		bind_grab = false;
		return;
	}

	switch (k)
	{
	case K_ESCAPE:
		// save off the configs when exiting this menu
		Host_WriteConfiguration ("config.cfg");
		M_Menu_Options_f ();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		m_soundlevel = m_sound_menu1;
		keys_cursor--;

		if (keys_cursor < 0)
			keys_cursor = NUMCOMMANDS - 1;

		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		m_soundlevel = m_sound_menu1;
		keys_cursor++;

		if (keys_cursor >= NUMCOMMANDS)
			keys_cursor = 0;

		break;

	case K_ENTER:		// go into bind mode
		M_FindKeysForCommand (bindnames[keys_cursor][0], keys);
		m_soundlevel = m_sound_menu2;

		if (keys[1] != -1)
			M_UnbindCommand (bindnames[keys_cursor][0]);

		bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
		m_soundlevel = m_sound_menu2;
		M_UnbindCommand (bindnames[keys_cursor][0]);
		break;
	}

	if (keys_cursor < keys_top)
		keys_top = keys_cursor;
	else if (keys_cursor >= keys_top + KEYS_SIZE)
		keys_top = keys_cursor - KEYS_SIZE + 1;
}


