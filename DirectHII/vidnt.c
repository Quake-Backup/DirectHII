// gl_vidnt.c -- NT GL vid component

#include "precompiled.h"
#include "resource.h"

#include "quakedef.h"
#include "winquake.h"

#include "d_video.h"
#include "d_image.h"
#include "d_texture.h"

byte globalcolormap[VID_GRADES * 256];

qboolean		scr_skipupdate;

qboolean	vid_initialized = false;
qboolean vid_ModeChangeRequested = false;

int			vid_modenum = NO_MODE;

float RTint[256], GTint[256], BTint[256];

viddef_t	vid;				// global video state

unsigned	d_8to24table_char[256];
unsigned	d_8to24table_rgba[256];
unsigned	d_8to24table_part[256];

// palettes for special mdl texture types
unsigned	d_8to24table_holey[256];
unsigned	d_8to24table_trans[256];
unsigned	d_8to24table_spec[256];

modestate_t	modestate = MS_UNINIT;


void Sys_AppActivate (BOOL fActive, BOOL minimize);
char *D_GetModeDescription (int mode);
void Sys_ClearAllStates (void);
void GL_Init (void);

// forward decls for Cmd_AddCommand
void VID_NumModes_f (void);
void VID_DescribeCurrentMode_f (void);
void VID_DescribeMode_f (void);
void VID_DescribeModes_f (void);


// valid window styles to retain aero
static DWORD WindowStyle = (WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_BORDER | WS_DLGFRAME | WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX);
static DWORD ExWindowStyle = (WS_EX_WINDOWEDGE);


//====================================

// vid_mode 0 is windowed; vid_modes 1+ are fullscreen
// if vid_mode 0 is set then the dimensions come from the vid_width and vid_height cvars
// otherwise the dimensions come from the mode
// vid_vsync may always be set irrespective
cvar_t		vid_mode = {"vid_mode", "0", true};
cvar_t		vid_vsync = {"vid_vsync", "0", true};
cvar_t		vid_width = {"vid_width", "800", true};
cvar_t		vid_height = {"vid_height", "600", true};


void VID_GetRenderDimensions (void)
{
	RECT cr;

	GetClientRect (vid.hWnd, &cr);

	vid.width = cr.right - cr.left;
	vid.height = cr.bottom - cr.top;
}


void VID_CenterWindow (HWND hWndCenter)
{
	RECT WorkArea;
	RECT WindowRect;

	GetWindowRect (hWndCenter, &WindowRect);
	SystemParametersInfo (SPI_GETWORKAREA, 0, &WorkArea, 0);

	SetWindowPos (
		hWndCenter,
		NULL,
		WorkArea.left + ((WorkArea.right - WorkArea.left) - (WindowRect.right - WindowRect.left)) / 2,
		WorkArea.top + ((WorkArea.bottom - WorkArea.top) - (WindowRect.bottom - WindowRect.top)) / 2,
		0,
		0,
		SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_DRAWFRAME
		);
}


qboolean VID_CreateWindow (int modenum, qboolean fullscreen)
{
	HDC				hdc;
	int				lastmodestate;
	RECT			WindowRect;
	HICON			hIcon = LoadIcon (global_hInstance, MAKEINTRESOURCE (IDI_ICON2));

	lastmodestate = modestate;

	WindowRect.top = WindowRect.left = 0;

	WindowRect.right = D_GetModeWidth (modenum);
	WindowRect.bottom = D_GetModeHeight (modenum);

	AdjustWindowRectEx (&WindowRect, WindowStyle, FALSE, ExWindowStyle);

	// Create the DIB window
	vid.hWnd = CreateWindowEx (
		ExWindowStyle,
		"DirectHII",
		"DirectHII",
		WindowStyle,
		WindowRect.left,
		WindowRect.top,
		WindowRect.right - WindowRect.left,
		WindowRect.bottom - WindowRect.top,
		NULL,
		NULL,
		global_hInstance,
		NULL
		);

	if (!vid.hWnd)
		Sys_Error ("Couldn't create window");

	if (fullscreen)
		modestate = MS_FULLSCREEN;
	else
	{
		VID_CenterWindow (vid.hWnd);
		modestate = MS_WINDOWED;
	}

	ShowWindow (vid.hWnd, SW_SHOWDEFAULT);
	UpdateWindow (vid.hWnd);

	// Because we have set the background brush for the window to NULL
	// (to avoid flickering when re-sizing the window on the desktop), we
	// clear the window to black when created, otherwise it will be
	// empty while Quake starts up.
	hdc = GetDC (vid.hWnd);
	PatBlt (hdc, 0, 0, WindowRect.right, WindowRect.bottom, BLACKNESS);
	ReleaseDC (vid.hWnd, hdc);

	VID_GetRenderDimensions ();

	SendMessage (vid.hWnd, WM_SETICON, (WPARAM) TRUE, (LPARAM) hIcon);
	SendMessage (vid.hWnd, WM_SETICON, (WPARAM) FALSE, (LPARAM) hIcon);

	return true;
}


int VID_SetMode (int modenum, unsigned char *palette)
{
	int				original_mode, temp;
	MSG				msg;

	// so Con_Printfs don't mess us up by forcing vid and snd updates
	temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;

	CDAudio_Pause ();
	MIDI_Pause ();

	if (vid_modenum == NO_MODE)
		original_mode = 0;
	else
		original_mode = vid_modenum;

	// Set either the fullscreen or windowed mode
	VID_CreateWindow (modenum, modenum > 0);

	// update cliprect for input
	IN_UpdateClipCursor ();

	CDAudio_Resume ();
	MIDI_Resume ();

	scr_disabled_for_loading = temp;

	// now we try to make sure we get the focus on the mode switch, because
	// sometimes in some systems we don't.  We grab the foreground, then
	// finish setting up, pump all our messages, and sleep for a little while
	// to let messages finish bouncing around the system, then we put
	// ourselves at the top of the z order, then grab the foreground again,
	// Who knows if it helps, but it probably doesn't hurt
	SetForegroundWindow (vid.hWnd);
	VID_SetPalette (palette);
	vid_modenum = modenum;
	Cvar_SetValue ("vid_mode", vid_modenum);

	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	Sleep (100);

	SetWindowPos (vid.hWnd, HWND_TOP, 0, 0, 0, 0,
		SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW |
		SWP_NOCOPYBITS);

	SetForegroundWindow (vid.hWnd);

	// fix the leftover Alt from any Alt-Tab or the like that switched us away
	Sys_ClearAllStates ();

	if (!msg_suppress_1)
		Con_Printf (PRINT_SAFE, va ("Video mode %s initialized\n", D_GetModeDescription (vid_modenum)));

	VID_SetPalette (palette);

	return true;
}


//====================================


/*
=================
VID_BeginRendering

=================
*/
qboolean VID_BeginRendering (void)
{
	// figure window dimensions
	VID_GetRenderDimensions ();

	if (scr_skipupdate)
		return false;

	D_BeginRendering ();

	return true;
}


void VID_EndRendering (void)
{
	if (!scr_skipupdate)
	{
		if (scr.timeRefresh)
			D_EndRendering (0);		// no vsync because timerefresh needs to run as fast as possible
		else if (loading_stage)
			D_EndRendering (0);		// loading progress meter is up; no vsync otherwise we'll only load 60 files per second!
		else if (cls.timedemo)
			D_EndRendering (0);		// no vsync because timedemos need to run as fast as possible
		else if (vid_vsync.value)
			D_EndRendering (1);		// obey the value of vid_vsync
		else D_EndRendering (0);
	}
}


int ColorIndex[16] = {
	0, 31, 47, 63, 79, 95, 111, 127, 143, 159, 175, 191, 199, 207, 223, 231
};

unsigned ColorPercent[16] = {
	25, 51, 76, 102, 114, 127, 140, 153, 165, 178, 191, 204, 216, 229, 237, 247
};


void VID_MakeCharTable (unsigned char *palette, unsigned *table)
{
	int i;

	for (i = 0; i < 256; i++, palette += 3)
	{
		int r = palette[0];
		int g = palette[1];
		int b = palette[2];

		table[i] = COLOR_FROM_RGBA (r, g, b, 255);
	}

	// 0 and 255 are transparent
	table[0] = table[255] = 0;
}


void VID_MakeRGBATable (unsigned char *palette, unsigned *table)
{
	int i;

	for (i = 0; i < 256; i++, palette += 3)
	{
		int r = palette[0];
		int g = palette[1];
		int b = palette[2];

		table[i] = COLOR_FROM_RGBA (r, g, b, 255);
	}

	// 255 is transparent
	table[255] = 0;
}


void VID_MakePartTable (unsigned char *palette, unsigned *table)
{
	int i, p;

	for (i = 0; i < 16; i++, table += 16)
	{
		int c = ColorIndex[i] * 3;

		int r = palette[c + 0];
		int g = palette[c + 1];
		int b = palette[c + 2];

		for (p = 0; p < 16; p++)
		{
			table[p] = COLOR_FROM_RGBA (r, g, b, ColorPercent[15 - p]);

			RTint[i * 16 + p] = ((float) r) / ((float) ColorPercent[15 - p]);
			GTint[i * 16 + p] = ((float) g) / ((float) ColorPercent[15 - p]);
			BTint[i * 16 + p] = ((float) b) / ((float) ColorPercent[15 - p]);
		}
	}
}


void VID_MakeHoleyTable (unsigned char *palette, unsigned *table)
{
	int i;

	for (i = 0; i < 256; i++, palette += 3)
	{
		int r = palette[0];
		int g = palette[1];
		int b = palette[2];

		table[i] = COLOR_FROM_RGBA (r, g, b, 255);
	}

	// 0 and 255 are transparent
	table[0] = table[255] = 0;
}


void VID_MakeTransTable (unsigned char *palette, unsigned *table)
{
	int i;

	for (i = 0; i < 256; i++, palette += 3)
	{
		int r = palette[0];
		int g = palette[1];
		int b = palette[2];

		if (i == 0)
			table[i] = 0;
		else if (i & 1)
			table[i] = COLOR_FROM_RGBA (r, g, b, 100);
		else table[i] = COLOR_FROM_RGBA (r, g, b, 255);
	}
}


void VID_MakeSpecTable (unsigned *basepalette, unsigned *table)
{
	int i;

	for (i = 0; i < 256; i++)
	{
		table[i] = basepalette[ColorIndex[i >> 4]] & 0x00ffffff;
		table[i] |= (int) ColorPercent[i & 15] << 24;
	}
}


void VID_SetPalette (unsigned char *palette)
{
	// Image_WriteDataToTGA ("palette.tga", palette, 16, 16, 24);

	// standard palettes
	VID_MakeCharTable (palette, d_8to24table_char);
	VID_MakeRGBATable (palette, d_8to24table_rgba);
	VID_MakePartTable (palette, d_8to24table_part);

	// special palettes
	VID_MakeHoleyTable (palette, d_8to24table_holey);
	VID_MakeTransTable (palette, d_8to24table_trans);
	VID_MakeSpecTable (d_8to24table_rgba, d_8to24table_spec);
}


void VID_RequestModeChange (void)
{
	// if a mode change is requested we just flag it, then at the start of the next frame we'll pick it up and actually do it
	vid_ModeChangeRequested = true;
}


void VID_Shutdown (void)
{
	if (vid_initialized)
	{
		D_DestroyD3DDevice ();
		Sys_AppActivate (false, false);
	}
}


/*
===================
VID_Init
===================
*/
void VID_Init (unsigned char *palette)
{
	int		i;
	int		width, height;
	int		vid_default = 0;
	qboolean windowed;

	Cvar_RegisterVariable (&vid_mode, VID_RequestModeChange);
	Cvar_RegisterVariable (&vid_vsync, NULL);
	Cvar_RegisterVariable (&vid_width, VID_RequestModeChange);
	Cvar_RegisterVariable (&vid_height, VID_RequestModeChange);

	Cmd_AddCommand ("vid_nummodes", VID_NumModes_f);
	Cmd_AddCommand ("vid_describecurrentmode", VID_DescribeCurrentMode_f);
	Cmd_AddCommand ("vid_describemode", VID_DescribeMode_f);
	Cmd_AddCommand ("vid_describemodes", VID_DescribeModes_f);

	// get the available video modes
	D_EnumerateVideoModes ();

	// pick one of them, based on cmdline args
	if ((i = COM_CheckParm ("-mode")) != 0)
		vid_default = atoi (com_argv[i + 1]);
	else
	{
		if (COM_CheckParm ("-current"))
		{
			width = 0;
			height = 0;
			windowed = false;
		}
		else
		{
			if (COM_CheckParm ("-window"))
				windowed = true;
			else windowed = false;

			if ((i = COM_CheckParm ("-width")) != 0)
				width = atoi (com_argv[i + 1]);
			else width = 0;

			if ((i = COM_CheckParm ("-height")) != 0)
				height = atoi (com_argv[i + 1]);
			else height = 0;
		}

		vid_default = D_FindVideoMode (width, height, windowed);
	}

	vid_initialized = true;

	vid.colormap = host_colormap;

	VID_SetMode (vid_default, palette);

	D_CreateD3DDevice (vid.hWnd, vid_modenum);

	VID_SetPalette (palette);
}


