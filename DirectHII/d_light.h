
// lightmaps
#define	LIGHTMAP_SIZE	256
#define	MAX_LIGHTMAPS	64

typedef struct lighttexel_s
{
	byte styles[4];
} lighttexel_t;

void D_CreateLightmapTexture (lighttexel_t *data[MAX_LIGHTMAPS], int numlightmaps);
void D_AnimateLight (int *styles);
void D_SetupDynamicLight (float *origin, float intensity);

