// sys_win.c -- Win32 system interface code

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "errno.h"
#include "conproc.h"
#include "resource.h"

#include <direct.h>

void CL_RemoveGIPFiles (char *path);

#define CRC_A 59461 // "Who's Ridin' With Chaos?"
#define CRC_B 54866 // "Santa needs a new sled!"

#define CONSOLE_ERROR_TIMEOUT	60.0	// # of seconds to wait on Sys_Error running dedicated before exiting
#define PAUSE_SLEEP		50				// sleep time on pause or minimization
#define NOT_FOCUS_SLEEP	20				// sleep time when not focus

qboolean	ActiveApp, Minimized;
qboolean	Win32AtLeastV4, WinNT;

void Sys_InitFloatTime (void);

qboolean			isDedicated;
static qboolean		sc_return_on_enter = false;
HANDLE				hinput, houtput;

static char			*tracking_tag = "Sticky Buns";

static HANDLE	tevent;
static HANDLE	hFile;
static HANDLE	heventParent;
static HANDLE	heventChild;

cvar_t		sys_delay = {"sys_delay", "0", true};


/*
===============================================================================

FILE IO

===============================================================================
*/

#define	MAX_HANDLES		10
FILE	*sys_handles[MAX_HANDLES];

int		findhandle (void)
{
	int		i;

	for (i = 1; i < MAX_HANDLES; i++)
		if (!sys_handles[i])
			return i;

	Sys_Error ("out of handles");
	return -1;
}

/*
================
filelength
================
*/
int filelength (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
	FILE	*f;
	int		i, retval;

	i = findhandle ();
	f = fopen (path, "rb");

	if (!f)
	{
		*hndl = -1;
		retval = -1;
	}
	else
	{
		sys_handles[i] = f;
		*hndl = i;
		retval = filelength (f);
	}

	return retval;
}

int Sys_FileOpenWrite (char *path)
{
	FILE	*f;
	int		i;

	i = findhandle ();
	f = fopen (path, "wb");

	if (!f)
		Sys_Error ("Error opening %s: %s", path, strerror (errno));

	sys_handles[i] = f;

	return i;
}

void Sys_FileClose (int handle)
{
	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek (int handle, int position)
{
	fseek (sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead (int handle, void *dest, int count)
{
	return fread (dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite (int handle, void *data, int count)
{
	return fwrite (data, 1, count, sys_handles[handle]);
}

int	Sys_FileTime (char *path)
{
	FILE	*f;
	int		retval;

	f = fopen (path, "rb");

	if (f)
	{
		fclose (f);
		retval = 1;
	}
	else
	{
		retval = -1;
	}

	return retval;
}

void Sys_mkdir (char *path)
{
	_mkdir (path);
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
	DWORD  flOldProtect;

	if (!VirtualProtect ((LPVOID) startaddr, length, PAGE_READWRITE, &flOldProtect))
		Sys_Error ("Protection change failed\n");
}


void Sys_SetFPCW (void)
{
}

void Sys_PushFPCW_SetHigh (void)
{
}

void Sys_PopFPCW (void)
{
}

void MaskExceptions (void)
{
}


/*
================
Sys_Init
================
*/
void Sys_Init (void)
{
	OSVERSIONINFO	vinfo;

	Sys_InitFloatTime ();

	vinfo.dwOSVersionInfoSize = sizeof (vinfo);

	if (!GetVersionEx (&vinfo))
		Sys_Error ("Couldn't get OS info");

	if (vinfo.dwMajorVersion < 4)
		Win32AtLeastV4 = false;
	else
		Win32AtLeastV4 = true;

	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("Hexen2 requires at least Win95 or NT 4.0");

	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		WinNT = true;
	else WinNT = false;
}


void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024], text2[1024];
	char		*text3 = "Press Enter to exit\n";
	char		*text4 = "***********************************\n";
	char		*text5 = "\n";
	DWORD		dummy;
	double		starttime;

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	if (isDedicated)
	{
		va_start (argptr, error);
		vsprintf (text, error, argptr);
		va_end (argptr);

		sprintf (text2, "ERROR: %s\n", text);
		WriteFile (houtput, text5, strlen (text5), &dummy, NULL);
		WriteFile (houtput, text4, strlen (text4), &dummy, NULL);
		WriteFile (houtput, text2, strlen (text2), &dummy, NULL);
		WriteFile (houtput, text3, strlen (text3), &dummy, NULL);
		WriteFile (houtput, text4, strlen (text4), &dummy, NULL);


		starttime = Sys_DoubleTime ();
		sc_return_on_enter = true;	// so Enter will get us out of here

		while (!Sys_ConsoleInput () && ((Sys_DoubleTime () - starttime) < CONSOLE_ERROR_TIMEOUT))
		{
		}
	}
	else
	{
		// switch to windowed so the message box is visible
		MessageBox (NULL, text, "Hexen II Error", MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
	}

	Host_Shutdown ();

	// shut down QHOST hooks if necessary
	DeinitConProc ();

	//timeEndPeriod (1);
	exit (1);
}

void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	DWORD		dummy;

	if (isDedicated)
	{
		va_start (argptr, fmt);
		vsprintf (text, fmt, argptr);
		va_end (argptr);

		WriteFile (houtput, text, strlen (text), &dummy, NULL);
	}
}

void Sys_Quit (void)
{
	Host_Shutdown ();

	if (tevent)
		CloseHandle (tevent);

	if (isDedicated)
		FreeConsole ();

	// shut down QHOST hooks if necessary
	DeinitConProc ();

	//timeEndPeriod (1);
	exit (0);
}


/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime (void)
{
	__int64 qpcnow;

	static qboolean first = true;
	static __int64 qpcstart;
	static __int64 qpcfreq;

	if (first)
	{
		QueryPerformanceFrequency ((LARGE_INTEGER *) &qpcfreq);
		QueryPerformanceCounter ((LARGE_INTEGER *) &qpcstart);

		first = false;

		return 0;
	}

	QueryPerformanceCounter ((LARGE_INTEGER *) &qpcnow);

	return (double) (qpcnow - qpcstart) / (double) qpcfreq;
}


/*
================
Sys_InitFloatTime
================
*/
void Sys_InitFloatTime (void)
{
	Sys_DoubleTime ();
}


char *Sys_ConsoleInput (void)
{
	static char	text[256];
	static int		len;
	INPUT_RECORD	recs[1024];
	DWORD		dummy, numread, numevents;
	int		ch;

	if (!isDedicated)
		return NULL;

	for (;;)
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Sys_Error ("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput (hinput, recs, 1, &numread))
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
				case '\r':
					WriteFile (houtput, "\r\n", 2, &dummy, NULL);

					if (len)
					{
						text[len] = 0;
						len = 0;
						return text;
					}
					else if (sc_return_on_enter)
					{
						// special case to allow exiting from the error handler on Enter
						text[0] = '\r';
						len = 0;
						return text;
					}

					break;

				case '\b':
					WriteFile (houtput, "\b \b", 3, &dummy, NULL);

					if (len)
					{
						len--;
					}

					break;

				default:
					if (ch >= ' ')
					{
						WriteFile (houtput, &ch, 1, &dummy, NULL);
						text[len] = ch;
						len = (len + 1) & 0xff;
					}

					break;
				}
			}
		}
	}

	return NULL;
}

void Sys_Sleep (void)
{
	Sleep (1);
}


void Sys_SendKeyEvents (void)
{
	MSG        msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		// we always update if there are any event, even if we're paused
		scr_skipupdate = 0;

		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();

		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
}


/*
================
Sys_ClearAllStates
================
*/
void Sys_ClearAllStates (void)
{
	int		i;

	// send an up event for each key, to make sure the server clears them all
	for (i = 0; i < 256; i++)
		Key_Event (i, false);

	Key_ClearStates ();
	IN_ClearStates ();
}


void Sys_AppActivate (BOOL fActive, BOOL minimize)
/****************************************************************************
*
* Function:     Sys_AppActivate
* Parameters:   fActive - True if app is activating
*
* Description:  If the application is activating, then swap the system
*               into SYSPAL_NOSTATIC mode so that our palettes will display
*               correctly.
*
****************************************************************************/
{
	static BOOL	sound_active;

	ActiveApp = fActive;
	Minimized = minimize;

	// enable/disable sound on focus gain/loss
	if (!ActiveApp && sound_active)
	{
		S_BlockSound ();
		sound_active = false;
	}
	else if (ActiveApp && !sound_active)
	{
		S_UnblockSound ();
		sound_active = true;
	}
}


LONG CDAudio_MessageHandler (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LONG WINAPI MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int fActive, fMinimized;

	switch (uMsg)
	{
	case WM_MENUCHAR:
		// http://stackoverflow.com/questions/3662192/disable-messagebeep-on-invalid-syskeypress
		return (MNC_CLOSE << 16);

	case WM_INPUT:
		// ms-help://MS.MSDNQTR.v90.en/winui/winui/windowsuserinterface/userinput/rawinput/rawinputreference/rawinputmessages/wm_input.htm
		if ((int) wParam == RIM_INPUT) IN_ReadRawInput ((HRAWINPUT) lParam);
		DefWindowProc (hWnd, uMsg, wParam, lParam);
		return 0;

	case WM_ERASEBKGND: return 1; // treachery!!! see your MSDN!

	case WM_CREATE:
		// bring the Sys_AppActivate flags up to date
		Sys_AppActivate (true, false);
		return 0;

	case WM_SIZE:
	case WM_MOVE:
		IN_UpdateClipCursor ();
		return 0;

	case WM_SYSKEYDOWN:
		/*
		D3D9_REMOVED
		if ((int) wParam == 13)
		{
			D_HandleAltEnter (hWnd, WindowStyle, ExWindowStyle);
			vid.width = D_GetDisplayWidth ();
			vid.height = D_GetDisplayHeight ();
			return 0;
		}
		else if (IN_MapKey (lParam) == K_F4)
		{
			// trap and prevent alt-f4 which otherwise would just close the window/kill the app
			return 0;
		}
		*/

		// fall through
	case WM_KEYDOWN:
		Key_Event (IN_MapKey (lParam), true);
		return 0;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		Key_Event (IN_MapKey (lParam), false);
		return 0;

	case WM_SYSCHAR:
		// keep Alt-Space from happening
		return 0;

	case WM_CLOSE:
		if (MessageBox (vid.hWnd, "Are you sure you want to quit?", "Confirm Exit", MB_YESNO | MB_SETFOREGROUND | MB_ICONQUESTION) == IDYES)
		{
			Sys_Quit ();
		}

		return 0;

	case WM_ACTIVATE:
		fActive = LOWORD (wParam);
		fMinimized = (BOOL) HIWORD (wParam);
		Sys_AppActivate (!(fActive == WA_INACTIVE), fMinimized);

		// fix the leftover Alt from any Alt-Tab or the like that switched us away
		Sys_ClearAllStates ();
		return 0;

	case WM_DESTROY:
		if (vid.hWnd)
			DestroyWindow (vid.hWnd);

		PostQuitMessage (0);
		return 0;

	case MM_MCINOTIFY:
		return CDAudio_MessageHandler (hWnd, uMsg, wParam, lParam);

	default:
		// pass all unhandled messages to DefWindowProc
		return DefWindowProc (hWnd, uMsg, wParam, lParam);
	}
}


void Sys_RegisterWindowClass (void)
{
	WNDCLASS		wc;

	// Register the frame class
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = (WNDPROC) MainWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = GetModuleHandle (NULL);
	wc.hIcon         = 0;
	wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = "DirectHII";

	if (!RegisterClass (&wc))
		Sys_Error ("Couldn't register window class");
}


/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/


/*
==================
WinMain
==================
*/
void SleepUntilInput (int time)
{
	MsgWaitForMultipleObjects (1, &tevent, FALSE, time, QS_ALLINPUT);
}


/*
==================
WinMain
==================
*/
HINSTANCE	global_hInstance;
int			global_nCmdShow;
char		*argv[MAX_NUM_ARGVS];
static char	*empty_string = "";


// http://ntcoder.com/bab/tag/getuserprofiledirectory/
#include "userenv.h"
#pragma comment (lib, "userenv.lib")

char *Sys_GetUserHomeDir (void)
{
	static char szHomeDirBuf[MAX_PATH] = {0};

	// We need a process with query permission set
	HANDLE hToken = 0;
	DWORD BufSize = MAX_PATH;

	OpenProcessToken (GetCurrentProcess (), TOKEN_QUERY, &hToken);

	// Returns a path like C:/Documents and Settings/nibu if my user name is nibu
	GetUserProfileDirectory (hToken, szHomeDirBuf, &BufSize);

	// Close handle opened via OpenProcessToken
	CloseHandle (hToken);

	return szHomeDirBuf;
}


BOOL Sys_DirectoryExists (LPCTSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes (szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}


BOOL Sys_FileExists (LPCTSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes (szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}


void Sys_SetWorkingDirectory (void)
{
	int i, j, k;

	char *homeDir = Sys_GetUserHomeDir ();

	const char *testDirs[] = {
		// these are the locations I have Quake installed to on various PCs; you may want to change it yourself
		homeDir,
		"\\Desktop Crap",
		"\\Games",
		"",
		NULL
	};

	const char *qDirs[] = {
		"Hexen II",
		"HexenII",
		"Hexen2",
		"H II",
		"HII",
		"H2",
		NULL
	};

	const char *qFiles[] = {
		"pak0.pak",
		"progs.dat",
		"config.cfg",
		NULL
	};

	// try to find ID1 content in the test paths
	for (i = 0;; i++)
	{
		if (!testDirs[i]) break;
		if (!Sys_DirectoryExists (testDirs[i])) continue;

		for (j = 0;; j++)
		{
			if (!qDirs[j]) break;
			if (!Sys_DirectoryExists (va ("%s\\%s\\"GAMENAME, testDirs[i], qDirs[j]))) continue;

			for (k = 0;; k++)
			{
				if (!qFiles[k]) break;
				if (!Sys_FileExists (va ("%s\\%s\\"GAMENAME"\\%s", testDirs[i], qDirs[j], qFiles[k]))) continue;

				SetCurrentDirectory (va ("%s\\%s", testDirs[i], qDirs[j]));
				return;
			}
		}
	}
}


int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	quakeparms_t	parms;
	double			time, oldtime, newtime;
	MEMORYSTATUS	lpBuffer;
	static	char	cwd[1024];
	int				t;
	char			*realcmdline;

#ifdef _DEBUG
	static	char	debugcmdline[1024];

	strcpy (debugcmdline, "-width 1024 -height 600 -window");
	realcmdline = debugcmdline;

	// force a working directory for debug builds because the exe isn't going to be in the correct game path
	// as an alternative we could mklink the quake gamedirs to the linker output path as a post-build step
	Sys_SetWorkingDirectory ();
#else
	realcmdline = lpCmdLine;
#endif

	/* previous instances do not exist in Win32 */
	if (hPrevInstance)
		return 0;

	CL_RemoveGIPFiles (NULL);

	global_hInstance = hInstance;
	global_nCmdShow = nCmdShow;

	lpBuffer.dwLength = sizeof (MEMORYSTATUS);
	GlobalMemoryStatus (&lpBuffer);

	if (!GetCurrentDirectory (sizeof (cwd), cwd))
		Sys_Error ("Couldn't determine current directory");

	if (cwd[strlen (cwd)-1] == '/')
		cwd[strlen (cwd)-1] = 0;

	parms.basedir = cwd;
	parms.cachedir = NULL;

	parms.argc = 1;
	argv[0] = empty_string;

	while (*realcmdline && (parms.argc < MAX_NUM_ARGVS))
	{
		while (*realcmdline && ((*realcmdline <= 32) || (*realcmdline > 126)))
			realcmdline++;

		if (*realcmdline)
		{
			argv[parms.argc] = realcmdline;
			parms.argc++;

			while (*realcmdline && ((*realcmdline > 32) && (*realcmdline <= 126)))
				realcmdline++;

			if (*realcmdline)
			{
				*realcmdline = 0;
				realcmdline++;
			}
		}
	}

	parms.argv = argv;

	COM_InitArgv (parms.argc, parms.argv);

	parms.argc = com_argc;
	parms.argv = com_argv;

	isDedicated = (COM_CheckParm ("-dedicated") != 0);

	tevent = CreateEvent (NULL, FALSE, FALSE, NULL);

	if (!tevent)
		Sys_Error ("Couldn't create event");

	if (isDedicated)
	{
		if (!AllocConsole ())
		{
			Sys_Error ("Couldn't create dedicated server console");
		}

		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);

		// give QHOST a chance to hook into the console
		if ((t = COM_CheckParm ("-HFILE")) > 0)
		{
			if (t < com_argc)
				hFile = (HANDLE) atoi (com_argv[t + 1]);
		}

		if ((t = COM_CheckParm ("-HPARENT")) > 0)
		{
			if (t < com_argc)
				heventParent = (HANDLE) atoi (com_argv[t + 1]);
		}

		if ((t = COM_CheckParm ("-HCHILD")) > 0)
		{
			if (t < com_argc)
				heventChild = (HANDLE) atoi (com_argv[t + 1]);
		}

		InitConProc (hFile, heventParent, heventChild);
	}

	Sys_RegisterWindowClass ();
	Sys_Init ();

	// because sound is off until we become active
	S_BlockSound ();

	Sys_Printf ("Host_Init\n");
	Host_Init (&parms);

	oldtime = Sys_DoubleTime ();

	Cvar_RegisterVariable (&sys_delay, NULL);

	// make Sleep calls higher precision
	// timeBeginPeriod (1);

	/* main window message loop */
	while (1)
	{
		if (isDedicated)
		{
			newtime = Sys_DoubleTime ();
			time = newtime - oldtime;

			while (time < sys_ticrate.value)
			{
				Sys_Sleep ();
				newtime = Sys_DoubleTime ();
				time = newtime - oldtime;
			}
		}
		else
		{
			// yield the CPU for a little while when paused, minimized, or not the focus
			if ((cl.paused && !ActiveApp) || Minimized)
			{
				SleepUntilInput (PAUSE_SLEEP);
				scr_skipupdate = 1;		// no point in bothering to draw
			}
			else if (!ActiveApp)
			{
				SleepUntilInput (NOT_FOCUS_SLEEP);
				scr_skipupdate = 1;		// no point in bothering to draw
			}

			newtime = Sys_DoubleTime ();
			time = newtime - oldtime;
		}

		if (sys_delay.value > 0)
			Sleep (sys_delay.value);

		Host_Frame (time);
		oldtime = newtime;
	}

	/* return success of application */
	return TRUE;
}

