// gl_vidnt.c -- NT GL vid component

#include "precompiled.h"
#include "resource.h"

#include "quakedef.h"
#include "winquake.h"

#include "d_video.h"


/*
=================
VID_NumModes
=================
*/
int VID_NumModes (void)
{
	/*
	D3D9_REMOVED
	return nummodes;
	*/
	return 0;
}


/*
=================
VID_GetModePtr
=================
*/
/*
D3D9_REMOVED
vmode_t *VID_GetModePtr (int modenum)
{
	if ((modenum >= 0) && (modenum < nummodes))
		return &modelist[modenum];
	else
	{
		strcpy (badmode.modedesc, "Bad mode");
		return &badmode;
	}
	return NULL;
}
*/



// KJB: Added this to return the mode driver name in description for console

char *VID_GetExtModeDescription (int mode)
{
	/*
	D3D9_REMOVED
	static char	pinfo[40];
	vmode_t		*pv;

	if ((mode < 0) || (mode >= nummodes))
		return NULL;


	return pinfo;
	*/
	return NULL;
}


/*
=================
VID_DescribeCurrentMode_f
=================
*/
void VID_DescribeCurrentMode_f (void)
{
	/*
	D3D9_REMOVED
	Con_Printf (PRINT_NORMAL, va ("%s\n", VID_GetExtModeDescription (vid_modenum)));
	*/
}


/*
=================
VID_NumModes_f
=================
*/
void VID_NumModes_f (void)
{
	/*
	D3D9_REMOVED
	if (nummodes == 1)
		Con_Printf (PRINT_NORMAL, va ("%d video mode is available\n", nummodes));
	else
		Con_Printf (PRINT_NORMAL, va ("%d video modes are available\n", nummodes));
		*/
}


/*
=================
VID_DescribeMode_f
=================
*/
void VID_DescribeMode_f (void)
{
	/*
	D3D9_REMOVED
	int		t, modenum;

	modenum = atoi (Cmd_Argv (1));
	Con_Printf (PRINT_NORMAL, va ("%s\n", VID_GetExtModeDescription (modenum)));
	*/
}


/*
=================
VID_DescribeModes_f
=================
*/
void VID_DescribeModes_f (void)
{
	/*
	D3D9_REMOVED
	int			i, lnummodes, t;
	char		*pinfo;
	vmode_t		*pv;

	lnummodes = VID_NumModes ();
	*/
}


