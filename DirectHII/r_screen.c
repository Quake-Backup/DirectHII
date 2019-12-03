// screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "d_video.h"
#include "d_draw.h"

void M_Print (int cx, int cy, char *str);
void M_Print2 (int cx, int cy, char *str);
void M_DrawTextBox2 (int x, int y, int width, int lines, qboolean bottom);
void M_PrintWhite (int cx, int cy, char *str);
void Sbar_DeathmatchOverlay (void);

/*

background clear
rendering
turtle/net/ram icons
sbar
centerprint / slow centerprint
notify lines
intermission / finale overlay
loading plaque
console
menu

required background clears
required update regions


syncronous draw mode or async
One off screen buffer, with updates either copied or xblited
Need to double buffer?


async draw will require the refresh area to be cleared, because it will be
xblited, but sync draw can just ignore it.

sync
draw

CenterPrint ()
SlowPrint ()
Screen_Update ();
Con_Printf ();

net
turn off messages option

the refresh is allways rendered, unless the console is full screen


console is:
notify lines
half
full


*/

// moving the globals into a cleaner struct
scrdef_t scr;

cvar_t		scr_viewsize = {"viewsize", "100", true};
cvar_t		scr_fov = {"fov", "90"};	// 10 - 170

#ifdef _DEBUG
cvar_t		scr_showfps = {"scr_showfps", "1"};
#else
cvar_t		scr_showfps = {"scr_showfps", "0"};
#endif

cvar_t		scr_conspeed = {"scr_conspeed", "300"};
cvar_t		scr_centertime = {"scr_centertime", "4"};
cvar_t		scr_showram = {"showram", "1"};
cvar_t		scr_showturtle = {"showturtle", "0"};
cvar_t		scr_showpause = {"showpause", "1"};
cvar_t		scr_printspeed = {"scr_printspeed", "8"};

extern	cvar_t	crosshair;
qboolean	scr_initialized;		// ready to draw

qpic_t		*scr_ram;
qpic_t		*scr_net;
qpic_t		*scr_turtle;

float		introTime = 0.0;

int			clearconsole;
int			clearnotify;

int			sb_lines;

qboolean	scr_disabled_for_loading;
qboolean	scr_drawloading;
float		scr_disabled_time;

static qboolean scr_needfull = false;

int	total_loading_size, current_loading_size, loading_stage;


void SCR_ScreenShot_f (void);
void Plaque_Draw (char *message, qboolean AlwaysDraw);
void Info_Plaque_Draw (char *message);
void Bottom_Plaque_Draw (char *message);

// replacement timing for anything dependent on host_frametime, which may be modified
// by host_framerate or host_timescale, and is therefore unreliable here.
double scr_frametime = 0;


/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;
int			scr_erase_lines;
int			scr_erase_center;

static int lines;
#define MAXLINES 27
static int StartC[MAXLINES], EndC[MAXLINES];

#define MAX_INFO 1024
char infomessage[MAX_INFO];
qboolean info_up = false;

extern int	*pr_info_string_index;
extern char	*pr_global_info_strings;
extern int	 pr_info_string_count;

void UpdateInfoMessage (void)
{
	unsigned int i;
	unsigned int check;
	char *newmessage;

	strcpy (infomessage, "Objectives:");

	if (!pr_global_info_strings)
		return;

	for (i = 0; i < 32; i++)
	{
		check = (1 << i);

		if (cl.info_mask & check)
		{
			newmessage = &pr_global_info_strings[pr_info_string_index[i]];
			strcat (infomessage, "@@");
			strcat (infomessage, newmessage);
		}
	}

	for (i = 0; i < 32; i++)
	{
		check = (1 << i);

		if (cl.info_mask2 & check)
		{
			newmessage = &pr_global_info_strings[pr_info_string_index[i + 32]];
			strcat (infomessage, "@@");
			strcat (infomessage, newmessage);
		}
	}
}

void FindTextBreaks (char *message, int Width)
{
	int length, pos, start, lastspace, oldlast;

	length = strlen (message);
	lines = pos = start = 0;
	lastspace = -1;

	while (1)
	{
		if (pos - start >= Width || message[pos] == '@' ||
			message[pos] == 0)
		{
			oldlast = lastspace;

			if (message[pos] == '@' || lastspace == -1 || message[pos] == 0)
				lastspace = pos;

			StartC[lines] = start;
			EndC[lines] = lastspace;
			lines++;

			if (lines == MAXLINES)
				return;

			if (message[pos] == '@')
				start = pos + 1;
			else if (oldlast == -1)
				start = lastspace;
			else
				start = lastspace + 1;

			lastspace = -1;
		}

		if (message[pos] == 32) lastspace = pos;
		else if (message[pos] == 0)
		{
			break;
		}

		pos++;
	}
}

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (char *str)
{
	strncpy (scr_centerstring, str, sizeof (scr_centerstring) - 1);
	scr_centertime_off = scr_centertime.value;
	scr_centertime_start = cl.time;

	FindTextBreaks (scr_centerstring, 38);
	scr_center_lines = lines;
}


void SCR_DrawCenterString (void)
{
	int		i;
	int		bx, by;
	int		remaining;
	char	temp[80];

	// the finale prints the characters one at a time
	if (cl.intermission)
		remaining = scr_printspeed.value * (cl.time - scr_centertime_start);
	else remaining = 9999;

	scr_erase_center = 0;

	FindTextBreaks (scr_centerstring, 38);

	by = (((25 - lines) * 8) / 2) - 40;

	for (i = 0; i < lines; i++, by += 8)
	{
		strncpy (temp, &scr_centerstring[StartC[i]], EndC[i] - StartC[i]);
		temp[EndC[i] - StartC[i]] = 0;
		bx = ((40 - strlen (temp)) * 8) / 2;
		M_Print2 (bx, by, temp);
	}
}

void SCR_CheckDrawCenterString (void)
{
	if (scr_center_lines > scr_erase_lines)
		scr_erase_lines = scr_center_lines;

	scr_centertime_off -= (cl.time - cl.oldtime);

	if (scr_centertime_off <= 0 && !cl.intermission)
		return;

	if (key_dest != key_game)
		return;

	if (intro_playing)
		Bottom_Plaque_Draw (scr_centerstring);
	else
		SCR_DrawCenterString ();
}

//=============================================================================

float SCR_CalcFovX (float fov_y, float width, float height)
{
	float   a;
	float   y;

	// bound, don't crash
	if (fov_y < 1) fov_y = 1;
	if (fov_y > 179) fov_y = 179;

	y = height / tan (fov_y / 360 * M_PI);
	a = atan (width / y);
	a = a * 360 / M_PI;

	return a;
}


float SCR_CalcFovY (float fov_x, float width, float height)
{
	float   a;
	float   x;

	// bound, don't crash
	if (fov_x < 1) fov_x = 1;
	if (fov_x > 179) fov_x = 179;

	x = width / tan (fov_x / 360 * M_PI);
	a = atan (height / x);
	a = a * 360 / M_PI;

	return a;
}


void SCR_SetFOV (refdef_t *rd, float fovvar, int width, int height)
{
	float aspect = (float) height / (float) width;

#define BASELINE_W	640.0f
#define BASELINE_H	432.0f

	// http://www.gamedev.net/topic/431111-perspective-math-calculating-horisontal-fov-from-vertical/
	// horizontalFov = atan (tan (verticalFov) * aspectratio)
	// verticalFov = atan (tan (horizontalFov) / aspectratio)
	if (aspect > (BASELINE_H / BASELINE_W))
	{
		// use the same calculation as GLQuake did (horizontal is constant, vertical varies)
		rd->fov_x = fovvar;
		rd->fov_y = SCR_CalcFovY (rd->fov_x, width, height);
	}
	else
	{
		// alternate calculation (vertical is constant, horizontal varies)
		// consistent with http://www.emsai.net/projects/widescreen/fovcalc/
		// note that the gun always uses this calculation irrespective of the aspect)
		rd->fov_y = SCR_CalcFovY (fovvar, BASELINE_W, BASELINE_H);
		rd->fov_x = SCR_CalcFovX (rd->fov_y, width, height);
	}
}


/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
void SCR_CalcRefdef (void)
{
	// bound viewsize
	if (scr_viewsize.value < 30) Cvar_Set ("viewsize", "30");
	if (scr_viewsize.value > 120) Cvar_Set ("viewsize", "120");

	// bound field of view
	if (scr_fov.value < 10) Cvar_Set ("fov", "10");
	if (scr_fov.value > 170) Cvar_Set ("fov", "170");

	if (scr_viewsize.value < 100)
	{
		// figure minimum sizes; assumes aspect is never < 4:3
		int minheight = 200;
		int minwidth = (vid.width * minheight) / vid.height;

		// figure scaling percentage
		int pct = ((scr_viewsize.value - 30) * 100) / 70;

		// and apply it
		vid.width2d = minwidth + ((vid.width - minwidth) * pct) / 100;
		vid.height2d = minheight + ((vid.height - minheight) * pct) / 100;
	}
	else
	{
		vid.width2d = vid.width;
		vid.height2d = vid.height;
	}

	// intermission is always full screen
	if (scr.timeRefresh)
		sb_lines2d = 0;
	else if (cl.intermission)
		sb_lines2d = 0;
	else if (scr_viewsize.value >= 110)
		sb_lines2d = 0;
	else sb_lines2d = 36;

	// scale sb_lines up for the new size
	sb_lines = (sb_lines2d * vid.height) / vid.height2d;

	// set FOV correctly
	SCR_SetFOV (&r_refdef, scr_fov.value, vid.width, vid.height - sb_lines);

	// set up a second refdef for the viewmodel; only fov is used and it special-cases fov for values > 90
	if (scr_fov.value < 90)
		SCR_SetFOV (&r_viewmodel_refdef, scr_fov.value, vid.width, vid.height - sb_lines);
	else SCR_SetFOV (&r_viewmodel_refdef, 90.0f, vid.width, vid.height - sb_lines);

	// deferred to after the refdef is set because we may be scaling up the 2d view
	D_UpdateDrawConstants (vid.width2d, vid.height2d, v_gamma.value, 1.0f);
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{
	Cvar_SetValue ("viewsize", scr_viewsize.value + 10);
	SB_ViewSizeChanged ();
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void)
{
	Cvar_SetValue ("viewsize", scr_viewsize.value - 10);
	SB_ViewSizeChanged ();
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	Cvar_RegisterVariable (&scr_showfps, NULL);
	Cvar_RegisterVariable (&scr_fov, NULL);
	Cvar_RegisterVariable (&scr_viewsize, NULL);
	Cvar_RegisterVariable (&scr_conspeed, NULL);
	Cvar_RegisterVariable (&scr_showram, NULL);
	Cvar_RegisterVariable (&scr_showturtle, NULL);
	Cvar_RegisterVariable (&scr_showpause, NULL);
	Cvar_RegisterVariable (&scr_centertime, NULL);
	Cvar_RegisterVariable (&scr_printspeed, NULL);

	// register our commands
	Cmd_AddCommand ("screenshot", SCR_ScreenShot_f);
	Cmd_AddCommand ("sizeup", SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown", SCR_SizeDown_f);

	scr_ram = Draw_PicFromWad ("ram");
	scr_net = Draw_PicFromWad ("net");
	scr_turtle = Draw_PicFromWad ("turtle");

	scr_initialized = true;
}



/*
==============
SCR_DrawRam
==============
*/
void SCR_DrawRam (void)
{
}

/*
==============
SCR_DrawTurtle
==============
*/
void SCR_DrawTurtle (void)
{
	static int	count;

	if (!scr_showturtle.value)
		return;

	if (scr_frametime < 0.1)
	{
		count = 0;
		return;
	}

	count++;

	if (count < 3)
		return;

	Draw_Pic (0, 0, scr_turtle);
}

/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (realtime - cl.last_received_message < 0.3)
		return;

	if (cls.demoplayback)
		return;

	Draw_Pic (64, 0, scr_net);
}

/*
==============
DrawPause
==============
*/
void SCR_DrawPause (void)
{
	qpic_t	*pic;
	static qboolean newdraw = false;
	int finaly;
	static float LogoPercent, LogoTargetPercent, LogoOldPercent, LogoBeginTime;

	if (!scr_showpause.value)		// turn off for screenshots
		return;

	if (!cl.paused)
	{
		newdraw = false;
		return;
	}

	if (!newdraw)
	{
		newdraw = true;
		LogoTargetPercent = 1;
		LogoOldPercent = 0;
		LogoBeginTime = realtime;
		LogoPercent = 0;
	}

	pic = Draw_CachePic ("gfx/menu/paused.lmp");

	if (LogoPercent < LogoTargetPercent)
	{
		LogoPercent = LogoOldPercent + (((LogoTargetPercent - LogoOldPercent) * 2.5) * (realtime - LogoBeginTime));

		if (LogoPercent > LogoTargetPercent)
		{
			LogoPercent = LogoTargetPercent;
		}
	}

	finaly = ((float) pic->height * LogoPercent) - pic->height;
	Draw_TransPicCropped ((vid.width2d - pic->width) / 2, finaly, pic);
}



/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
	int		size, count, offset;
	qpic_t	*pic;

	if (!scr_drawloading && loading_stage == 0)
		return;

	pic = Draw_CachePic ("gfx/menu/loading.lmp");
	offset = (vid.width2d - pic->width) / 2;
	Draw_TransPic (offset, 0, pic);

	if (loading_stage == 0)
		return;

	if (total_loading_size)
		size = current_loading_size * 106 / total_loading_size;
	else
		size = 0;

	if (loading_stage == 1)
		count = size;
	else
		count = 106;

	if (count)
	{
		Draw_Fill (offset + 42, 87, count, 1, 136);
		Draw_Fill (offset + 42, 87 + 1, count, 4, 138);
		Draw_Fill (offset + 42, 87 + 5, count, 1, 136);
	}

	if (loading_stage == 2)
		count = size;
	else
		count = 0;

	if (count)
	{
		Draw_Fill (offset + 42, 97, count, 1, 168);
		Draw_Fill (offset + 42, 97 + 1, count, 4, 170);
		Draw_Fill (offset + 42, 97 + 5, count, 1, 168);
	}
}



//=============================================================================


/*
==================
SCR_SetUpToDrawConsole
==================
*/
void SCR_SetUpToDrawConsole (void)
{
	static double con_oldtime = -1;
	double con_frametime;

	// check for first call
	if (con_oldtime < 0) con_oldtime = realtime;

	// get correct frametime
	con_frametime = realtime - con_oldtime;
	con_oldtime = realtime;

	Con_CheckResize ();

	if (scr_drawloading)
		return;		// never a console with loading plaque

	// decide on the height of the console
	con_forcedup = !cl.worldmodel || cls.signon != SIGNONS;

	if (con_forcedup)
	{
		scr.ConLines = 1.0f;		// full screen
		scr.ConCurrent = scr.ConLines;
	}
	else if (key_dest == key_console)
		scr.ConLines = 0.5f;	// half screen
	else scr.ConLines = 0;		// none visible

	// scr_conspeed is calibrated for a window height of 200 so scale it down for the 0..1 range
	if (scr.ConLines < scr.ConCurrent)
	{
		scr.ConCurrent -= scr_conspeed.value * con_frametime * (1.0f / 200.0f);
		if (scr.ConLines > scr.ConCurrent)
			scr.ConCurrent = scr.ConLines;

	}
	else if (scr.ConLines > scr.ConCurrent)
	{
		scr.ConCurrent += scr_conspeed.value * con_frametime * (1.0f / 200.0f);
		if (scr.ConLines < scr.ConCurrent)
			scr.ConCurrent = scr.ConLines;
	}
}


/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	if (scr.ConCurrent)
	{
		Con_DrawConsole ((int) (vid.height2d * scr.ConCurrent), true);
		clearconsole = 0;
	}
	else
	{
		if (key_dest == key_game || key_dest == key_message)
			Con_DrawNotify ();	// only draw notify in game
	}
}


/*
==============================================================================

SCREEN SHOTS

==============================================================================
*/

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
==================
SCR_ScreenShot_f
==================
*/
void SCR_ScreenShot_f (void)
{
	int i;
	char checkname[MAX_OSPATH];

	sprintf (checkname, "%s/shots", com_gamedir);
	Sys_mkdir (checkname);

	// find a file name to save it to
	for (i = 0; i <= 99; i++)
	{
		sprintf (checkname, "%s/shots/hexen%02i.tga", com_gamedir, i);

		if (Sys_FileTime (checkname) == -1)
			break;	// file doesn't exist
	}

	if (i == 100)
	{
		Con_Printf (PRINT_NORMAL, "SCR_ScreenShot_f: Couldn't create a TGA file\n");
		return;
	}

	D_CaptureScreenshot (checkname, v_gamma.value);
}


//=============================================================================


/*
===============
SCR_BeginLoadingPlaque

================
*/
void SCR_BeginLoadingPlaque (void)
{
	S_StopAllSounds (true);

	if (cls.state != ca_connected) return;
	if (cls.signon != SIGNONS) return;

	// redraw with no console and the loading plaque
	Con_ClearNotify ();
	scr_centertime_off = 0;
	scr.ConCurrent = 0;

	// huh???
	scr_drawloading = true;
	// SB_Changed ();
	scr_drawloading = false;

	scr_disabled_for_loading = true;
	scr_disabled_time = realtime;
}

/*
===============
SCR_EndLoadingPlaque

================
*/
void SCR_EndLoadingPlaque (void)
{
	scr_disabled_for_loading = false;
	Con_ClearNotify ();
}

//=============================================================================

char	*scr_notifystring;
qboolean	scr_drawdialog;

void SCR_DrawNotifyString (void)
{
	Plaque_Draw (scr_notifystring, 1);
}

/*
==================
SCR_ModalMessage

Displays a text string in the center of the screen and waits for a Y or N
keypress.
==================
*/
int SCR_ModalMessage (char *text)
{
	if (cls.state == ca_dedicated)
		return true;

	scr_notifystring = text;

	// draw a fresh screen
	scr_drawdialog = true;
	SCR_UpdateScreen ();
	scr_drawdialog = false;

	S_ClearBuffer ();		// so dma doesn't loop current sound

	do
	{
		key_count = -1;		// wait for a key down and up
		Sys_SendKeyEvents ();
	} while (key_lastpress != 'y' && key_lastpress != 'n' && key_lastpress != K_ESCAPE);

	SCR_UpdateScreen ();

	return key_lastpress == 'y';
}


//=============================================================================


// This is also located in screen.c
#define PLAQUE_WIDTH 26

void Plaque_Draw (char *message, qboolean AlwaysDraw)
{
	int i;
	char temp[80];
	int bx, by;

	if (scr.ConCurrent >= 1.0f && !AlwaysDraw)
		return;		// console is full screen

	if (!*message)
		return;

	FindTextBreaks (message, PLAQUE_WIDTH);

	by = ((25 - lines) * 8) / 2;
	M_DrawTextBox2 (32, by - 16, 30, lines + 2, false);

	for (i = 0; i < lines; i++, by += 8)
	{
		strncpy (temp, &message[StartC[i]], EndC[i] - StartC[i]);
		temp[EndC[i] - StartC[i]] = 0;
		bx = ((40 - strlen (temp)) * 8) / 2;
		M_Print2 (bx, by, temp);
	}
}

void Info_Plaque_Draw (char *message)
{
	int i;
	char temp[80];
	int bx, by;

	if (scr.ConCurrent >= 1.0f)
		return;		// console is full screen

	if (!*message)
		return;

	scr_needfull = true;

	FindTextBreaks (message, PLAQUE_WIDTH + 4);

	if (lines == MAXLINES)
	{
		Con_Printf (PRINT_DEVELOPER, "Info_Plaque_Draw: line overflow error\n");
		lines = MAXLINES - 1;
	}

	by = ((25 - lines) * 8) / 2;
	M_DrawTextBox2 (15, by - 16, PLAQUE_WIDTH + 4 + 4, lines + 2, false);

	for (i = 0; i < lines; i++, by += 8)
	{
		strncpy (temp, &message[StartC[i]], EndC[i] - StartC[i]);
		temp[EndC[i] - StartC[i]] = 0;
		bx = ((40 - strlen (temp)) * 8) / 2;
		M_Print2 (bx, by, temp);
	}
}
void I_DrawCharacter (int cx, int line, int num)
{
	Draw_Character (cx + ((vid.width2d - 320) >> 1), line + ((vid.height2d - 200) >> 1), num);
}

void I_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		I_DrawCharacter (cx, cy, ((unsigned char) (*str)) + 256);
		str++;
		cx += 8;
	}
}

void Bottom_Plaque_Draw (char *message)
{
	int i;
	char temp[80];
	int bx, by;

	if (!*message)
		return;

	scr_needfull = true;

	FindTextBreaks (message, PLAQUE_WIDTH);

	by = (((vid.height2d) / 8) - lines - 2) * 8;

	M_DrawTextBox2 (32, by - 16, 30, lines + 2, true);

	for (i = 0; i < lines; i++, by += 8)
	{
		strncpy (temp, &message[StartC[i]], EndC[i] - StartC[i]);
		temp[EndC[i] - StartC[i]] = 0;
		bx = ((40 - strlen (temp)) * 8) / 2;
		M_Print (bx, by, temp);
	}
}
//==========================================================================
//
// SB_IntermissionOverlay
//
//==========================================================================

void SB_IntermissionOverlay (void)
{
	qpic_t	*pic;
	int		elapsed, size, bx, by, i;
	char	*message, temp[80];

	if (cl.gametype == GAME_DEATHMATCH)
	{
		Sbar_DeathmatchOverlay ();
		return;
	}

	switch (cl.intermission)
	{
	case 1:
		pic = Draw_CachePic ("gfx/meso.lmp");
		break;
	case 2:
		pic = Draw_CachePic ("gfx/egypt.lmp");
		break;
	case 3:
		pic = Draw_CachePic ("gfx/roman.lmp");
		break;
	case 4:
		pic = Draw_CachePic ("gfx/castle.lmp");
		break;
	case 5:
		pic = Draw_CachePic ("gfx/castle.lmp");
		break;
	case 6:
		pic = Draw_CachePic ("gfx/end-1.lmp");
		break;
	case 7:
		pic = Draw_CachePic ("gfx/end-2.lmp");
		break;
	case 8:
		pic = Draw_CachePic ("gfx/end-3.lmp");
		break;
	case 9:
		pic = Draw_CachePic ("gfx/castle.lmp");
		break;
	case 10:
		pic = Draw_CachePic ("gfx/mpend.lmp");
		break;
	case 11:
		pic = Draw_CachePic ("gfx/mpmid.lmp");
		break;
	case 12:
		pic = Draw_CachePic ("gfx/end-3.lmp");
		break;

	default:
		Sys_Error ("SB_IntermissionOverlay: Bad episode");
		return;
	}

	Draw_Pic (((vid.width2d - 320) >> 1), ((vid.height2d - 200) >> 1), pic);

	if (cl.intermission >= 6 && cl.intermission <= 8)
	{
		elapsed = (cl.time - cl.completed_time) * 20;
		elapsed -= 50;

		if (elapsed < 0) elapsed = 0;
	}
	else if (cl.intermission == 12)
	{
		elapsed = (introTime);

		if (introTime < 500)
			introTime += 0.25;
	}
	else
	{
		elapsed = (cl.time - cl.completed_time) * 20;
	}

	if (cl.intermission <= 4 && cl.intermission + 394 <= pr_string_count)
		message = &pr_global_strings[pr_string_index[cl.intermission + 394]];
	else if (cl.intermission == 5)
		message = &pr_global_strings[pr_string_index[ABILITIES_STR_INDEX + NUM_CLASSES * 2 + 1]];
	else if (cl.intermission >= 6 && cl.intermission <= 8 && cl.intermission + 386 <= pr_string_count)
		message = &pr_global_strings[pr_string_index[cl.intermission + 386]];
	else if (cl.intermission == 9)
		message = &pr_global_strings[pr_string_index[391]];
	else
		message = "";

	if (cl.intermission == 10)
		message = &pr_global_strings[pr_string_index[538]];
	else if (cl.intermission == 11)
		message = &pr_global_strings[pr_string_index[545]];
	else if (cl.intermission == 12)
		message = &pr_global_strings[pr_string_index[561]];

	FindTextBreaks (message, 38);

	if (cl.intermission == 8)
		by = 16;
	else
		by = ((25 - lines) * 8) / 2;

	for (i = 0; i < lines; i++, by += 8)
	{
		size = EndC[i] - StartC[i];
		strncpy (temp, &message[StartC[i]], size);

		if (size > elapsed) size = elapsed;

		temp[size] = 0;

		bx = ((40 - strlen (temp)) * 8) / 2;

		if (cl.intermission < 6 || cl.intermission > 9)
			I_Print (bx, by, temp);
		else
			M_PrintWhite (bx, by, temp);

		elapsed -= size;

		if (elapsed <= 0) break;
	}

	if (i == lines && elapsed >= 300 && cl.intermission >= 6 && cl.intermission <= 7)
	{
		cl.intermission++;
		cl.completed_time = cl.time;
	}

	//	Con_Printf("Time is %10.2f\n",elapsed);
}

//==========================================================================
//
// SB_FinaleOverlay
//
//==========================================================================

void SB_FinaleOverlay (void)
{
	qpic_t	*pic;

	pic = Draw_CachePic ("gfx/finale.lmp");
	Draw_TransPic ((vid.width2d - pic->width) / 2, 16, pic);
}

void SCR_ShowFPS (void)
{
	char str[32] = {0};

	static int frames = 0;
	static double starttime = 0;
	static qboolean first = true;
	static float fps = 0.0f;

	if (first || cls.state != ca_connected)
	{
		frames = 0;
		starttime = realtime;
		first = false;
		return;
	}

	frames++;

	if (realtime - starttime > 0.25)
	{
		fps = (double) frames / (realtime - starttime);
		starttime = realtime;
		frames = 0;
	}

	if (scr_showfps.value)
	{
		sprintf (str, "%0.2f fps", fps);
		Draw_String (vid.width2d - (strlen (str) + 1) * 8, 8, str);
	}
}


/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.

WARNING: be very careful calling this from elsewhere, because the refresh
needs almost the entire 256k of stack space!
==================
*/
void SCR_UpdateScreen (void)
{
	if (scr_disabled_for_loading)
	{
		if (realtime - scr_disabled_time > 60)
		{
			scr_disabled_for_loading = false;
			total_loading_size = 0;
			loading_stage = 0;
			Con_Printf (PRINT_NORMAL, "load failed.\n");
		}
		else
			return;
	}

	if (!scr_initialized || !con_initialized)
		return;				// not initialized yet

	if (VID_BeginRendering ())
	{
		// determine size of refresh window
		SCR_CalcRefdef ();

		// do 3D refresh drawing, and then update the screen
		SCR_SetUpToDrawConsole ();

		if (cl.intermission > 1 || cl.intermission <= 12)
			V_RenderView ();

		// switch to the alternate draw RT
		D_DrawSet2D (vid.width2d, vid.height2d);

		if (scr_drawdialog)
		{
			SB_Draw ();
			Draw_FadeScreen ();
			SCR_DrawNotifyString ();
		}
		else if (scr_drawloading)
		{
			SB_Draw ();
			Draw_FadeScreen ();
			SCR_DrawLoading ();
		}
		else if (cl.intermission >= 1 && cl.intermission <= 12)
		{
			SB_IntermissionOverlay ();

			if (cl.intermission < 12)
			{
				SCR_DrawConsole ();
				M_Draw ();
			}
		}
		else
		{
			SCR_DrawRam ();
			SCR_DrawNet ();
			SCR_DrawTurtle ();
			SCR_DrawPause ();
			SCR_CheckDrawCenterString ();
			SB_Draw ();
			Plaque_Draw (plaquemessage, 0);
			SCR_ShowFPS ();
			SCR_DrawConsole ();
			M_Draw ();

			if (errormessage)
				Plaque_Draw (errormessage, 1);
		}

		if (loading_stage)
			SCR_DrawLoading ();

		V_UpdatePalette ();

		// splat the draw RT to the main scene
		D_DrawEnd2D (vid.width, vid.height, (float) vid.width2d / (float) vid.width, (float) vid.height2d / (float) vid.height);

		VID_EndRendering ();
	}
}


void SCR_ShowLoadingSize (void)
{
	static int last_loading_size = -1;
	extern qboolean	vid_initialized;

	if (!vid_initialized)
		return;

	// only update if the loading size has actually changed
	if (last_loading_size != current_loading_size)
	{
		SCR_UpdateScreen ();
		last_loading_size = current_loading_size;
	}
}


void SCR_DrawDisc (qpic_t *disc)
{
	// because you can't draw to the front buffer in D3D we instead do a mini-refresh of just the disc pic
	if (!scr_initialized || !con_initialized)
		return;				// not initialized yet

	if (VID_BeginRendering ())
	{
		// determine size of refresh window
		SCR_CalcRefdef ();

		// do 3D refresh drawing, and then update the screen
		SCR_SetUpToDrawConsole ();

		// switch to the alternate draw RT
		D_DrawSet2D (vid.width2d, vid.height2d);

		Draw_Pic (vid.width2d - 28, 0, disc);

		V_UpdatePalette ();

		// splat the draw RT to the main scene
		D_DrawEnd2D (vid.width, vid.height, (float) vid.width2d / (float) vid.width, (float) vid.height2d / (float) vid.height);

		VID_EndRendering ();
	}
}

