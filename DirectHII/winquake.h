// winquake.h: Win32-specific Quake header file

extern	HINSTANCE	global_hInstance;
extern	int			global_nCmdShow;

extern qboolean		ActiveApp, Minimized;

extern qboolean	Win32AtLeastV4, WinNT;

void IN_MouseEvent (int mstate);

extern qboolean	mouseinitialized;

extern HANDLE	hinput, houtput;

void IN_UpdateClipCursor (void);

void S_BlockSound (void);
void S_UnblockSound (void);

