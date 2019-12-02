// r_misc.c

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "d_main.h"
#include "d_texture.h"
#include "d_mesh.h"

void R_InitParticles (void);
void R_ClearParticles (void);

byte *playerTranslation;

typedef struct playertexture_s
{
	int classnum;
	int color;
	int texnum;
} playertexture_t;


playertexture_t r_playertextures[MAX_SCOREBOARD];


playertexture_t *R_FindPlayerTexture (int classnum, int color)
{
	int i;

	for (i = 0; i < MAX_SCOREBOARD; i++)
	{
		if (!r_playertextures[i].texnum) continue;
		if (r_playertextures[i].classnum != classnum) continue;
		if (r_playertextures[i].color != color) continue;

		return &r_playertextures[i];
	}

	return NULL;
}


playertexture_t *R_GetPlayerTexture (int classnum, int color)
{
	int i;

	for (i = 0; i < MAX_SCOREBOARD; i++)
	{
		if (r_playertextures[i].texnum) continue;

		r_playertextures[i].classnum = classnum;
		r_playertextures[i].color = color;

		return &r_playertextures[i];
	}

	return NULL;
}


void R_PurgePlayerTextures (void)
{
	D_PurgePlayerTextures ();
	memset (r_playertextures, 0, sizeof (r_playertextures));
}


/*
==================
R_InitTextures
==================
*/
void R_InitNoTexture (void)
{
	byte dest[4] = {0, 0xff, 0xff, 0};

	memset (&r_notexture_mip, 0, sizeof (r_notexture_mip));
	r_notexture_mip.width = r_notexture_mip.height = 2;
	r_notexture_mip.texnum = D_LoadTexture ("", 2, 2, 2, dest, d_8to24table_rgba, TEX_MIPMAP);
}


/*
===============
R_Init
===============
*/
void R_Init (void)
{
	extern byte *hunk_base;
	int counter;

	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);
	Cmd_AddCommand ("pointfile", R_ReadPointFile_f);

	Cvar_RegisterVariable (&r_clearcolor, NULL);
	Cvar_RegisterVariable (&r_waterwarp, NULL);
	Cvar_RegisterVariable (&r_norefresh, NULL);
	Cvar_RegisterVariable (&r_lightmap, NULL);
	Cvar_RegisterVariable (&r_fullbright, NULL);
	Cvar_RegisterVariable (&r_drawentities, NULL);
	Cvar_RegisterVariable (&r_drawviewmodel, NULL);
	Cvar_RegisterVariable (&r_novis, NULL);
	Cvar_RegisterVariable (&r_wholeframe, NULL);

	Cvar_RegisterVariable (&gl_clear, NULL);
	Cvar_RegisterVariable (&gl_polyblend, NULL);

	R_InitNoTexture ();
	R_InitParticles ();

	VCache_Init ();

	for (counter = 0; counter < MAX_EXTRA_TEXTURES; counter++)
		gl_extra_textures[counter] = 0;

	playerTranslation = (byte *) FS_LoadFile (GAME_HUNK, "gfx/player.lmp");

	if (!playerTranslation)
		Sys_Error ("Couldn't load gfx/player.lmp");
}

extern int color_offsets[NUM_CLASSES];

void R_BuildTranslationTable (int playerclass, int color, unsigned *basepalette, unsigned *translated)
{
	int		i;
	byte	*sourceA, *sourceB, *colorA, *colorB;

	int top = (color & 0xf0) >> 4;
	int bottom = (color & 15);

	if (top > 10) top = 0;
	if (bottom > 10) bottom = 0;

	top -= 1;
	bottom -= 1;

	// is this crap just correcting the backwards ranges????
	colorA = playerTranslation + 256 + color_offsets[playerclass];
	colorB = colorA + 256;
	sourceA = colorB + 256 + (top * 256);
	sourceB = colorB + 256 + (bottom * 256);

	// build the translated palette
	for (i = 0; i < 256; i++)
	{
		int translate = i;

		if (top >= 0 && (colorA[i] != 255)) translate = sourceA[i];
		if (bottom >= 0 && (colorB[i] != 255)) translate = sourceB[i];

		translated[i] = basepalette[translate];
	}
}


/*
===============
R_TranslatePlayerSkin

Translates a skin texture by the per-player color lookup
===============
*/
int R_TranslatePlayerSkin (int playernum)
{
	extern byte *player_8bit_texels[NUM_CLASSES];
	int	playerclass = (int) cl.scores[playernum].playerclass - 1;
	int color = cl.scores[playernum].colors & 0xff;
	playertexture_t *pt = NULL;

	// bound class
	if (!(playerclass >= 0 && playerclass < NUM_CLASSES)) playerclass = 0;

	// already loaded
	if ((pt = R_FindPlayerTexture (playerclass, color)) != NULL) return pt->texnum;

	// this model not loaded (it's an error elsewhere in the code if this ever happens)
	if (!player_8bit_texels[playerclass]) return 0;

	// pick the first free playertexture_t
	if ((pt = R_GetPlayerTexture (playerclass, color)) == NULL)
	{
		// none free; just purge them all and try again
		R_PurgePlayerTextures ();
		return R_TranslatePlayerSkin (playernum);
	}
	else
	{
		// locate the original skin pixels
		// ("player doesn't have a model yet" can no longer happen because translation only occurs immediately before drawing, so a model must exist to make it that far)
		entity_t *e = &cl_entities[1 + playernum];
		model_t *model = e->model;
		aliashdr_t *hdr = model->aliashdr;
		byte *original = player_8bit_texels[playerclass];
		unsigned translate32[256];

		// get a fresh texture
		pt->texnum = D_GetTexture (va ("player_%i_%i", playerclass, color), hdr->skinwidth, hdr->skinheight, 1, 0, TEX_MIPMAP | TEX_PLAYERTEXTURE);

		// build the translation table
		R_BuildTranslationTable (playerclass, color, d_8to24table_rgba, translate32);

		// and load the texture
		D_FillTexture8 (pt->texnum, original, hdr->skinwidth, hdr->skinheight, hdr->skinwidth, translate32, TEX_MIPMAP | TEX_PLAYERTEXTURE);

		// this is now a valid playertexture
		return pt->texnum;
	}
}


/*
===============
R_NewMap
===============
*/
void R_NewMap (void)
{
	int	i;

	// starting with a clean slate
	memset (&r_worldentity, 0, sizeof (r_worldentity));
	r_worldentity.model = cl.worldmodel;

	// clear out efrags in case the level hasn't been reloaded (FIXME: is this one short?)
	// note: i think these are always cleared even before here because maps do fully reload????
	for (i = 0; i < cl.worldmodel->numleafs; i++)
	{
		if (cl.worldmodel->leafs[i].efrags != NULL)
		{
			// Con_Printf (PRINT_SAFE, va ("leaf %i with non-NULL efrags\n", i));
			cl.worldmodel->leafs[i].efrags = NULL;
		}
	}

	r_viewleaf = NULL;
	R_ClearParticles ();

	// builds surfaces and lightmaps
	R_BuildSurfaces ();

	// free anything not used in this map
	D_FreeUnusedTextures ();
	D_FreeUnusedAliasBuffers ();

	// player textures are purged between maps
	R_PurgePlayerTextures ();

	// bring the view up to date
	V_NewMap ();
}


/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
void R_TimeRefresh_f (void)
{
	int			i;
	float		start, stop, time;
	float		startangle;
	float		timeRefreshTime = 1.8;

	D_SyncPipeline ();

	startangle = r_refdef.viewangles[1];
	start = Sys_DoubleTime ();
	scr.timeRefresh = true;
	SCR_CalcRefdef ();

	// do a 360 in 1.8 seconds
	for (i = 0; ; i++)
	{
		if (VID_BeginRendering ())
		{
			r_refdef.viewangles[1] = startangle + (Sys_DoubleTime () - start) * (360.0 / timeRefreshTime);

			while (r_refdef.viewangles[1] > 360.0)
				r_refdef.viewangles[1] -= 360.0;

			R_RenderView ();
			VID_EndRendering ();
		}

		if ((time = Sys_DoubleTime () - start) >= timeRefreshTime) break;
	}

	D_SyncPipeline ();

	stop = Sys_DoubleTime ();
	time = stop - start;
	scr.timeRefresh = false;
	r_refdef.viewangles[1] = startangle;

	Con_Printf (PRINT_NORMAL, va ("%i frames, %f seconds (%f fps)\n", i, time, (float) i / time));
}


