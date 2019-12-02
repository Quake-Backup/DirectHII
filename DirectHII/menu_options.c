
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"

extern	cvar_t	crosshair;
extern	cvar_t	freelook;


//=============================================================================
/* OPTIONS MENU */

#define	SLIDER_RANGE	10

enum {
	OPT_CUSTOMIZE = 0,
	OPT_CONSOLE,
	OPT_DEFAULTS,
	OPT_SCRSIZE,	//3
	OPT_GAMMA,		//4
	OPT_MOUSESPEED,	//5
	OPT_MUSICTYPE,	//6
	OPT_MUSICVOL,	//7
	OPT_SNDVOL,		//8
	OPT_ALWAYRUN,	//9
	OPT_INVMOUSE,	//10
	OPT_LOOKSPRING,	//11
	OPT_LOOKSTRAFE,	//12
	OPT_CROSSHAIR,	//13
	OPT_ALWAYSMLOOK,//14
	OPT_VIDEO,		//15
	OPTIONS_ITEMS
};

int		options_cursor;

void M_Menu_Options_f (void)
{
	key_dest = key_menu;
	m_state = m_options;
	m_soundlevel = m_sound_menu2;
}


void M_AdjustSliders (int dir)
{
	m_soundlevel = m_sound_menu3;

	switch (options_cursor)
	{
	case OPT_SCRSIZE:	// screen size
		scr_viewsize.value += dir * 10;

		if (scr_viewsize.value < 30)
			scr_viewsize.value = 30;

		if (scr_viewsize.value > 120)
			scr_viewsize.value = 120;

		Cvar_SetValue ("viewsize", scr_viewsize.value);
		SB_ViewSizeChanged ();
		break;

	case OPT_GAMMA:	// gamma
		v_gamma.value -= dir * 0.05;

		if (v_gamma.value < 0.5)
			v_gamma.value = 0.5;

		if (v_gamma.value > 1)
			v_gamma.value = 1;

		Cvar_SetValue ("gamma", v_gamma.value);
		break;

	case OPT_MOUSESPEED:	// mouse speed
		sensitivity.value += dir * 0.5;

		if (sensitivity.value < 1)
			sensitivity.value = 1;

		if (sensitivity.value > 11)
			sensitivity.value = 11;

		Cvar_SetValue ("sensitivity", sensitivity.value);
		break;
	case OPT_MUSICTYPE: // bgm type
		if (strcmpi (bgmtype.string, "midi") == 0)
		{
			if (dir < 0)
				Cvar_Set ("bgmtype", "none");
			else
				Cvar_Set ("bgmtype", "cd");
		}
		else if (strcmpi (bgmtype.string, "cd") == 0)
		{
			if (dir < 0)
				Cvar_Set ("bgmtype", "midi");
			else
				Cvar_Set ("bgmtype", "none");
		}
		else
		{
			if (dir < 0)
				Cvar_Set ("bgmtype", "cd");
			else
				Cvar_Set ("bgmtype", "midi");
		}

		break;

	case OPT_MUSICVOL:	// music volume
		bgmvolume.value += dir * 0.1;

		if (bgmvolume.value < 0)
			bgmvolume.value = 0;

		if (bgmvolume.value > 1)
			bgmvolume.value = 1;

		Cvar_SetValue ("bgmvolume", bgmvolume.value);
		break;
	case OPT_SNDVOL:	// sfx volume
		volume.value += dir * 0.1;

		if (volume.value < 0)
			volume.value = 0;

		if (volume.value > 1)
			volume.value = 1;

		Cvar_SetValue ("volume", volume.value);
		break;

	case OPT_ALWAYRUN:	// allways run
		if (cl_forwardspeed.value > 200)
		{
			Cvar_SetValue ("cl_forwardspeed", 200);
			Cvar_SetValue ("cl_backspeed", 200);
		}
		else
		{
			Cvar_SetValue ("cl_forwardspeed", 400);
			Cvar_SetValue ("cl_backspeed", 400);
		}

		break;

	case OPT_INVMOUSE:	// invert mouse
		Cvar_SetValue ("m_pitch", -m_pitch.value);
		break;

	case OPT_LOOKSPRING:	// lookspring
		Cvar_SetValue ("lookspring", !lookspring.value);
		break;

	case OPT_LOOKSTRAFE:	// lookstrafe
		Cvar_SetValue ("lookstrafe", !lookstrafe.value);
		break;

	case OPT_CROSSHAIR:
		Cvar_SetValue ("crosshair", !crosshair.value);
		break;

	case OPT_ALWAYSMLOOK:
		Cvar_SetValue ("freelook", !freelook.value);
		break;
	}
}


void M_DrawSlider (int x, int y, float range)
{
	int	i;

	if (range < 0)
		range = 0;

	if (range > 1)
		range = 1;

	M_DrawCharacter (x - 8, y, 256);

	for (i = 0; i < SLIDER_RANGE; i++)
		M_DrawCharacter (x + i * 8, y, 257);

	M_DrawCharacter (x + i * 8, y, 258);
	M_DrawCharacter (x + (SLIDER_RANGE - 1) * 8 * range, y, 259);
}

void M_DrawCheckbox (int x, int y, int on)
{
	if (on)
		M_Print (x, y, "on");
	else
		M_Print (x, y, "off");
}

void M_Options_Draw (void)
{
	float		r;

	ScrollTitle ("gfx/menu/title3.lmp");

	M_Print (16, 60 + (0 * 8), "    Customize controls");
	M_Print (16, 60 + (1 * 8), "         Go to console");
	M_Print (16, 60 + (2 * 8), "     Reset to defaults");

	M_Print (16, 60 + (3 * 8), "           Screen size");
	r = (scr_viewsize.value - 30) / (120 - 30);
	M_DrawSlider (220, 60 + (3 * 8), r);

	M_Print (16, 60 + (4 * 8), "            Brightness");
	r = (1.0 - v_gamma.value) / 0.5;
	M_DrawSlider (220, 60 + (4 * 8), r);

	M_Print (16, 60 + (5 * 8), "           Mouse Speed");
	r = (sensitivity.value - 1) / 10;
	M_DrawSlider (220, 60 + (5 * 8), r);

	M_Print (16, 60 + (6 * 8), "            Music Type");

	if (strcmpi (bgmtype.string, "midi") == 0)
		M_Print (220, 60 + (6 * 8), "MIDI");
	else if (strcmpi (bgmtype.string, "cd") == 0)
		M_Print (220, 60 + (6 * 8), "CD");
	else
		M_Print (220, 60 + (6 * 8), "None");

	M_Print (16, 60 + (7 * 8), "          Music Volume");
	r = bgmvolume.value;
	M_DrawSlider (220, 60 + (7 * 8), r);

	M_Print (16, 60 + (8 * 8), "          Sound Volume");
	r = volume.value;
	M_DrawSlider (220, 60 + (8 * 8), r);

	M_Print (16, 60 + (9 * 8), "            Always Run");
	M_DrawCheckbox (220, 60 + (9 * 8), cl_forwardspeed.value > 200);

	M_Print (16, 60 + (OPT_INVMOUSE * 8), "          Invert Mouse");
	M_DrawCheckbox (220, 60 + (OPT_INVMOUSE * 8), m_pitch.value < 0);

	M_Print (16, 60 + (OPT_LOOKSPRING * 8), "            Lookspring");
	M_DrawCheckbox (220, 60 + (OPT_LOOKSPRING * 8), lookspring.value);

	M_Print (16, 60 + (OPT_LOOKSTRAFE * 8), "            Lookstrafe");
	M_DrawCheckbox (220, 60 + (OPT_LOOKSTRAFE * 8), lookstrafe.value);

	M_Print (16, 60 + (OPT_CROSSHAIR * 8), "        Show Crosshair");
	M_DrawCheckbox (220, 60 + (OPT_CROSSHAIR * 8), crosshair.value);

	M_Print (16, 60 + (OPT_ALWAYSMLOOK * 8), "            Mouse Look");
	M_DrawCheckbox (220, 60 + (OPT_ALWAYSMLOOK * 8), freelook.value);

	M_Print (16, 60 + (OPT_VIDEO * 8), "         Video Options");

	// cursor
	M_DrawCharacter (200, 60 + options_cursor * 8, 12 + ((int) (realtime * 4) & 1));
}


void M_Options_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		// save off the configs when exiting this menu
		Host_WriteConfiguration ("config.cfg");
		M_Menu_Main_f ();
		break;

	case K_ENTER:
		m_soundlevel = m_sound_menu2;

		switch (options_cursor)
		{
		case OPT_CUSTOMIZE:
			M_Menu_Keys_f ();
			break;
		case OPT_CONSOLE:
			m_state = m_none;
			Con_ToggleConsole_f ();
			break;
		case OPT_DEFAULTS:
			Cbuf_AddText ("exec default.cfg\n");
			break;
		case OPT_VIDEO:
			M_Menu_Video_f ();
			break;
		default:
			M_AdjustSliders (1);
			break;
		}

		return;

	case K_UPARROW:
		m_soundlevel = m_sound_menu1;
		options_cursor--;

		if (options_cursor < 0)
			options_cursor = OPTIONS_ITEMS - 1;

		break;

	case K_DOWNARROW:
		m_soundlevel = m_sound_menu1;
		options_cursor++;

		if (options_cursor >= OPTIONS_ITEMS)
			options_cursor = 0;

		break;

	case K_LEFTARROW:
		M_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_AdjustSliders (1);
		break;
	}
}


