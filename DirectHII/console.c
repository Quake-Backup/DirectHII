// console.c

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

int 		con_linewidth;

float		con_cursorspeed = 4;

#define		CON_TEXTSIZE	16384

qboolean 	con_forcedup;		// because no entities to refresh

int			con_totallines;		// total lines in console scrollback
int			con_backscroll;		// lines up from bottom to display
int			con_current;		// where next message will be printed
int			con_x;				// offset in current line for next print
static short		*con_text = 0;

cvar_t		con_notifytime = {"con_notifytime", "3"};		//seconds

#define	NUM_CON_TIMES 4
float		con_times[NUM_CON_TIMES];	// realtime time the line was generated
// for transparent notify lines

int			con_vislines;

qboolean	con_debuglog;

#define		MAXCMDLINE	256
extern	char	key_lines[32][MAXCMDLINE];
extern	int		edit_line;
extern	int		key_linepos;


qboolean	con_initialized;

int			con_notifylines;		// scan lines to clear for notify lines

extern void M_Menu_Main_f (void);

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void)
{
	if (key_dest == key_console)
	{
		if (cls.state == ca_connected)
		{
			key_dest = key_game;
			key_lines[edit_line][1] = 0;	// clear any typing
			key_linepos = 1;
		}
		else
		{
			M_Menu_Main_f ();
		}
	}
	else
		key_dest = key_console;

	SCR_EndLoadingPlaque ();
	memset (con_times, 0, sizeof (con_times));
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void)
{
	short i;

	for (i = 0; i < CON_TEXTSIZE; i++)
		con_text[i] = ' ';
}


/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void)
{
	int		i;

	for (i = 0; i < NUM_CON_TIMES; i++)
		con_times[i] = 0;
}


/*
================
Con_MessageMode_f
================
*/
extern qboolean team_message;

void Con_MessageMode_f (void)
{
	key_dest = key_message;
	team_message = false;
}


/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void)
{
	key_dest = key_message;
	team_message = true;
}


/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	short	tbuf[CON_TEXTSIZE];

	width = (vid.width >> 3) - 2;

	if (width == con_linewidth)
		return;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = 78;
		con_linewidth = width;
		con_totallines = CON_TEXTSIZE / con_linewidth;
		Con_Clear_f ();
	}
	else
	{
		oldwidth = con_linewidth;
		con_linewidth = width;
		oldtotallines = con_totallines;
		con_totallines = CON_TEXTSIZE / con_linewidth;
		numlines = oldtotallines;

		if (con_totallines < numlines)
			numlines = con_totallines;

		numchars = oldwidth;

		if (con_linewidth < numchars)
			numchars = con_linewidth;

		memcpy (tbuf, con_text, CON_TEXTSIZE << 1);
		Con_Clear_f ();

		for (i = 0; i < numlines; i++)
		{
			for (j = 0; j < numchars; j++)
			{
				con_text[(con_totallines - 1 - i) * con_linewidth + j] =
					tbuf[((con_current - i + oldtotallines) %
							oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con_backscroll = 0;
	con_current = con_totallines - 1;
}


/*
================
Con_Init
================
*/
void Con_Init (void)
{
#define MAXGAMEDIRLEN	1000
	char	temp[MAXGAMEDIRLEN+1];
	char	*t2 = "/qconsole.log";

	con_debuglog = COM_CheckParm ("-condebug");

	if (con_debuglog)
	{
		if (strlen (com_gamedir) < (MAXGAMEDIRLEN - strlen (t2)))
		{
			sprintf (temp, "%s%s", com_gamedir, t2);
			unlink (temp);
		}
	}

	con_text = Hunk_Alloc (GAME_HUNK, CON_TEXTSIZE << 1);
	Con_Clear_f ();
	con_linewidth = -1;
	Con_CheckResize ();

	Con_Printf (PRINT_NORMAL, "Console initialized.\n");

	// register our commands
	Cvar_RegisterVariable (&con_notifytime, NULL);

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
	con_initialized = true;

	PR_LoadStrings ();
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed (void)
{
	int i, j;

	con_x = 0;
	con_current++;

	j = (con_current % con_totallines) * con_linewidth;

	for (i = 0; i < con_linewidth; i++)
		con_text[i+j] = ' ';
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the notify window will pop up.
================
*/
void Con_Print (char *txt)
{
	int		y;
	int		c, l;
	static int	cr;
	int		mask;

	con_backscroll = 0;

	if (txt[0] == 1)
	{
		mask = 256;		// go to colored text
		S_LocalSound ("misc/comm.wav");
		// play talk wav
		txt++;
	}
	else if (txt[0] == 2)
	{
		mask = 256;		// go to colored text
		txt++;
	}
	else
		mask = 0;


	while ((c = *txt))
	{
		// count word length
		for (l = 0; l < con_linewidth; l++)
			if (txt[l] <= ' ')
				break;

		// word wrap
		if (l != con_linewidth && (con_x + l > con_linewidth))
			con_x = 0;

		txt++;

		if (cr)
		{
			con_current--;
			cr = false;
		}


		if (!con_x)
		{
			Con_Linefeed ();

			// mark time for transparent overlay
			if (con_current >= 0)
				con_times[con_current % NUM_CON_TIMES] = realtime;
		}

		switch (c)
		{
		case '\n':
			con_x = 0;
			break;

		case '\r':
			con_x = 0;
			cr = 1;
			break;

		default:	// display character and advance
			y = con_current % con_totallines;
			con_text[y *con_linewidth+con_x] = c | mask;
			con_x++;

			if (con_x >= con_linewidth)
				con_x = 0;

			break;
		}
	}
}


/*
================
Con_DebugLog
================
*/
#include <io.h>

void Con_DebugLog (char *file, char *fmt, ...)
{
	va_list argptr;
	static char data[1024];
	int fd;

	va_start (argptr, fmt);
	vsprintf (data, fmt, argptr);
	va_end (argptr);
	fd = open (file, O_WRONLY | O_CREAT | O_APPEND, 0666);
	write (fd, data, strlen (data));
	close (fd);
}


/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
void Con_Printf (int mode, char *msg)
{
	// don't confuse non-developers with techie stuff...
	if (mode == PRINT_DEVELOPER && !developer.value) return;

	// log all messages to file
	if (con_debuglog)
		Con_DebugLog (va ("%s/qconsole.log", com_gamedir), msg);

	if (!con_initialized)
		return;

	// write it to the scrollable buffer
	Con_Print (msg);

	/*
	H2 doesn't do this....
	// if (1) it didn't come through the Con_SafePrintf path, and (2) video is not yet fully initialized, don't do a screen update yet
	if (!r_initialized && !scr.DisabledForLoading) return;

	// update the screen if the console is displayed
	if (cls.signon != SIGNONS && !scr.DisabledForLoading && mode != PRINT_SAFE)
	{
		// SCR_UpdateScreen now has built-in recursion protection so we don't need to do it ourselves here
		SCR_UpdateScreen ();
	}
	*/
}


void VID_Printf (char *msg)
{
	Con_Printf (PRINT_SAFE, msg);
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
void Con_DrawInput (void)
{
	int		y;
	int		i;
	char	*text;

	if (key_dest != key_console && !con_forcedup)
		return;		// don't draw anything

	text = key_lines[edit_line];

	// add the cursor frame
	text[key_linepos] = 10 + ((int) (realtime * con_cursorspeed) & 1);

	// fill out remainder with spaces
	for (i = key_linepos + 1; i < con_linewidth; i++)
		text[i] = ' ';

	//	prestep if horizontally scrolling
	if (key_linepos >= con_linewidth)
		text += 1 + key_linepos - con_linewidth;

	// draw it
	y = con_vislines - 16;

	for (i = 0; i < con_linewidth; i++)
		Draw_Character ((i + 1) << 3, con_vislines - 16, text[i]);

	// remove cursor
	key_lines[edit_line][key_linepos] = 0;
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	int		x, v;
	short	*text;
	int		i;
	float	time;
	extern char chat_buffer[];

	v = 0;

	for (i = con_current - NUM_CON_TIMES + 1; i <= con_current; i++)
	{
		if (i < 0)
			continue;

		time = con_times[i % NUM_CON_TIMES];

		if (time == 0)
			continue;

		time = realtime - time;

		if (time > con_notifytime.value)
			continue;

		text = con_text + (i % con_totallines) * con_linewidth;

		clearnotify = 0;

		for (x = 0; x < con_linewidth; x++)
			Draw_Character ((x + 1) << 3, v, text[x]);

		v += 8;
	}


	if (key_dest == key_message)
	{
		clearnotify = 0;

		x = 0;

		Draw_String (8, v, "say:");

		while (chat_buffer[x])
		{
			Draw_Character ((x + 5) << 3, v, chat_buffer[x]);
			x++;
		}

		Draw_Character ((x + 5) << 3, v, 10 + ((int) (realtime * con_cursorspeed) & 1));
		v += 8;
	}

	if (v > con_notifylines)
		con_notifylines = v;
}

/*
================
Con_DrawConsole

Draws the console with the solid background
The typing input line at the bottom should only be drawn if typing is allowed
================
*/
void Con_DrawConsole (int lines, qboolean drawinput)
{
	int				i, x, y;
	int				rows;
	short			*text;
	int				j;

	if (lines <= 0)
		return;

	// draw the background
	Draw_ConsoleBackground (lines);

	// draw the text
	con_vislines = lines;

	rows = (lines - 16) >> 3;		// rows of text to draw
	y = lines - 16 - (rows << 3);	// may start slightly negative

	for (i = con_current - rows + 1; i <= con_current; i++, y += 8)
	{
		j = i - con_backscroll;

		if (j < 0)
			j = 0;

		text = con_text + (j % con_totallines) * con_linewidth;

		for (x = 0; x < con_linewidth; x++)
			Draw_Character ((x + 1) << 3, y, text[x]);
	}

	// draw the input prompt, user text, and cursor if desired
	if (drawinput)
		Con_DrawInput ();
}

