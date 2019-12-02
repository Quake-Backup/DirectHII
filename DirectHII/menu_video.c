

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "menu_common.h"
#include "d_video.h"


//========================================================
// Video menu stuff
//========================================================

static int	vid_line, vid_wmodes;

typedef struct modedesc_s
{
	int		modenum;
	char	*desc;
	int		iscur;
} modedesc_t;

#define MAX_COLUMN_SIZE		9
#define MODE_AREA_HEIGHT	(MAX_COLUMN_SIZE + 2)
#define MAX_MODEDESCS		(MAX_COLUMN_SIZE*3)

static modedesc_t	modedescs[MAX_MODEDESCS];


//=============================================================================
/* VIDEO MENU */

void M_Menu_Video_f (void)
{
	key_dest = key_menu;
	m_state = m_video;
	m_soundlevel = m_sound_menu2;
}


void M_Video_Draw (void)
{
	/*
	D3D9_REMOVED
	char		*ptr;
	int			lnummodes, i, k, column, row;
	vmode_t		*pv;
	*/

	ScrollTitle ("gfx/menu/title7.lmp");

	/*
	D3D9_REMOVED
	vid_wmodes = 0;
	lnummodes = VID_NumModes ();

	for (i = 1; (i < lnummodes) && (vid_wmodes < MAX_MODEDESCS); i++)
	{
		ptr = D_GetModeDescription (i);
		pv = VID_GetModePtr (i);

		k = vid_wmodes;

		modedescs[k].modenum = i;
		modedescs[k].desc = ptr;
		modedescs[k].iscur = 0;

		if (i == vid_modenum)
			modedescs[k].iscur = 1;

		vid_wmodes++;

	}

	if (vid_wmodes > 0)
	{
		M_Print (2 * 8, 60 + 0 * 8, "Fullscreen Modes (WIDTHxHEIGHTxBPP)");

		column = 8;
		row = 60 + 2 * 8;

		for (i = 0; i < vid_wmodes; i++)
		{
			if (modedescs[i].iscur)
				M_PrintWhite (column, row, modedescs[i].desc);
			else
				M_Print (column, row, modedescs[i].desc);

			column += 13 * 8;

			if ((i % VID_ROW_SIZE) == (VID_ROW_SIZE - 1))
			{
				column = 8;
				row += 8;
			}
		}
	}
	*/

	M_Print (3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 2, "Video modes must be set from the");
	M_Print (3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 3, "command line with -width <width>");
	M_Print (3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 4, "and -bpp <bits-per-pixel>");
	M_Print (3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 6, "Select windowed mode with -window");
}


void M_Video_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		m_soundlevel = m_sound_menu1;
		M_Menu_Options_f ();
		break;

	default:
		break;
	}
}


