
// console
extern int con_totallines;
extern int con_backscroll;
extern	qboolean con_forcedup;	// because no entities to refresh
extern qboolean con_initialized;
extern byte *con_chars;
extern	int	con_notifylines;		// scan lines to clear for notify lines

// printing...
#define PRINT_NORMAL		0
#define PRINT_DEVELOPER		1
#define PRINT_SAFE			2

void Con_CheckResize (void);
void Con_Init (void);
void Con_DrawConsole (int lines, qboolean drawinput);
void Con_Print (char *txt);
void Con_Printf (int mode, char *msg);
void Con_Clear_f (void);
void Con_DrawNotify (void);
void Con_ClearNotify (void);
void Con_ToggleConsole_f (void);

