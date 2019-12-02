
// crap shared by the renderer, the front-end and the menu system for video stuff
#define MAX_MODE_LIST	30
#define VID_ROW_SIZE	3

#define NO_MODE					0xffffffff

typedef enum { MS_WINDOWED, MS_FULLSCREEN, MS_UNINIT } modestate_t;

extern modestate_t	modestate;
extern int		vid_modenum;

int VID_NumModes (void);
char *D_GetModeDescription (int mode);


// video
void D_CreateD3DDevice (HWND hWnd, int modenum);
void D_HandleAltEnter (HWND hWnd, DWORD WindowStyle, DWORD ExWindowStyle);
void D_DestroyD3DDevice (void);
int D_GetDisplayWidth (void);
int D_GetDisplayHeight (void);
void D_SetViewport (int x, int y, int w, int h, float zn, float zf);

void D_EnumerateVideoModes (void);
int D_FindVideoMode (int width, int height, qboolean windowed);
int D_GetModeWidth (int modenum);
int D_GetModeHeight (int modenum);

void D_BeginRendering (void);
void D_EndRendering (int syncInterval);

qboolean D_BeginWaterWarp (void);
void D_DoWaterWarp (int width, int height);

void D_CaptureScreenshot (char *checkname, float gammaval);

void D_UpdateDrawConstants (int width, int height, float gammaval, float contrastval);

