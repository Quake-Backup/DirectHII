// screen.h

typedef struct scrdef_s {
	float ConLines;
	float ConCurrent;
	qboolean timeRefresh;
} scrdef_t;

extern scrdef_t scr;

void SCR_Init (void);

void SCR_UpdateScreen (void);

void SCR_CalcRefdef (void);

void SCR_SizeUp (void);
void SCR_SizeDown (void);
void SCR_CenterPrint (char *str);

void SCR_BeginLoadingPlaque (void);
void SCR_EndLoadingPlaque (void);

int SCR_ModalMessage (char *text);

extern	int			sb_lines;
extern	int			sb_lines2d;

extern	int			clearnotify;	// set to 0 whenever notify text is drawn
extern	qboolean	scr_disabled_for_loading;
extern	qboolean	scr_skipupdate;

extern	cvar_t		scr_viewsize;

extern cvar_t scr_viewsize;

extern int			total_loading_size, current_loading_size, entity_file_size, loading_stage;

void SCR_UpdateWholeScreen (void);

// use instead of host_frametime here for an unscaled timer
extern double scr_frametime;

