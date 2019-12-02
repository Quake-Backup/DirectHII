
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"

//=============================================================================
/* QUIT MENU */

//int		msgNumber;
int		m_quit_prevstate;
qboolean	wasInMenus;
extern qboolean	m_recursiveDraw;

#ifndef	_WIN32
char *quitMessage [] =
{
	/* .........1.........2.... */
	"   Look! Behind you!    ",
	"  There's a big nasty   ",
	"   thing - shoot it!    ",
	"                        ",

	"  You can't go now, I   ",
	"   was just getting     ",
	"    warmed up.          ",
	"                        ",

	"    One more game.      ",
	"      C'mon...          ",
	"   Who's gonna know?    ",
	"                        ",

	"   What's the matter?   ",
	"   Palms too sweaty to  ",
	"     keep playing?      ",
	"                        ",

	"  Watch your local store",
	"      for Hexen 2       ",
	"    plush toys and      ",
	"    greeting cards!     ",

	"  Hexen 2...            ",
	"                        ",
	"    Too much is never   ",
	"        enough.         ",

	"  Sure go ahead and     ",
	"  leave.  But I know    ",
	"  you'll be back.       ",
	"                        ",

	"                        ",
	"  Insert cute phrase    ",
	"        here            ",
	"                        "
};
#endif

static float LinePos;
static int LineTimes;
static int MaxLines;
char **LineText;
static qboolean SoundPlayed;


#define MAX_LINES 138

char *CreditText[MAX_LINES] =
{
	"Project Director: James Monroe",
	"Creative Director: Brian Raffel",
	"Project Coordinator: Kevin Schilder",
	"",
	"Lead Programmer: James Monroe",
	"",
	"Programming:",
	"   Mike Gummelt",
	"   Josh Weier",
	"",
	"Additional Programming:",
	"   Josh Heitzman",
	"   Nathan Albury",
	"   Rick Johnson",
	"",
	"Assembly Consultant:",
	"   Mr. John Scott",
	"",
	"Lead Design: Jon Zuk",
	"",
	"Design:",
	"   Tom Odell",
	"   Jeremy Statz",
	"   Mike Renner",
	"   Eric Biessman",
	"   Kenn Hoekstra",
	"   Matt Pinkston",
	"   Bobby Duncanson",
	"   Brian Raffel",
	"",
	"Art Director: Les Dorscheid",
	"",
	"Art:",
	"   Kim Lathrop",
	"   Gina Garren",
	"   Joe Koberstein",
	"   Kevin Long",
	"   Jeff Butler",
	"   Scott Rice",
	"   John Payne",
	"   Steve Raffel",
	"",
	"Animation:",
	"   Eric Turman",
	"   Chaos (Mike Werckle)",
	"",
	"Music:",
	"   Kevin Schilder",
	"",
	"Sound:",
	"   Chia Chin Lee",
	"",
	"Activision",
	"",
	"Producer:",
	"   Steve Stringer",
	"",
	"Marketing Product Manager:",
	"   Henk Hartong",
	"",
	"Marketing Associate:",
	"   Kevin Kraff",
	"",
	"Senior Quality",
	"Assurance Lead:",
	"   Tim Vanlaw",
	"",
	"Quality Assurance Lead:",
	"   Doug Jacobs",
	"",
	"Quality Assurance Team:",
	"   Steve Rosenthal, Steve Elwell,",
	"   Chad Bordwell, David Baker,",
	"   Aaron Casillas, Damien Fischer,",
	"   Winnie Lee, Igor Krinitskiy,",
	"   Samantha Lee, John Park",
	"   Ian Stevens, Chris Toft",
	"",
	"Production Testers:",
	"   Steve Rosenthal and",
	"   Chad Bordwell",
	"",
	"Additional QA and Support:",
	"    Tony Villalobos",
	"    Jason Sullivan",
	"",
	"Installer by:",
	"   Steve Stringer, Adam Goldberg,",
	"   Tanya Martino, Eric Schmidt,",
	"   Ronnie Lane",
	"",
	"Art Assistance by:",
	"   Carey Chico and Franz Boehm",
	"",
	"BizDev Babe:",
	"   Jamie Bafus",
	"",
	"And...",
	"",
	"Our Big Toe:",
	"   Mitch Lasky",
	"",
	"",
	"Special Thanks to:",
	"  Id software",
	"  The original Hexen2 crew",
	"   We couldn't have done it",
	"   without you guys!",
	"",
	"",
	"Published by Id Software, Inc.",
	"Distributed by Activision, Inc.",
	"",
	"The Id Software Technology used",
	"under license in Hexen II (tm)",
	"(c) 1996, 1997 Id Software, Inc.",
	"All Rights Reserved.",
	"",
	"Hexen(r) is a registered trademark",
	"of Raven Software Corp.",
	"Hexen II (tm) and the Raven logo",
	"are trademarks of Raven Software",
	"Corp.  The Id Software name and",
	"id logo are trademarks of",
	"Id Software, Inc.  Activision(r)",
	"is a registered trademark of",
	"Activision, Inc. All other",
	"trademarks are the property of",
	"their respective owners.",
	"",
	"",
	"",
	"Send bug descriptions to:",
	"   h2bugs@mail.ravensoft.com",
	"",
	"",
	"No yaks were harmed in the",
	"making of this game!"
};

#define MAX_LINES2 150

char *Credit2Text[MAX_LINES2] =
{
	"PowerTrip: James (emorog) Monroe",
	"Cartoons: Brian Raffel",
	"         (use more puzzles)",
	"Doc Keeper: Kevin Schilder",
	"",
	"Whip cracker: James Monroe",
	"",
	"Whipees:",
	"   Mike (i didn't break it) Gummelt",
	"   Josh (extern) Weier",
	"",
	"We don't deserve whipping:",
	"   Josh (I'm not on this project)",
	"         Heitzman",
	"   Nathan (deer hunter) Albury",
	"   Rick (model crusher) Johnson",
	"",
	"Bit Packer:",
	"   Mr. John (Slaine) Scott",
	"",
	"Lead Slacker: Jon (devil boy) Zuk",
	"",
	"Other Slackers:",
	"   Tom (can i have an office) Odell",
	"   Jeremy (nt crashed again) Statz",
	"   Mike (i should be doing my ",
	"         homework) Renner",
	"   Eric (the nose) Biessman",
	"   Kenn (.plan) Hoekstra",
	"   Matt (big elbow) Pinkston",
	"   Bobby (needs haircut) Duncanson",
	"   Brian (they're in my town) Raffel",
	"",
	"Use the mouse: Les Dorscheid",
	"",
	"What's a mouse?:",
	"   Kim (where's my desk) Lathrop",
	"   Gina (i can do your laundry)",
	"        Garren",
	"   Joe (broken axle) Koberstein",
	"   Kevin (titanic) Long",
	"   Jeff (norbert) Butler",
	"   Scott (what's the DEL key for?)",
	"          Rice",
	"   John (Shpluuurt!) Payne",
	"   Steve (crash) Raffel",
	"",
	"Boners:",
	"   Eric (terminator) Turman",
	"   Chaos Device",
	"",
	"Drum beater:",
	"   Kevin Schilder",
	"",
	"Whistle blower:",
	"   Chia Chin (bruce) Lee",
	"",
	"",
	"Activision",
	"",
	"Producer:",
	"   Steve 'Ferris' Stringer",
	"",
	"Marketing Product Manager:",
	"   Henk 'GODMODE' Hartong",
	"",
	"Marketing Associate:",
	"   Kevin 'Kraffinator' Kraff",
	"",
	"Senior Quality",
	"Assurance Lead:",
	"   Tim 'Outlaw' Vanlaw",
	"",
	"Quality Assurance Lead:",
	"   Doug Jacobs",
	"",
	"Shadow Finders:",
	"   Steve Rosenthal, Steve Elwell,",
	"   Chad Bordwell,",
	"   David 'Spice Girl' Baker,",
	"   Error Casillas, Damien Fischer,",
	"   Winnie Lee,"
	"   Ygor Krynytyskyy,",
	"   Samantha (Crusher) Lee, John Park",
	"   Ian Stevens, Chris Toft",
	"",
	"Production Testers:",
	"   Steve 'Damn It's Cold!'",
	"       Rosenthal and",
	"   Chad 'What Hotel Receipt?'",
	"        Bordwell",
	"",
	"Additional QA and Support:",
	"    Tony Villalobos",
	"    Jason Sullivan",
	"",
	"Installer by:",
	"   Steve 'Bahh' Stringer,",
	"   Adam Goldberg, Tanya Martino,",
	"   Eric Schmidt, Ronnie Lane",
	"",
	"Art Assistance by:",
	"   Carey 'Damien' Chico and",
	"   Franz Boehm",
	"",
	"BizDev Babe:",
	"   Jamie Bafus",
	"",
	"And...",
	"",
	"Our Big Toe:",
	"   Mitch Lasky",
	"",
	"",
	"Special Thanks to:",
	"  Id software",
	"  Anyone who ever worked for Raven,",
	"  (except for Alex)",
	"",
	"",
	"Published by Id Software, Inc.",
	"Distributed by Activision, Inc.",
	"",
	"The Id Software Technology used",
	"under license in Hexen II (tm)",
	"(c) 1996, 1997 Id Software, Inc.",
	"All Rights Reserved.",
	"",
	"Hexen(r) is a registered trademark",
	"of Raven Software Corp.",
	"Hexen II (tm) and the Raven logo",
	"are trademarks of Raven Software",
	"Corp.  The Id Software name and",
	"id logo are trademarks of",
	"Id Software, Inc.  Activision(r)",
	"is a registered trademark of",
	"Activision, Inc. All other",
	"trademarks are the property of",
	"their respective owners.",
	"",
	"",
	"",
	"Send bug descriptions to:",
	"   h2bugs@mail.ravensoft.com",
	"",
	"Special Thanks To:",
	"   E.H.S., The Osmonds,",
	"   B.B.V.D., Daisy The Lovin' Lamb,",
	"  'You Killed' Kenny,",
	"   and Baby Biessman.",
	"",
};

#define QUIT_SIZE 18

void M_Menu_Quit_f (void)
{
	if (m_state == m_quit)
		return;

	wasInMenus = (key_dest == key_menu);
	key_dest = key_menu;
	m_quit_prevstate = m_state;
	m_state = m_quit;
	m_soundlevel = m_sound_menu2;
	//	msgNumber = rand ()&7;

	LinePos = 0;
	LineTimes = 0;
	LineText = CreditText;
	MaxLines = MAX_LINES;
	SoundPlayed = false;
}


void M_Quit_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case 'n':
	case 'N':

		if (wasInMenus)
		{
			m_state = m_quit_prevstate;
			m_soundlevel = m_sound_menu2;
		}
		else
		{
			key_dest = key_game;
			m_state = m_none;
		}

		break;

	case 'Y':
	case 'y':
		key_dest = key_console;
		Host_Quit_f ();
		break;

	default:
		break;
	}

}

void M_Quit_Draw (void)
{
	int i, x, y, place, topy;
	qpic_t	*p;

	if (wasInMenus)
	{
		m_state = m_quit_prevstate;
		m_recursiveDraw = true;
		M_Draw ();
		m_state = m_quit;
	}

	LinePos += scr_frametime * 1.75;

	if (LinePos > MaxLines + QUIT_SIZE + 2)
	{
		LinePos = 0;
		SoundPlayed = false;
		LineTimes++;

		if (LineTimes >= 2)
		{
			MaxLines = MAX_LINES2;
			LineText = Credit2Text;
			CDAudio_Play (12, false);
		}
	}

	y = 12;
	M_DrawTextBox (0, 0, 38, 23);
	M_PrintWhite (16, y, "        Hexen II version 1.12       ");	y += 8;
	M_PrintWhite (16, y, "         by Raven Software          ");	y += 16;

	if (LinePos > 55 && !SoundPlayed && LineText == Credit2Text)
	{
		m_soundlevel = m_sound_steve;
		SoundPlayed = true;
	}

	topy = y;
	place = floor (LinePos);
	y -= floor ((LinePos - place) * 8);

	for (i = 0; i < QUIT_SIZE; i++, y += 8)
	{
		if (i + place - QUIT_SIZE >= MaxLines)
			break;

		if (i + place < QUIT_SIZE)
			continue;

		if (LineText[i + place - QUIT_SIZE][0] == ' ')
			M_PrintWhite (24, y, LineText[i + place - QUIT_SIZE]);
		else
			M_Print (24, y, LineText[i + place - QUIT_SIZE]);
	}

	p = Draw_CachePic ("gfx/box_mm2.lmp");
	x = 24;
	y = topy - 8;

	for (i = 4; i < 38; i++, x += 8)
	{
		M_DrawPic (x, y, p);	//background at top for smooth scroll out
		M_DrawPic (x, y + (QUIT_SIZE * 8), p);	//draw at bottom for smooth scroll in
	}

	y += (QUIT_SIZE * 8) + 8;
	M_PrintWhite (16, y, "          Press y to exit           ");
}
