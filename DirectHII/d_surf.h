
#define MAX_SURF_INDEXES	0x100000

typedef struct brushpolyvert_s {
	float xyz[3];
	float st[2];
	float lm[2];
	float mapnum;
	byte styles[4];
} brushpolyvert_t;

void D_SurfCreateVertexBuffer (brushpolyvert_t *verts, int numvertexes);
void D_SurfBeginDrawingModel (qboolean alpha);
void D_SetLightmappedState (int texnum);
void D_SetAbsLightState (int texnum);

// sky
void D_LoadSkyTextures (byte *src, unsigned *palette);
void D_SetSkyState (void);

// water
void D_SetWaterState (int texnum);

// dynamic lights
void D_SetDynamicLightSurfState (int texnum);
void D_DrawSurfaceBatch (unsigned *indexes, int numindexes);

