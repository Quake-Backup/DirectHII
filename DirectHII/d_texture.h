
// textures
/*
* mode:
* 0 - standard
* 1 - color 0 transparent, odd - translucent, even - full value
* 2 - color 0 transparent
* 3 - special (particle translucency table)
*/
#define TEX_RGBA8			(0 << 0) // all textures are created as RGBA format unless explicitly specified otherwise
#define TEX_MIPMAP			(1 << 0) // creates a full mipmap chain
#define TEX_ALPHA			(1 << 1) // standard alphs channel with alpha edge fix
#define TEX_SCRAP			(1 << 2) // texture is in the scrap
#define TEX_TRANSPARENT		(1 << 3) // special Hexen 2 transparency type
#define TEX_HOLEY			(1 << 4) // special Hexen 2 transparency type
#define TEX_SPECIAL_TRANS	(1 << 5) // special Hexen 2 transparency type
#define TEX_RENDERTARGET	(1 << 6) // also create a rendertarget view of the texture
#define TEX_CUBEMAP			(1 << 7) // creates a cubemap (not used by this Hexen 2 engine)
#define TEX_STAGING			(1 << 8) // creates the texture in the staging pool
#define TEX_R32F			(1 << 9) // single-channel 32-bit floating-point texture
#define TEX_NOISE			(1 << 10) // 2-channel 16-bit per channel signed-normalized format for use with noise
#define TEX_PLAYERTEXTURE	(1 << 11) // used for player colour translations

// if any of these are set the texture may be processed for having an alpha channel
#define TEX_ANYALPHA		(TEX_ALPHA | TEX_TRANSPARENT | TEX_HOLEY | TEX_SPECIAL_TRANS)

// textures marked disposable may be flushed on map changes
#define TEX_DISPOSABLE		(1 << 30)

// needs to be common
#define SCRAP_SIZE		256

int D_LoadTexture (char *identifier, int width, int height, int stride, byte *data, unsigned *palette, int flags);
void D_FillTexture8 (int texnum, byte *data, int width, int height, int stride, unsigned *palette, int flags);
void D_FillTexture32 (int texnum, unsigned *data, int width, int height, int stride, int flags);
int D_LoadScrapTexture (int width, int height, byte *data, unsigned *palette, int *scrap_x, int *scrap_y);

void D_LoadTextureData32 (int texnum, int level, int xofs, int yofs, int width, int height, void *data);
void D_LoadTextureData8 (int texnum, int level, int xofs, int yofs, int width, int height, byte *src, unsigned *palette);

void D_FreeUnusedTextures (void);

int D_FindTexture (char *identifier, int width, int height, int arraysize, int crc, int flags);
int D_GetTexture (char *identifier, int width, int height, int arraysize, int crc, int flags);

void D_ClearTextureBindings (void);
void D_SetTextureMode (char *mode, int anisotropy);
void D_PurgePlayerTextures (void);


