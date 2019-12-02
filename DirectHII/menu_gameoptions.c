
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"


//=============================================================================
/* GAME OPTIONS MENU */

typedef struct level_s {
	char	*name;
	char	*description;
} level_t;

level_t		levels[] =
{
	{"demo1", "Blackmarsh"},							// 0
	{"demo2", "Barbican"},								// 1

	{"ravdm1", "Deathmatch 1"},							// 2

	{"demo1", "Blackmarsh"},								// 3
	{"demo2", "Barbican"},								// 4
	{"demo3", "The Mill"},								// 5
	{"village1", "King's Court"},						// 6
	{"village2", "Inner Courtyard"},						// 7
	{"village3", "Stables"},								// 8
	{"village4", "Palace Entrance"},						// 9
	{"village5", "The Forgotten Chapel"},				// 10
	{"rider1a", "Famine's Domain"},						// 11

	{"meso2", "Plaza of the Sun"},						// 12
	{"meso1", "The Palace of Columns"},					// 13
	{"meso3", "Square of the Stream"},					// 14
	{"meso4", "Tomb of the High Priest"},				// 15
	{"meso5", "Obelisk of the Moon"},					// 16
	{"meso6", "Court of 1000 Warriors"},					// 17
	{"meso8", "Bridge of Stars"},						// 18
	{"meso9", "Well of Souls"},							// 19

	{"egypt1", "Temple of Horus"},						// 20
	{"egypt2", "Ancient Temple of Nefertum"},			// 21
	{"egypt3", "Temple of Nefertum"},					// 22
	{"egypt4", "Palace of the Pharaoh"},					// 23
	{"egypt5", "Pyramid of Anubis"},						// 24
	{"egypt6", "Temple of Light"},						// 25
	{"egypt7", "Shrine of Naos"},						// 26
	{"rider2c", "Pestilence's Lair"},					// 27

	{"romeric1", "The Hall of Heroes"},					// 28
	{"romeric2", "Gardens of Athena"},					// 29
	{"romeric3", "Forum of Zeus"},						// 30
	{"romeric4", "Baths of Demetrius"},					// 31
	{"romeric5", "Temple of Mars"},						// 32
	{"romeric6", "Coliseum of War"},						// 33
	{"romeric7", "Reflecting Pool"},						// 34

	{"cath", "Cathedral"},								// 35
	{"tower", "Tower of the Dark Mage"},					// 36
	{"castle4", "The Underhalls"},						// 37
	{"castle5", "Eidolon's Ordeal"},						// 38
	{"eidolon", "Eidolon's Lair"},						// 39

	{"ravdm1", "Atrium of Immolation"},					// 40
	{"ravdm2", "Total Carnage"},							// 41
	{"ravdm3", "Reckless Abandon"},						// 42
	{"ravdm4", "Temple of RA"},							// 43
	{"ravdm5", "Tom Foolery"},							// 44

	{"ravdm1", "Deathmatch 1"},							// 45

	//OEM
	{"demo1", "Blackmarsh"},								// 46
	{"demo2", "Barbican"},								// 47
	{"demo3", "The Mill"},								// 48
	{"village1", "King's Court"},						// 49
	{"village2", "Inner Courtyard"},						// 50
	{"village3", "Stables"},								// 51
	{"village4", "Palace Entrance"},						// 52
	{"village5", "The Forgotten Chapel"},				// 53
	{"rider1a", "Famine's Domain"},						// 54

	//Mission Pack
	{"keep1", "Eidolon's Lair"},						// 55
	{"keep2", "Village of Turnabel"},					// 56
	{"keep3", "Duke's Keep"},							// 57
	{"keep4", "The Catacombs"},						// 58
	{"keep5", "Hall of the Dead"},					// 59

	{"tibet1", "Tulku"},								// 60
	{"tibet2", "Ice Caverns"},							// 61
	{"tibet3", "The False Temple"},					// 62
	{"tibet4", "Courtyards of Tsok"},					// 63
	{"tibet5", "Temple of Kalachakra"},				// 64
	{"tibet6", "Temple of Bardo"},						// 65
	{"tibet7", "Temple of Phurbu"},					// 66
	{"tibet8", "Palace of Emperor Egg Chen"},			// 67
	{"tibet9", "Palace Inner Chambers"},				// 68
	{"tibet10", "The Inner Sanctum of Praevus"},		// 69
};

typedef struct episode_s {
	char	*description;
	int		firstLevel;
	int		levels;
} episode_t;

episode_t	episodes[] =
{
	// Demo
	{"Demo", 0, 2},
	{"Demo Deathmatch", 2, 1},

	// Registered
	{"Village", 3, 9},
	{"Meso", 12, 8},
	{"Egypt", 20, 8},
	{"Romeric", 28, 7},
	{"Cathedral", 35, 5},
	{"MISSION PACK", 55, 15},
	{"Deathmatch", 40, 5},

	// OEM
	{"Village", 46, 9},
	{"Deathmatch", 45, 1},
};

#define OEM_START 9
#define REG_START 2
#define MP_START 7
#define DM_START 8

int	startepisode;
int	startlevel;
int maxplayers;
qboolean m_serverInfoMessage = false;
double m_serverInfoMessageTime;

int gameoptions_cursor_table[] = {40, 56, 64, 72, 80, 88, 96, 104, 112, 128, 136};
#define	NUM_GAMEOPTIONS	11
int		gameoptions_cursor;

void M_Menu_GameOptions_f (void)
{
	key_dest = key_menu;
	m_state = m_gameoptions;
	m_soundlevel = m_sound_menu2;

	if (maxplayers == 0)
		maxplayers = svs.maxclients;

	if (maxplayers < 2)
		maxplayers = svs.maxclientslimit;

	setup_class = cl_playerclass.value;

	if (setup_class < 1 || setup_class > NUM_CLASSES)
		setup_class = NUM_CLASSES;

	setup_class--;

	if (registered.value && (startepisode < REG_START || startepisode >= OEM_START))
		startepisode = REG_START;

	if (coop.value)
	{
		startlevel = 0;

		if (startepisode == 1)
			startepisode = 0;
		else if (startepisode == DM_START)
			startepisode = REG_START;

		if (gameoptions_cursor >= NUM_GAMEOPTIONS - 1)
			gameoptions_cursor = 0;
	}
}


void M_GameOptions_Draw (void)
{
	ScrollTitle ("gfx/menu/title4.lmp");

	M_DrawTextBox (152 + 8, 60, 10, 1);
	M_Print (160 + 8, 68, "begin game");

	M_Print (0 + 8, 84, "      Max players");
	M_Print (160 + 8, 84, va ("%i", maxplayers));

	M_Print (0 + 8, 92, "        Game Type");

	if (coop.value)
		M_Print (160 + 8, 92, "Cooperative");
	else
		M_Print (160 + 8, 92, "Deathmatch");

	M_Print (0 + 8, 100, "        Teamplay");
	/*	if (rogue)
		{
		char *msg;

		switch((int)teamplay.value)
		{
		case 1: msg = "No Friendly Fire"; break;
		case 2: msg = "Friendly Fire"; break;
		case 3: msg = "Tag"; break;
		case 4: msg = "Capture the Flag"; break;
		case 5: msg = "One Flag CTF"; break;
		case 6: msg = "Three Team CTF"; break;
		default: msg = "Off"; break;
		}
		M_Print (160+8, 100, msg);
		}
		else
		*/
	{
		char *msg;

		switch ((int) teamplay.value)
		{
		case 1: msg = "No Friendly Fire"; break;
		case 2: msg = "Friendly Fire"; break;
		default: msg = "Off"; break;
		}

		M_Print (160 + 8, 100, msg);
	}

	M_Print (0 + 8, 108, "            Class");
	M_Print (160 + 8, 108, ClassNames[setup_class]);

	M_Print (0 + 8, 116, "       Difficulty");

	M_Print (160 + 8, 116, DiffNames[setup_class][(int) skill.value]);

	M_Print (0 + 8, 124, "       Frag Limit");

	if (fraglimit.value == 0)
		M_Print (160 + 8, 124, "none");
	else
		M_Print (160 + 8, 124, va ("%i frags", (int) fraglimit.value));

	M_Print (0 + 8, 132, "       Time Limit");

	if (timelimit.value == 0)
		M_Print (160 + 8, 132, "none");
	else
		M_Print (160 + 8, 132, va ("%i minutes", (int) timelimit.value));

	M_Print (0 + 8, 140, "     Random Class");

	if (randomclass.value)
		M_Print (160 + 8, 140, "on");
	else
		M_Print (160 + 8, 140, "off");

	M_Print (0 + 8, 156, "         Episode");
	M_Print (160 + 8, 156, episodes[startepisode].description);

	M_Print (0 + 8, 164, "           Level");
	M_Print (160 + 8, 164, levels[episodes[startepisode].firstLevel + startlevel].name);
	M_Print (96, 180, levels[episodes[startepisode].firstLevel + startlevel].description);

	// line cursor
	M_DrawCharacter (172 - 16, gameoptions_cursor_table[gameoptions_cursor] + 28, 12 + ((int) (realtime * 4) & 1));

	/*	rjr
		if (m_serverInfoMessage)
		{
		if ((realtime - m_serverInfoMessageTime) < 5.0)
		{
		x = (320-26*8)/2;
		M_DrawTextBox (x, 138, 24, 4);
		x += 8;
		M_Print (x, 146, "  More than 4 players   ");
		M_Print (x, 154, " requires using command ");
		M_Print (x, 162, "line parameters; please ");
		M_Print (x, 170, "   see techinfo.txt.    ");
		}
		else
		{
		m_serverInfoMessage = false;
		}
		}*/
}


void M_NetStart_Change (int dir)
{
	int count;

	switch (gameoptions_cursor)
	{
	case 1:
		maxplayers += dir;

		if (maxplayers > svs.maxclientslimit)
		{
			maxplayers = svs.maxclientslimit;
			m_serverInfoMessage = true;
			m_serverInfoMessageTime = realtime;
		}

		if (maxplayers < 2)
			maxplayers = 2;

		break;

	case 2:
		Cvar_SetValue ("coop", coop.value ? 0 : 1);

		if (coop.value)
		{
			startlevel = 0;

			if (startepisode == 1)
				startepisode = 0;
			else if (startepisode == DM_START)
				startepisode = REG_START;
		}

		break;

	case 3:
		//		if (rogue)
		//			count = 6;
		//		else
		count = 2;

		Cvar_SetValue ("teamplay", teamplay.value + dir);

		if (teamplay.value > count)
			Cvar_SetValue ("teamplay", 0);
		else if (teamplay.value < 0)
			Cvar_SetValue ("teamplay", count);

		break;

	case 4:
		setup_class += dir;

		if (setup_class < 0)
			setup_class = NUM_CLASSES - 1;

		if (setup_class > NUM_CLASSES - 1)
			setup_class = 0;

		break;

	case 5:
		Cvar_SetValue ("skill", skill.value + dir);

		if (skill.value > 3)
			Cvar_SetValue ("skill", 0);

		if (skill.value < 0)
			Cvar_SetValue ("skill", 3);

		break;

	case 6:
		Cvar_SetValue ("fraglimit", fraglimit.value + dir * 10);

		if (fraglimit.value > 100)
			Cvar_SetValue ("fraglimit", 0);

		if (fraglimit.value < 0)
			Cvar_SetValue ("fraglimit", 100);

		break;

	case 7:
		Cvar_SetValue ("timelimit", timelimit.value + dir * 5);

		if (timelimit.value > 60)
			Cvar_SetValue ("timelimit", 0);

		if (timelimit.value < 0)
			Cvar_SetValue ("timelimit", 60);

		break;

	case 8:

		if (randomclass.value)
			Cvar_SetValue ("randomclass", 0);
		else
			Cvar_SetValue ("randomclass", 1);

		break;

	case 9:
		startepisode += dir;

		if (registered.value)
		{
			count = DM_START;

			if (!coop.value)
				count++;

			if (startepisode < REG_START)
				startepisode = count - 1;

			if (startepisode >= count)
				startepisode = REG_START;

			startlevel = 0;
		}
		else
		{
			count = 2;

			if (startepisode < 0)
				startepisode = count - 1;

			if (startepisode >= count)
				startepisode = 0;

			startlevel = 0;
		}

		break;

	case 10:

		if (coop.value)
		{
			startlevel = 0;
			break;
		}

		startlevel += dir;
		count = episodes[startepisode].levels;

		if (startlevel < 0)
			startlevel = count - 1;

		if (startlevel >= count)
			startlevel = 0;

		break;
	}
}

void M_GameOptions_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		m_soundlevel = m_sound_menu1;
		gameoptions_cursor--;

		if (gameoptions_cursor < 0)
		{
			gameoptions_cursor = NUM_GAMEOPTIONS - 1;

			if (coop.value)
				gameoptions_cursor--;
		}

		break;

	case K_DOWNARROW:
		m_soundlevel = m_sound_menu1;
		gameoptions_cursor++;

		if (coop.value)
		{
			if (gameoptions_cursor >= NUM_GAMEOPTIONS - 1)
				gameoptions_cursor = 0;
		}
		else
		{
			if (gameoptions_cursor >= NUM_GAMEOPTIONS)
				gameoptions_cursor = 0;
		}

		break;

	case K_LEFTARROW:

		if (gameoptions_cursor == 0)
			break;

		m_soundlevel = m_sound_menu3;
		M_NetStart_Change (-1);
		break;

	case K_RIGHTARROW:

		if (gameoptions_cursor == 0)
			break;

		m_soundlevel = m_sound_menu3;
		M_NetStart_Change (1);
		break;

	case K_ENTER:
		m_soundlevel = m_sound_menu2;

		if (gameoptions_cursor == 0)
		{
			if (sv.active)
				Cbuf_AddText ("disconnect\n");

			Cbuf_AddText (va ("playerclass %d\n", setup_class + 1));
			Cbuf_AddText ("listen 0\n");	// so host_netport will be re-examined
			Cbuf_AddText (va ("maxplayers %u\n", maxplayers));
			SCR_BeginLoadingPlaque ();

			Cbuf_AddText (va ("map %s\n", levels[episodes[startepisode].firstLevel + startlevel].name));

			return;
		}

		M_NetStart_Change (1);
		break;
	}
}


