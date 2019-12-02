// vid.h -- video driver defs

#define VID_CBITS	6
#define VID_GRADES	(1 << VID_CBITS)

typedef struct viddef_s
{
	// main window
	HWND			hWnd;

	// to do - get rid of this...
	byte			*colormap;		// 256 * VID_GRADES size

	// 3d view which should always be the same size as the refresh window
	int				width;
	int				height;

	// scaled 2d view
	int				width2d;
	int				height2d;
} viddef_t;

extern byte globalcolormap[VID_GRADES * 256];

extern	viddef_t	vid;				// global video state

extern	unsigned	d_8to24table_char[];
extern	unsigned	d_8to24table_rgba[];
extern	unsigned	d_8to24table_part[];

extern	unsigned	d_8to24table_holey[];
extern	unsigned	d_8to24table_trans[];
extern	unsigned	d_8to24table_spec[];


void	VID_SetPalette (unsigned char *palette);
// called at startup and after any gamma correction

void	VID_Init (unsigned char *palette);
// Called at startup to set up translation tables, takes 256 8 bit RGB values
// the palette data will go away after the call, so it must be copied off if
// the video driver will need it again

void	VID_Shutdown (void);
// Called at shutdown

int VID_SetMode (int modenum, unsigned char *palette);
// sets the mode; only used by the Quake engine for resetting to mode 0 (the
// base mode) on memory allocation failures

