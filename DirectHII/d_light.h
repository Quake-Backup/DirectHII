
// lightmaps
#define	LIGHTMAP_SIZE	256
#define	MAX_LIGHTMAPS	64

void D_CreateLightmapTexture (void *data[MAX_LIGHTMAPS], int numlightmaps);
void D_AnimateLight (int *styles);
void D_SetupDynamicLight (float *origin, float intensity);

