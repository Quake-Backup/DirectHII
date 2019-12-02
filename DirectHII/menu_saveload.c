
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"


//=============================================================================
/* LOAD/SAVE MENU */

int		load_cursor;		// 0 < load_cursor < MAX_SAVEGAMES

#define	MAX_SAVEGAMES		12
char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH + 1];
int		loadable[MAX_SAVEGAMES];

void M_ScanSaves (void)
{
	int		i, j;
	char	name[MAX_OSPATH];
	FILE	*f;
	int		version;

	for (i = 0; i < MAX_SAVEGAMES; i++)
	{
		strcpy (m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		sprintf (name, "%s/s%i/info.dat", com_gamedir, i);
		f = fopen (name, "r");

		if (!f)
			continue;

		fscanf (f, "%i\n", &version);
		fscanf (f, "%79s\n", name);
		strncpy (m_filenames[i], name, sizeof (m_filenames[i]) - 1);

		// change _ back to space
		for (j = 0; j < SAVEGAME_COMMENT_LENGTH; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';

		loadable[i] = true;
		fclose (f);
	}
}

void M_Menu_Load_f (void)
{
	m_soundlevel = m_sound_menu2;
	m_state = m_load;
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Menu_Save_f (void)
{
	if (!sv.active)
		return;

	if (cl.intermission)
		return;

	if (svs.maxclients != 1)
		return;

	m_soundlevel = m_sound_menu2;
	m_state = m_save;
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Load_Draw (void)
{
	int		i;

	ScrollTitle ("gfx/menu/load.lmp");

	for (i = 0; i < MAX_SAVEGAMES; i++)
		M_Print (16, 60 + 8 * i, m_filenames[i]);

	// line cursor
	M_DrawCharacter (8, 60 + load_cursor * 8, 12 + ((int) (realtime * 4) & 1));
}


void M_Save_Draw (void)
{
	int		i;

	ScrollTitle ("gfx/menu/save.lmp");

	for (i = 0; i < MAX_SAVEGAMES; i++)
		M_Print (16, 60 + 8 * i, m_filenames[i]);

	// line cursor
	M_DrawCharacter (8, 60 + load_cursor * 8, 12 + ((int) (realtime * 4) & 1));
}


void M_Load_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		m_soundlevel = m_sound_menu2;

		if (!loadable[load_cursor])
			return;

		m_state = m_none;
		key_dest = key_game;

		// Host_Loadgame_f can't bring up the loading plaque because too much
		// stack space has been used, so do it now
		SCR_BeginLoadingPlaque ();

		// issue the load command
		Cbuf_AddText (va ("load s%i\n", load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		m_soundlevel = m_sound_menu1;
		load_cursor--;

		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES - 1;

		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		m_soundlevel = m_sound_menu1;
		load_cursor++;

		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;

		break;
	}
}


void M_Save_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		m_state = m_none;
		key_dest = key_game;
		Cbuf_AddText (va ("save s%i\n", load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		m_soundlevel = m_sound_menu1;
		load_cursor--;

		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES - 1;

		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		m_soundlevel = m_sound_menu1;
		load_cursor++;

		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;

		break;
	}
}


//=============================================================================
/* MULTIPLAYER LOAD/SAVE MENU */

char *message, *message2;
double message_time;

void M_ScanMSaves (void)
{
	int		i, j;
	char	name[MAX_OSPATH];
	FILE	*f;
	int		version;

	for (i = 0; i < MAX_SAVEGAMES; i++)
	{
		strcpy (m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		sprintf (name, "%s/ms%i/info.dat", com_gamedir, i);
		f = fopen (name, "r");

		if (!f)
			continue;

		fscanf (f, "%i\n", &version);
		fscanf (f, "%79s\n", name);
		strncpy (m_filenames[i], name, sizeof (m_filenames[i]) - 1);

		// change _ back to space
		for (j = 0; j < SAVEGAME_COMMENT_LENGTH; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';

		loadable[i] = true;
		fclose (f);
	}
}

void M_Menu_MLoad_f (void)
{
	m_soundlevel = m_sound_menu2;
	m_state = m_mload;
	key_dest = key_menu;
	M_ScanMSaves ();
}


void M_Menu_MSave_f (void)
{
	if (!sv.active || cl.intermission || svs.maxclients == 1)
	{
		message = "Only a network server";
		message2 = "can save a multiplayer game";
		message_time = realtime;
		return;
	}

	m_soundlevel = m_sound_menu2;
	m_state = m_msave;
	key_dest = key_menu;
	M_ScanMSaves ();
}


void M_MLoad_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_ENTER:
		m_soundlevel = m_sound_menu2;

		if (!loadable[load_cursor])
			return;

		m_state = m_none;
		key_dest = key_game;

		if (sv.active)
			Cbuf_AddText ("disconnect\n");

		Cbuf_AddText ("listen 1\n");	// so host_netport will be re-examined

		// Host_Loadgame_f can't bring up the loading plaque because too much
		// stack space has been used, so do it now
		SCR_BeginLoadingPlaque ();

		// issue the load command
		Cbuf_AddText (va ("load ms%i\n", load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		m_soundlevel = m_sound_menu1;
		load_cursor--;

		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES - 1;

		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		m_soundlevel = m_sound_menu1;
		load_cursor++;

		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;

		break;
	}
}


void M_MSave_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_ENTER:
		m_state = m_none;
		key_dest = key_game;
		Cbuf_AddText (va ("save ms%i\n", load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		m_soundlevel = m_sound_menu1;
		load_cursor--;

		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES - 1;

		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		m_soundlevel = m_sound_menu1;
		load_cursor++;

		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;

		break;
	}
}


