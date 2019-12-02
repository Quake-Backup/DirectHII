
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"

extern	float introTime;

m_state_t m_state = m_none;
m_soundlevel_t m_soundlevel = m_sound_none;

qboolean	m_recursiveDraw;

int			m_return_state;
qboolean	m_return_onerror;
char		m_return_reason[32];

static float TitlePercent = 0;
static float TitleTargetPercent = 1;
static float TitleOldPercent = 0;
static float TitleBeginTime = 0;

static float LogoPercent = 0;
static float LogoTargetPercent = 1;
static float LogoOldPercent = 0;
static float LogoBeginTime = 0;

extern qboolean introPlaying;

extern float introTime;


//=============================================================================
/* Support Routines */

/*
================
M_DrawCharacter

Draws one solid graphics character, centered, on line
================
*/
void M_DrawCharacter (int cx, int line, int num)
{
	Draw_Character (cx + ((vid.width2d - 320) >> 1), line, num);
}

void M_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, ((unsigned char) (*str)) + 256);
		str++;
		cx += 8;
	}
}

/*
================
M_DrawCharacter2

Draws one solid graphics character, centered H and V
================
*/
void M_DrawCharacter2 (int cx, int line, int num)
{
	Draw_Character (cx + ((vid.width2d - 320) >> 1), line + ((vid.height2d - 200) >> 1), num);
}

void M_Print2 (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter2 (cx, cy, ((unsigned char) (*str)) + 256);
		str++;
		cx += 8;
	}
}

void M_PrintWhite (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (unsigned char) *str);
		str++;
		cx += 8;
	}
}

void M_DrawTransPic (int x, int y, qpic_t *pic)
{
	Draw_TransPic (x + ((vid.width2d - 320) >> 1), y, pic);
}

void M_DrawTransPic2 (int x, int y, qpic_t *pic)
{
	Draw_TransPic (x + ((vid.width2d - 320) >> 1), y + ((vid.height2d - 200) >> 1), pic);
}

void M_DrawPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x + ((vid.width2d - 320) >> 1), y, pic);
}

void M_DrawTransPicCropped (int x, int y, qpic_t *pic)
{
	Draw_TransPicCropped (x + ((vid.width2d - 320) >> 1), y, pic);
}

void M_DrawTextBox (int x, int y, int width, int lines)
{
	qpic_t	*p, *tm, *bm;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");

	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}

	p = Draw_CachePic ("gfx/box_bl.lmp");
	M_DrawTransPic (cx, cy + 8, p);

	// draw middle
	cx += 8;
	tm = Draw_CachePic ("gfx/box_tm.lmp");
	bm = Draw_CachePic ("gfx/box_bm.lmp");

	while (width > 0)
	{
		cy = y;
		M_DrawTransPic (cx, cy, tm);
		p = Draw_CachePic ("gfx/box_mm.lmp");

		for (n = 0; n < lines; n++)
		{
			cy += 8;

			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");

			M_DrawTransPic (cx, cy, p);
		}

		M_DrawTransPic (cx, cy + 8, bm);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");

	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}

	p = Draw_CachePic ("gfx/box_br.lmp");
	M_DrawTransPic (cx, cy + 8, p);
}


void M_DrawTextBox2 (int x, int y, int width, int lines, qboolean bottom)
{
	qpic_t	*p, *tm, *bm;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");

	if (bottom)
		M_DrawTransPic (cx, cy, p);
	else
		M_DrawTransPic2 (cx, cy, p);

	p = Draw_CachePic ("gfx/box_ml.lmp");

	for (n = 0; n < lines; n++)
	{
		cy += 8;

		if (bottom)
			M_DrawTransPic (cx, cy, p);
		else
			M_DrawTransPic2 (cx, cy, p);
	}

	p = Draw_CachePic ("gfx/box_bl.lmp");

	if (bottom)
		M_DrawTransPic (cx, cy + 8, p);
	else
		M_DrawTransPic2 (cx, cy + 8, p);

	// draw middle
	cx += 8;
	tm = Draw_CachePic ("gfx/box_tm.lmp");
	bm = Draw_CachePic ("gfx/box_bm.lmp");

	while (width > 0)
	{
		cy = y;

		if (bottom)
			M_DrawTransPic (cx, cy, tm);
		else
			M_DrawTransPic2 (cx, cy, tm);

		p = Draw_CachePic ("gfx/box_mm.lmp");

		for (n = 0; n < lines; n++)
		{
			cy += 8;

			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");

			if (bottom)
				M_DrawTransPic (cx, cy, p);
			else
				M_DrawTransPic2 (cx, cy, p);
		}

		if (bottom)
			M_DrawTransPic (cx, cy + 8, bm);
		else
			M_DrawTransPic2 (cx, cy + 8, bm);

		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");

	if (bottom)
		M_DrawTransPic (cx, cy, p);
	else
		M_DrawTransPic2 (cx, cy, p);

	p = Draw_CachePic ("gfx/box_mr.lmp");

	for (n = 0; n < lines; n++)
	{
		cy += 8;

		if (bottom)
			M_DrawTransPic (cx, cy, p);
		else
			M_DrawTransPic2 (cx, cy, p);
	}

	p = Draw_CachePic ("gfx/box_br.lmp");

	if (bottom)
		M_DrawTransPic (cx, cy + 8, p);
	else
		M_DrawTransPic2 (cx, cy + 8, p);
}

//=============================================================================


/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f (void)
{
	m_soundlevel = m_sound_menu2;

	if (key_dest == key_menu)
	{
		if (m_state != m_main)
		{
			LogoTargetPercent = TitleTargetPercent = 1;
			LogoPercent = TitlePercent = 0;
			LogoOldPercent = TitleOldPercent = 0;
			LogoBeginTime = TitleBeginTime = realtime;

			M_Menu_Main_f ();
			return;
		}

		key_dest = key_game;
		m_state = m_none;
		return;
	}

	if (key_dest == key_console)
	{
		Con_ToggleConsole_f ();
	}
	else
	{
		LogoTargetPercent = TitleTargetPercent = 1;
		LogoPercent = TitlePercent = 0;
		LogoOldPercent = TitleOldPercent = 0;
		LogoBeginTime = TitleBeginTime = realtime;

		M_Menu_Main_f ();
	}
}

char BigCharWidth[27][27];

//#define BUILD_BIG_CHAR 1

void M_BuildBigCharWidth (void)
{
#ifdef BUILD_BIG_CHAR
	qpic_t	*p;
	int ypos, xpos;
	byte			*source;
	int biggestX, adjustment;
	char After[20], Before[20];
	int numA, numB;
	FILE *FH;
	char temp[MAX_OSPATH];

	p = Draw_CachePic ("gfx/menu/bigfont.lmp");

	for (numA = 0; numA < 27; numA++)
	{
		memset (After, 20, sizeof (After));
		source = p->data + ((numA % 8) * 20) + (numA / 8 * p->width * 20);
		biggestX = 0;

		for (ypos = 0; ypos < 19; ypos++)
		{
			for (xpos = 0; xpos < 19; xpos++, source++)
			{
				if (*source)
				{
					After[ypos] = xpos;

					if (xpos > biggestX) biggestX = xpos;
				}
			}

			source += (p->width - 19);
		}

		biggestX++;

		for (numB = 0; numB < 27; numB++)
		{
			memset (Before, 0, sizeof (Before));
			source = p->data + ((numB % 8) * 20) + (numB / 8 * p->width * 20);
			adjustment = 0;

			for (ypos = 0; ypos < 19; ypos++)
			{
				for (xpos = 0; xpos < 19; xpos++, source++)
				{
					if (!(*source))
					{
						Before[ypos]++;
					}
					else break;
				}

				source += (p->width - xpos);
			}


			while (1)
			{
				for (ypos = 0; ypos < 19; ypos++)
				{
					if (After[ypos] - Before[ypos] >= 15) break;

					Before[ypos]--;
				}

				if (ypos < 19) break;

				adjustment--;
			}

			BigCharWidth[numA][numB] = adjustment + biggestX;
		}
	}

	sprintf (temp, "%s\\gfx\\menu\\fontsize.lmp", com_gamedir);
	FH = fopen (temp, "wb");
	fwrite (BigCharWidth, 1, sizeof (BigCharWidth), FH);
	fclose (FH);
#else
	memcpy (BigCharWidth, FS_LoadFile (LOAD_HUNK, "gfx/menu/fontsize.lmp"), sizeof (BigCharWidth));
#endif
}


/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/

int M_DrawBigCharacter (int x, int y, int num, int numNext);

void M_DrawBigString (int x, int y, char *string)
{
	int c, length;

	x += ((vid.width2d - 320) >> 1);

	length = strlen (string);

	for (c = 0; c < length; c++)
	{
		x += M_DrawBigCharacter (x, y, string[c], string[c + 1]);
	}
}






void ScrollTitle (char *name)
{
	qpic_t	*p;
	static char *LastName = "";
	int finaly;
	static qboolean CanSwitch = true;

	if (TitlePercent < TitleTargetPercent)
	{
		TitlePercent = TitleOldPercent + (((TitleTargetPercent - TitleOldPercent) * 2.5) * (realtime - TitleBeginTime));

		if (TitlePercent > TitleTargetPercent)
		{
			TitlePercent = TitleTargetPercent;
		}
	}
	else if (TitlePercent > TitleTargetPercent)
	{
		TitlePercent = TitleOldPercent - (((TitleOldPercent - TitleTargetPercent) * 2.5) * (realtime - TitleBeginTime));

		if (TitlePercent <= TitleTargetPercent)
		{
			TitlePercent = TitleTargetPercent;
			CanSwitch = true;
		}
	}

	if (LogoPercent < LogoTargetPercent)
	{
		LogoPercent = LogoOldPercent + (((LogoTargetPercent - LogoOldPercent) * 2.5) * (realtime - LogoBeginTime));

		if (LogoPercent > LogoTargetPercent)
		{
			LogoPercent = LogoTargetPercent;
		}
	}

	if (strcmpi (LastName, name) != 0 && TitleTargetPercent != 0)
	{
		TitleTargetPercent = 0;
		TitleOldPercent = TitlePercent;
		TitleBeginTime = realtime;
	}

	if (CanSwitch)
	{
		LastName = name;
		CanSwitch = false;
		TitleTargetPercent = 1;
		TitleOldPercent = TitlePercent;
		TitleBeginTime = realtime;
	}

	p = Draw_CachePic (LastName);
	finaly = ((float) p->height * TitlePercent) - p->height;
	M_DrawTransPicCropped ((320 - p->width) / 2, finaly, p);

	switch (m_state)
	{
	case m_keys:
	case m_load:
	case m_save:
		// these states don't show the logo plaque
		break;

	default:
		p = Draw_CachePic ("gfx/menu/hplaque.lmp");
		finaly = ((float) p->height * LogoPercent) - p->height;
		M_DrawTransPicCropped (10, finaly, p);
		break;
	}
}


char	*plaquemessage = NULL;   // Pointer to current plaque message
char    *errormessage = NULL;


//=============================================================================
/* Menu Subsystem */


void M_Init (void)
{
	Cmd_AddCommand ("togglemenu", M_ToggleMenu_f);

	Cmd_AddCommand ("menu_main", M_Menu_Main_f);
	Cmd_AddCommand ("menu_singleplayer", M_Menu_SinglePlayer_f);
	Cmd_AddCommand ("menu_load", M_Menu_Load_f);
	Cmd_AddCommand ("menu_save", M_Menu_Save_f);
	Cmd_AddCommand ("menu_multiplayer", M_Menu_MultiPlayer_f);
	Cmd_AddCommand ("menu_setup", M_Menu_Setup_f);
	Cmd_AddCommand ("menu_options", M_Menu_Options_f);
	Cmd_AddCommand ("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand ("menu_video", M_Menu_Video_f);
	Cmd_AddCommand ("help", M_Menu_Help_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
	Cmd_AddCommand ("menu_class", M_Menu_Class2_f);

	M_BuildBigCharWidth ();
}


void M_Draw (void)
{
	if (m_state == m_none || key_dest != key_menu)
	{
		// reset the title and logo scroll stuff if we're not drawing the menu
		LogoTargetPercent = TitleTargetPercent = 1;
		LogoPercent = TitlePercent = 0;
		LogoOldPercent = TitleOldPercent = 0;
		LogoBeginTime = TitleBeginTime = realtime;
		return;
	}

	if (!m_recursiveDraw)
	{
		if (scr.ConCurrent)
			Draw_ConsoleBackground (vid.height2d);
		else Draw_FadeScreen ();
	}
	else m_recursiveDraw = false;

	switch (m_state)
	{
	case m_none:
		break;

	case m_main:
		M_Main_Draw ();
		break;

	case m_singleplayer:
		M_SinglePlayer_Draw ();
		break;

	case m_difficulty:
		M_Difficulty_Draw ();
		break;

	case m_class:
		M_Class_Draw ();
		break;

	case m_load:
	case m_mload:
		M_Load_Draw ();
		break;

	case m_save:
	case m_msave:
		M_Save_Draw ();
		break;

	case m_multiplayer:
		M_MultiPlayer_Draw ();
		break;

	case m_setup:
		M_Setup_Draw ();
		break;

	case m_net:
		M_Net_Draw ();
		break;

	case m_options:
		M_Options_Draw ();
		break;

	case m_keys:
		M_Keys_Draw ();
		break;

	case m_video:
		M_Video_Draw ();
		break;

	case m_help:
		M_Help_Draw ();
		break;

	case m_quit:
		M_Quit_Draw ();
		break;

	case m_serialconfig:
		M_SerialConfig_Draw ();
		break;

	case m_modemconfig:
		M_ModemConfig_Draw ();
		break;

	case m_lanconfig:
		M_LanConfig_Draw ();
		break;

	case m_gameoptions:
		M_GameOptions_Draw ();
		break;

	case m_search:
		M_Search_Draw ();
		break;

	case m_slist:
		M_ServerList_Draw ();
		break;
	}

	switch (m_soundlevel)
	{
	case m_sound_menu1: S_LocalSound ("raven/menu1.wav"); break;
	case m_sound_menu2: S_LocalSound ("raven/menu2.wav"); break;
	case m_sound_menu3: S_LocalSound ("raven/menu3.wav"); break;
	case m_sound_steve: S_LocalSound ("rj/steve.wav"); break;
	default: break;
	};

	m_soundlevel = m_sound_none;
}


void M_Keydown (int key)
{
	switch (m_state)
	{
	case m_none:
		return;

	case m_main:
		M_Main_Key (key);
		return;

	case m_singleplayer:
		M_SinglePlayer_Key (key);
		return;

	case m_difficulty:
		M_Difficulty_Key (key);
		return;

	case m_class:
		M_Class_Key (key);
		return;

	case m_load:
		M_Load_Key (key);
		return;

	case m_save:
		M_Save_Key (key);
		return;

	case m_mload:
		M_MLoad_Key (key);
		return;

	case m_msave:
		M_MSave_Key (key);
		return;

	case m_multiplayer:
		M_MultiPlayer_Key (key);
		return;

	case m_setup:
		M_Setup_Key (key);
		return;

	case m_net:
		M_Net_Key (key);
		return;

	case m_options:
		M_Options_Key (key);
		return;

	case m_keys:
		M_Keys_Key (key);
		return;

	case m_video:
		M_Video_Key (key);
		return;

	case m_help:
		M_Help_Key (key);
		return;

	case m_quit:
		M_Quit_Key (key);
		return;

	case m_serialconfig:
		M_SerialConfig_Key (key);
		return;

	case m_modemconfig:
		M_ModemConfig_Key (key);
		return;

	case m_lanconfig:
		M_LanConfig_Key (key);
		return;

	case m_gameoptions:
		M_GameOptions_Key (key);
		return;

	case m_search:
		M_Search_Key (key);
		break;

	case m_slist:
		M_ServerList_Key (key);
		return;
	}
}


void M_ConfigureNetSubsystem (void)
{
	// enable/disable net systems to match desired config
	extern int 	lanConfig_port;
	extern int	m_net_cursor;

	Cbuf_AddText ("stopdemo\n");

	if (SerialConfig || DirectConfig)
	{
		Cbuf_AddText ("com1 enable\n");
	}

	if (IPXConfig || TCPIPConfig)
		net_hostport = lanConfig_port;
}

