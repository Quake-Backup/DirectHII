/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// in_win.c -- windows 95 mouse and joystick code

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "d_video.h"

// needed so that we know we're in the keybindings menu and we need to activate the mouse
extern int bind_grab;

// mouse variables
cvar_t	freelook = {"freelook", "1", true};

qboolean	mouseactive;
qboolean	mouseinitialized;


int ri_MouseState = 0;
int ri_MouseOldState = 0;

int ri_DownButtons[] = {RI_MOUSE_BUTTON_1_DOWN, RI_MOUSE_BUTTON_2_DOWN, RI_MOUSE_BUTTON_3_DOWN, RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_5_DOWN};
int ri_UpButtons[] = {RI_MOUSE_BUTTON_1_UP, RI_MOUSE_BUTTON_2_UP, RI_MOUSE_BUTTON_3_UP, RI_MOUSE_BUTTON_4_UP, RI_MOUSE_BUTTON_5_UP};

#define MOUSE_BUTTONS	5


void Force_CenterView_f (void)
{
	cl.viewangles[0] = 0;
}


void IN_UpdateClipCursor (void)
{
	if (mouseinitialized && mouseactive)
	{
		RECT rect;

		// this hack works because the RECT layout mirrors 2 POINTs
		GetClientRect (vid.hWnd, &rect);
		ClientToScreen (vid.hWnd, (POINT *) &rect.left);
		ClientToScreen (vid.hWnd, (POINT *) &rect.right);
		ClipCursor (&rect);
	}
}


void IN_MouseEvent (int mstate)
{
	if (mouseactive)
	{
		int	i;

		// perform button actions
		for (i = 0; i < MOUSE_BUTTONS; i++)
		{
			if ((mstate & (1 << i)) && !(ri_MouseOldState & (1 << i))) Key_Event (K_MOUSE1 + i, true);
			if (!(mstate & (1 << i)) && (ri_MouseOldState & (1 << i))) Key_Event (K_MOUSE1 + i, false);
		}

		ri_MouseOldState = mstate;
	}
}


void IN_ReadRawInput (HRAWINPUT hRawInput)
{
	int i;
	RAWINPUT ri_Data;
	UINT ri_Size = sizeof (RAWINPUT);

	if (!mouseactive) return;
	if (!mouseinitialized) return;

	if (!GetRawInputData (hRawInput, RID_INPUT, &ri_Data, &ri_Size, sizeof (RAWINPUTHEADER))) return;

	if (ri_Data.header.dwType == RIM_TYPEMOUSE)
	{
		// detect wheel
		if (ri_Data.data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
		{
			if ((signed short) ri_Data.data.mouse.usButtonData > 0)
			{
				Key_Event (K_MWHEELUP, true);
				Key_Event (K_MWHEELUP, false);
			}
			else
			{
				Key_Event (K_MWHEELDOWN, true);
				Key_Event (K_MWHEELDOWN, false);
			}
		}

		// decode buttons
		for (i = 0; i < MOUSE_BUTTONS; i++)
		{
			if (ri_Data.data.mouse.usButtonFlags & ri_DownButtons[i]) ri_MouseState |= (1 << i);
			if (ri_Data.data.mouse.usButtonFlags & ri_UpButtons[i]) ri_MouseState &= ~(1 << i);
		}

		// read movement and add it to the move
		if ((in_mlook.state & 1) || freelook.value)
		{
			// compensate for different scaling
			if (ri_Data.data.mouse.lLastX) cl.viewangles[1] -= m_yaw.value * (float) ri_Data.data.mouse.lLastX * sensitivity.value * 6.666f;
			if (ri_Data.data.mouse.lLastY) cl.viewangles[0] += m_pitch.value * (float) ri_Data.data.mouse.lLastY * sensitivity.value * 6.666f;

			// clamp pitch
			if (cl.viewangles[0] > 80) cl.viewangles[0] = 80;
			if (cl.viewangles[0] < -70) cl.viewangles[0] = -70;
		}

		// run events (even if state is 0 as there may be previous state to be cleared)
		IN_MouseEvent (ri_MouseState);
	}
}


void IN_ClearStates (void)
{
	if (mouseactive)
	{
		// send a key up event for any mouse buttons that are currently down
		IN_MouseEvent (0);

		// and clear all of our states
		ri_MouseState = 0;
		ri_MouseOldState = 0;
	}
}


void IN_CaptureMouse (void)
{
	if (!mouseactive)
	{
		RAWINPUTDEVICE ri_Mouse = {1, 2, RIDEV_NOLEGACY, vid.hWnd};

		RegisterRawInputDevices (&ri_Mouse, 1, sizeof (RAWINPUTDEVICE));

		while (ShowCursor (FALSE) >= 0);

		mouseactive = true;

		IN_UpdateClipCursor ();
		IN_ClearStates ();
	}
}


void IN_Shutdown (void)
{
	if (mouseactive)
	{
		RAWINPUTDEVICE ri_Mouse = {1, 2, RIDEV_REMOVE, NULL};

		RegisterRawInputDevices (&ri_Mouse, 1, sizeof (RAWINPUTDEVICE));

		IN_ClearStates ();
		ClipCursor (NULL);

		while (ShowCursor (TRUE) < 0);

		// revert the time period
		//timeEndPeriod (1);

		mouseactive = false;
	}
}


void IN_Frame (void)
{
	// this is called every frame to update the mouse state
	if (!mouseinitialized) return;

	// release the mouse if it was active, capture the mouse if it wasn't active
	if (!ActiveApp || Minimized)
		IN_Shutdown ();
	else if (bind_grab || key_dest == key_game)
		IN_CaptureMouse ();
	else if (modestate == MS_WINDOWED)
		IN_Shutdown ();
	else IN_CaptureMouse ();
}


void IN_Init (void)
{
	// mouse variables
	Cvar_RegisterVariable (&freelook, NULL);

	Cmd_AddCommand ("force_centerview", Force_CenterView_f);

	if (COM_CheckParm ("-nomouse"))
		return;

	mouseinitialized = true;
	mouseactive = false;

	IN_CaptureMouse ();
}


byte        scantokey[128] =
{
	//  0           1       2       3       4       5       6       7
	//  8           9       A       B       C       D       E       F
	0, 27, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', '-', '=', K_BACKSPACE, 9, // 0
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
	'o', 'p', '[', ']', 13, K_CTRL, 'a', 's',     // 1
	'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
	'\'', '`', K_SHIFT, '\\', 'z', 'x', 'c', 'v',     // 2
	'b', 'n', 'm', ',', '.', '/', K_SHIFT, '*',
	K_ALT, ' ', 0, K_F1, K_F2, K_F3, K_F4, K_F5,  // 3
	K_F6, K_F7, K_F8, K_F9, K_F10, K_PAUSE, 0, K_HOME,
	K_UPARROW, K_PGUP, '-', K_LEFTARROW, '5', K_RIGHTARROW, '+', K_END, //4
	K_DOWNARROW, K_PGDN, K_INS, K_DEL, 0, 0, 0, K_F11,
	K_F12, 0, 0, 0, 0, 0, 0, 0,       // 5
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,        // 6
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0         // 7
};

byte        shiftscantokey[128] =
{
	//  0           1       2       3       4       5       6       7
	//  8           9       A       B       C       D       E       F
	0, 27, '!', '@', '#', '$', '%', '^',
	'&', '*', '(', ')', '_', '+', K_BACKSPACE, 9, // 0
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
	'O', 'P', '{', '}', 13, K_CTRL, 'A', 'S',     // 1
	'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
	'"', '~', K_SHIFT, '|', 'Z', 'X', 'C', 'V',     // 2
	'B', 'N', 'M', '<', '>', '?', K_SHIFT, '*',
	K_ALT, ' ', 0, K_F1, K_F2, K_F3, K_F4, K_F5,  // 3
	K_F6, K_F7, K_F8, K_F9, K_F10, 0, 0, K_HOME,
	K_UPARROW, K_PGUP, '_', K_LEFTARROW, '%', K_RIGHTARROW, '+', K_END, //4
	K_DOWNARROW, K_PGDN, K_INS, K_DEL, 0, 0, 0, K_F11,
	K_F12, 0, 0, 0, 0, 0, 0, 0,       // 5
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,        // 6
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0         // 7
};


/*
=======
IN_MapKey

Map from windows to quake keynums
=======
*/
int IN_MapKey (int key)
{
	key = (key >> 16) & 255;

	if (key > 127)
		return 0;

	return scantokey[key];
}

