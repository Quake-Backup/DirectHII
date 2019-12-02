
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"


//=============================================================================
/* SETUP MENU */

void M_DrawTransPicTranslate (int x, int y, qpic_t *pic, int classnum, int color)
{
	Draw_TransPicTranslate (x + ((vid.width2d - 320) >> 1), y, pic, classnum, color);
}

int		setup_cursor = 5;
int		setup_cursor_table[] = {40, 56, 80, 104, 128, 156};

char	setup_hostname[16];
char	setup_myname[16];
int		setup_oldtop;
int		setup_oldbottom;
int		setup_top;
int		setup_bottom;

#define	NUM_SETUP_CMDS	6

void M_Menu_Setup_f (void)
{
	key_dest = key_menu;
	m_state = m_setup;
	m_soundlevel = m_sound_menu2;
	strcpy (setup_myname, cl_name.string);
	strcpy (setup_hostname, hostname.string);
	setup_top = setup_oldtop = ((int) cl_color.value) >> 4;
	setup_bottom = setup_oldbottom = ((int) cl_color.value) & 15;
	setup_class = cl_playerclass.value;

	if (setup_class < 1 || setup_class > NUM_CLASSES)
		setup_class = NUM_CLASSES;
}


void M_Setup_Draw (void)
{
	qpic_t	*p;

	ScrollTitle ("gfx/menu/title4.lmp");

	M_Print (64, 40, "Hostname");
	M_DrawTextBox (160, 32, 16, 1);
	M_Print (168, 40, setup_hostname);

	M_Print (64, 56, "Your name");
	M_DrawTextBox (160, 48, 16, 1);
	M_Print (168, 56, setup_myname);

	M_Print (64, 80, "Current Class: ");
	M_Print (88, 88, ClassNames[setup_class - 1]);

	M_Print (64, 104, "First color patch");
	M_Print (64, 128, "Second color patch");

	M_DrawTextBox (64, 156 - 8, 14, 1);
	M_Print (72, 156, "Accept Changes");

	p = Draw_CachePic (va ("gfx/menu/netp%i.lmp", setup_class));

	M_DrawTransPicTranslate (220, 72, p, setup_class - 1, (setup_top << 4) | setup_bottom);

	M_DrawCharacter (56, setup_cursor_table [setup_cursor], 12 + ((int) (realtime * 4) & 1));

	if (setup_cursor == 0)
		M_DrawCharacter (168 + 8 * strlen (setup_hostname), setup_cursor_table [setup_cursor], 10 + ((int) (realtime * 4) & 1));

	if (setup_cursor == 1)
		M_DrawCharacter (168 + 8 * strlen (setup_myname), setup_cursor_table [setup_cursor], 10 + ((int) (realtime * 4) & 1));
}


void M_Setup_Key (int k)
{
	int l;

	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_UPARROW:
		m_soundlevel = m_sound_menu1;
		setup_cursor--;

		if (setup_cursor < 0)
			setup_cursor = NUM_SETUP_CMDS - 1;

		break;

	case K_DOWNARROW:
		m_soundlevel = m_sound_menu1;
		setup_cursor++;

		if (setup_cursor >= NUM_SETUP_CMDS)
			setup_cursor = 0;

		break;

	case K_LEFTARROW:
		if (setup_cursor < 2)
			return;

		m_soundlevel = m_sound_menu3;

		if (setup_cursor == 2) setup_class--;
		if (setup_cursor == 3) setup_top--;
		if (setup_cursor == 4) setup_bottom--;

		break;

	case K_RIGHTARROW:
		if (setup_cursor < 2)
			return;

		m_soundlevel = m_sound_menu3;

		if (setup_cursor == 2) setup_class++;
		if (setup_cursor == 3) setup_top++;
		if (setup_cursor == 4) setup_bottom++;

		break;

	case K_ENTER:
		if (setup_cursor == 0 || setup_cursor == 1)
			return;

		if (strcmp (cl_name.string, setup_myname) != 0)
			Cbuf_AddText (va ("name \"%s\"\n", setup_myname));

		if (strcmp (hostname.string, setup_hostname) != 0)
			Cvar_Set ("hostname", setup_hostname);

		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
			Cbuf_AddText (va ("color %i %i\n", setup_top, setup_bottom));

		Cbuf_AddText (va ("playerclass %d\n", setup_class));
		m_soundlevel = m_sound_menu2;
		M_Menu_MultiPlayer_f ();
		break;

	case K_BACKSPACE:
		if (setup_cursor == 0)
		{
			if (strlen (setup_hostname))
				setup_hostname[strlen (setup_hostname) - 1] = 0;
		}

		if (setup_cursor == 1)
		{
			if (strlen (setup_myname))
				setup_myname[strlen (setup_myname) - 1] = 0;
		}

		break;

	default:
		if (k < 32 || k > 127)
			break;

		if (setup_cursor == 0)
		{
			l = strlen (setup_hostname);

			if (l < 15)
			{
				setup_hostname[l + 1] = 0;
				setup_hostname[l] = k;
			}
		}

		if (setup_cursor == 1)
		{
			l = strlen (setup_myname);

			if (l < 15)
			{
				setup_myname[l + 1] = 0;
				setup_myname[l] = k;
			}
		}
	}

	// bound our stuff
	if (setup_class < 1) setup_class = NUM_CLASSES;
	if (setup_class > NUM_CLASSES) setup_class = 1;
	if (setup_top > 10) setup_top = 0;
	if (setup_top < 0) setup_top = 10;
	if (setup_bottom > 10) setup_bottom = 0;
	if (setup_bottom < 0) setup_bottom = 10;
}

