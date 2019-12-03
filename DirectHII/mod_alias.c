// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "d_texture.h"

extern qboolean	vid_initialized;

extern model_t	*loadmodel;
extern char	loadname[32];	// for hunk tags


/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

static trivertx_t	*poseverts[MAXALIASFRAMES];
byte	*player_8bit_texels[NUM_CLASSES];
static vec3_t	r_alias_mins, r_alias_maxs;

/*
=================
Mod_LoadAliasFrame
=================
*/
void *Mod_LoadAliasFrame (aliashdr_t *pheader, void *pin, maliasframedesc_t *frame)
{
	trivertx_t		*pinframe;
	int				i, j;
	daliasframe_t	*pdaliasframe;

	pdaliasframe = (daliasframe_t *) pin;

	strcpy (frame->name, pdaliasframe->name);
	frame->firstpose = pheader->numposes;
	frame->numposes = 1;

	for (i = 0; i < 3; i++)
	{
		// these are byte values, so we don't have to worry about
		// endianness
		frame->bboxmin.v[i] = pdaliasframe->bboxmin.v[i];
		frame->bboxmax.v[i] = pdaliasframe->bboxmax.v[i];
	}

	pinframe = (trivertx_t *) (pdaliasframe + 1);

	for (j = 0; j < pheader->numverts; j++)
	{
		for (i = 0; i < 3; i++)
		{
			if (r_alias_mins[i] > pinframe[j].v[i]) r_alias_mins[i] = pinframe[j].v[i];
			if (r_alias_maxs[i] < pinframe[j].v[i]) r_alias_maxs[i] = pinframe[j].v[i];
		}
	}

	poseverts[pheader->numposes] = pinframe;
	pheader->numposes++;

	pinframe += pheader->numverts;

	return (void *) pinframe;
}


/*
=================
Mod_LoadAliasGroup
=================
*/
void *Mod_LoadAliasGroup (aliashdr_t *pheader, void *pin, maliasframedesc_t *frame)
{
	daliasgroup_t		*pingroup;
	int					i, j, k, numframes;
	daliasinterval_t	*pin_intervals;
	void				*ptemp;

	pingroup = (daliasgroup_t *) pin;

	numframes = LittleLong (pingroup->numframes);

	frame->firstpose = pheader->numposes;
	frame->numposes = numframes;

	for (i = 0; i < 3; i++)
	{
		// these are byte values, so we don't have to worry about endianness
		frame->bboxmin.v[i] = pingroup->bboxmin.v[i];
		frame->bboxmax.v[i] = pingroup->bboxmax.v[i];
	}

	pin_intervals = (daliasinterval_t *) (pingroup + 1);

	frame->interval = LittleFloat (pin_intervals->interval);

	pin_intervals += numframes;

	ptemp = (void *) pin_intervals;

	for (i = 0; i < numframes; i++)
	{
		poseverts[pheader->numposes] = (trivertx_t *) ((daliasframe_t *) ptemp + 1);

		for (j = 0; j < pheader->numverts; j++)
		{
			for (k = 0; k < 3; k++)
			{
				if (r_alias_mins[k] > poseverts[pheader->numposes][j].v[k]) r_alias_mins[k] = poseverts[pheader->numposes][j].v[k];
				if (r_alias_maxs[k] < poseverts[pheader->numposes][j].v[k]) r_alias_maxs[k] = poseverts[pheader->numposes][j].v[k];
			}
		}

		pheader->numposes++;
		ptemp = (trivertx_t *) ((daliasframe_t *) ptemp + 1) + pheader->numverts;
	}

	return ptemp;
}

//=========================================================

/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes - Ed
=================
*/

typedef struct floodfill_s {
	short		x, y;
} floodfill_t;

// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
	if (pos[off] == fillcolor) \
		{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
		} \
		else if (pos[off] != 255) fdc = pos[off]; \
}

void Mod_FloodFillSkin (byte *skin, int skinwidth, int skinheight)
{
	byte				fillcolor = *skin; // assume this is the pixel to fill
	floodfill_t			fifo[FLOODFILL_FIFO_SIZE];
	int					inpt = 0, outpt = 0;
	int					filledcolor = -1;
	int					i;

	if (filledcolor == -1)
	{
		filledcolor = 0;

		// attempt to find opaque black
		for (i = 0; i < 256; ++i)
		{
			if (d_8to24table_rgba[i] == (255 << 0)) // alpha 1.0
			{
				filledcolor = i;
				break;
			}
		}
	}

	// can't fill to filled color or to transparent color (used as visited marker)
	if ((fillcolor == filledcolor) || (fillcolor == 255))
	{
		//printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
		return;
	}

	fifo[inpt].x = 0, fifo[inpt].y = 0;
	inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

	while (outpt != inpt)
	{
		int			x = fifo[outpt].x, y = fifo[outpt].y;
		int			fdc = filledcolor;
		byte		*pos = &skin[x + skinwidth * y];

		outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

		if (x > 0)				FLOODFILL_STEP (-1, -1, 0);
		if (x < skinwidth - 1)	FLOODFILL_STEP (1, 1, 0);
		if (y > 0)				FLOODFILL_STEP (-skinwidth, 0, -1);
		if (y < skinheight - 1)	FLOODFILL_STEP (skinwidth, 0, 1);

		skin[x + skinwidth * y] = fdc;
	}
}

/*
===============
Mod_LoadAllSkins
===============
*/
void *Mod_LoadAllSkins (aliashdr_t *pheader, int numskins, daliasskintype_t *pskintype, int mdl_flags)
{
	int		i;
	char	name[32];
	int		s;
	byte	*skin;

	skin = (byte *) (pskintype + 1);

	if (numskins < 1 || numskins > MAX_SKINS)
		Sys_Error ("Mod_LoadAliasModel: Invalid # of skins: %d\n", numskins);

	for (i = 0; i < numskins; i++)
	{
		Mod_FloodFillSkin (skin, pheader->skinwidth, pheader->skinheight);

		s = pheader->skinwidth * pheader->skinheight;

		// save 8 bit texels for the player model to remap
		if (!strcmp (loadmodel->name, "models/paladin.mdl"))
		{
			player_8bit_texels[0] = (byte *) Hunk_Alloc (GAME_HUNK, s);
			memcpy (player_8bit_texels[0], (byte *) (pskintype + 1), s);
		}
		else if (!strcmp (loadmodel->name, "models/crusader.mdl"))
		{
			player_8bit_texels[1] = (byte *) Hunk_Alloc (GAME_HUNK, s);
			memcpy (player_8bit_texels[1], (byte *) (pskintype + 1), s);
		}
		else if (!strcmp (loadmodel->name, "models/necro.mdl"))
		{
			player_8bit_texels[2] = (byte *) Hunk_Alloc (GAME_HUNK, s);
			memcpy (player_8bit_texels[2], (byte *) (pskintype + 1), s);
		}
		else if (!strcmp (loadmodel->name, "models/assassin.mdl"))
		{
			player_8bit_texels[3] = (byte *) Hunk_Alloc (GAME_HUNK, s);
			memcpy (player_8bit_texels[3], (byte *) (pskintype + 1), s);
		}

		sprintf (name, "%s_%i", loadmodel->name, i);

		if (mdl_flags & EF_HOLEY)
			pheader->texnum[i] = D_LoadTexture (name, pheader->skinwidth, pheader->skinheight, pheader->skinwidth, (byte *) (pskintype + 1), d_8to24table_holey, TEX_MIPMAP | TEX_HOLEY);
		else if (mdl_flags & EF_TRANSPARENT)
			pheader->texnum[i] = D_LoadTexture (name, pheader->skinwidth, pheader->skinheight, pheader->skinwidth, (byte *) (pskintype + 1), d_8to24table_trans, TEX_MIPMAP | TEX_TRANSPARENT);
		else if (mdl_flags & EF_SPECIAL_TRANS)
			pheader->texnum[i] = D_LoadTexture (name, pheader->skinwidth, pheader->skinheight, pheader->skinwidth, (byte *) (pskintype + 1), d_8to24table_spec, TEX_MIPMAP | TEX_SPECIAL_TRANS);
		else pheader->texnum[i] = D_LoadTexture (name, pheader->skinwidth, pheader->skinheight, pheader->skinwidth, (byte *) (pskintype + 1), d_8to24table_rgba, TEX_MIPMAP);

		pskintype = (daliasskintype_t *) ((byte *) (pskintype + 1) + s);
	}

	return (void *) pskintype;
}

//=========================================================================

void R_MakeAliasModelDisplayListsRAPO (model_t *m, aliashdr_t *hdr, trivertx_t *xyzverts[MAXALIASFRAMES], stvert_t *stverts, dnewtriangle_t *triangles);
void R_MakeAliasModelDisplayListsIDPO (model_t *m, aliashdr_t *hdr, trivertx_t *xyzverts[MAXALIASFRAMES], stvert_t *stverts, dtriangle_t *triangles);


/*
=================
Mod_LoadAliasModelNew
reads extra field for num ST verts, and extra index list of them
=================
*/
void Mod_LoadAliasModelNew (model_t *mod, void *buffer)
{
	int					i, j;
	newmdl_t			*pinmodel;
	stvert_t			*pinstverts;
	dnewtriangle_t		*pintriangles;
	int					version, numframes;
	int					size;
	daliasframetype_t	*pframetype;
	daliasskintype_t	*pskintype;
	aliashdr_t			*pheader;

	pinmodel = (newmdl_t *) buffer;
	version = LittleLong (pinmodel->version);

	if (version != ALIAS_NEWVERSION)
		Sys_Error ("%s has wrong version number (%i should be %i)", mod->name, version, ALIAS_NEWVERSION);

	//	Con_Printf("Loading NEW model %s\n",mod->name);
	// allocate space for a working header, plus all the data except the frames,
	// skin and group info
	size = sizeof (aliashdr_t) + (LittleLong (pinmodel->numframes) - 1) * sizeof (pheader->frames[0]);
	pheader = Hunk_Alloc (MAP_HUNK, size);

	mod->flags = LittleLong (pinmodel->flags);

	// endian-adjust and copy the data, starting with the alias model header
	pheader->boundingradius = LittleFloat (pinmodel->boundingradius);
	pheader->numskins = LittleLong (pinmodel->numskins);
	pheader->skinwidth = LittleLong (pinmodel->skinwidth);
	pheader->skinheight = LittleLong (pinmodel->skinheight);

	if (pheader->skinheight > MAX_SKIN_HEIGHT)
		Sys_Error ("model %s has a skin taller than %d", mod->name, MAX_SKIN_HEIGHT);

	pheader->numverts = LittleLong (pinmodel->numverts);
	pheader->version = LittleLong (pinmodel->num_st_verts);	//hide num_st in version

	if (pheader->numverts <= 0)
		Sys_Error ("model %s has no vertices", mod->name);

	if (pheader->numverts > MAXALIASVERTS)
		Sys_Error ("model %s has too many vertices", mod->name);

	pheader->numtris = LittleLong (pinmodel->numtris);

	if (pheader->numtris <= 0)
		Sys_Error ("model %s has no triangles", mod->name);

	pheader->numframes = LittleLong (pinmodel->numframes);
	numframes = pheader->numframes;

	if (numframes < 1)
		Sys_Error ("Mod_LoadAliasModel: Invalid # of frames: %d\n", numframes);

	pheader->size = LittleFloat (pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
	mod->synctype = LittleLong (pinmodel->synctype);
	mod->numframes = pheader->numframes;

	for (i = 0; i < 3; i++)
	{
		pheader->scale[i] = LittleFloat (pinmodel->scale[i]);
		pheader->scale_origin[i] = LittleFloat (pinmodel->scale_origin[i]);
		pheader->eyeposition[i] = LittleFloat (pinmodel->eyeposition[i]);
	}

	// load the skins
	pskintype = (daliasskintype_t *) &pinmodel[1];
	pskintype = Mod_LoadAllSkins (pheader, pheader->numskins, pskintype, mod->flags);

	// load base s and t vertices
	pinstverts = (stvert_t *) pskintype;

	for (i = 0; i < pheader->version; i++)	//version holds num_st_verts
	{
		pinstverts[i].onseam = LittleLong (pinstverts[i].onseam);
		pinstverts[i].s = LittleLong (pinstverts[i].s);
		pinstverts[i].t = LittleLong (pinstverts[i].t);
	}

	// load triangle lists
	pintriangles = (dnewtriangle_t *) &pinstverts[pheader->version];

	for (i = 0; i < pheader->numtris; i++)
	{
		pintriangles[i].facesfront = LittleLong (pintriangles[i].facesfront);

		for (j = 0; j < 3; j++)
		{
			pintriangles[i].vertindex[j] = LittleShort (pintriangles[i].vertindex[j]);
			pintriangles[i].stindex[j] = LittleShort (pintriangles[i].stindex[j]);
		}
	}

	// load the frames
	pheader->numposes = 0;
	pframetype = (daliasframetype_t *) &pintriangles[pheader->numtris];

	r_alias_mins[0] = r_alias_mins[1] = r_alias_mins[2] = 32768;
	r_alias_maxs[0] = r_alias_maxs[1] = r_alias_maxs[2] = -32768;

	for (i = 0; i < numframes; i++)
	{
		aliasframetype_t	frametype;

		frametype = LittleLong (pframetype->type);

		if (frametype == ALIAS_SINGLE)
			pframetype = (daliasframetype_t *) Mod_LoadAliasFrame (pheader, pframetype + 1, &pheader->frames[i]);
		else pframetype = (daliasframetype_t *) Mod_LoadAliasGroup (pheader, pframetype + 1, &pheader->frames[i]);
	}

	//Con_Printf("Model is %s\n",mod->name);
	//Con_Printf("   Mins is %5.2f, %5.2f, %5.2f\n",mins[0],mins[1],mins[2]);
	//Con_Printf("   Maxs is %5.2f, %5.2f, %5.2f\n",maxs[0],maxs[1],maxs[2]);

	mod->type = mod_alias;

	// FIXME: do this right
	//	mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
	//	mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;
	mod->mins[0] = (r_alias_mins[0] * pheader->scale[0] + pheader->scale_origin[0]) - 10;
	mod->mins[1] = (r_alias_mins[1] * pheader->scale[1] + pheader->scale_origin[1]) - 10;
	mod->mins[2] = (r_alias_mins[2] * pheader->scale[2] + pheader->scale_origin[2]) - 10;
	mod->maxs[0] = (r_alias_maxs[0] * pheader->scale[0] + pheader->scale_origin[0]) + 10;
	mod->maxs[1] = (r_alias_maxs[1] * pheader->scale[1] + pheader->scale_origin[1]) + 10;
	mod->maxs[2] = (r_alias_maxs[2] * pheader->scale[2] + pheader->scale_origin[2]) + 10;

	// build the draw lists
	if (vid_initialized)
		R_MakeAliasModelDisplayListsRAPO (mod, pheader, poseverts, pinstverts, pintriangles);

	mod->aliashdr = pheader;
}

/*
=================
Mod_LoadAliasModel
=================
*/
void Mod_LoadAliasModel (model_t *mod, void *buffer)
{
	int					i;
	mdl_t				*pinmodel;
	stvert_t			*pinstverts;
	dtriangle_t			*pintriangles;
	int					version, numframes;
	int					size;
	daliasframetype_t	*pframetype;
	daliasskintype_t	*pskintype;
	aliashdr_t			*pheader;

	pinmodel = (mdl_t *) buffer;
	version = LittleLong (pinmodel->version);

	if (version != ALIAS_VERSION)
		Sys_Error ("%s has wrong version number (%i should be %i)", mod->name, version, ALIAS_VERSION);

	// allocate space for a working header, plus all the data except the frames,
	// skin and group info
	size = sizeof (aliashdr_t) + (LittleLong (pinmodel->numframes) - 1) * sizeof (pheader->frames[0]);
	pheader = Hunk_Alloc (MAP_HUNK, size);

	mod->flags = LittleLong (pinmodel->flags);

	// endian-adjust and copy the data, starting with the alias model header
	pheader->boundingradius = LittleFloat (pinmodel->boundingradius);
	pheader->numskins = LittleLong (pinmodel->numskins);
	pheader->skinwidth = LittleLong (pinmodel->skinwidth);
	pheader->skinheight = LittleLong (pinmodel->skinheight);

	if (pheader->skinheight > MAX_SKIN_HEIGHT)
		Sys_Error ("model %s has a skin taller than %d", mod->name, MAX_SKIN_HEIGHT);

	pheader->numverts = LittleLong (pinmodel->numverts);
	pheader->version = pheader->numverts;	//hide num_st in version

	if (pheader->numverts <= 0)
		Sys_Error ("model %s has no vertices", mod->name);

	if (pheader->numverts > MAXALIASVERTS)
		Sys_Error ("model %s has too many vertices", mod->name);

	pheader->numtris = LittleLong (pinmodel->numtris);

	if (pheader->numtris <= 0)
		Sys_Error ("model %s has no triangles", mod->name);

	pheader->numframes = LittleLong (pinmodel->numframes);
	numframes = pheader->numframes;

	if (numframes < 1)
		Sys_Error ("Mod_LoadAliasModel: Invalid # of frames: %d\n", numframes);

	pheader->size = LittleFloat (pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
	mod->synctype = LittleLong (pinmodel->synctype);
	mod->numframes = pheader->numframes;

	for (i = 0; i < 3; i++)
	{
		pheader->scale[i] = LittleFloat (pinmodel->scale[i]);
		pheader->scale_origin[i] = LittleFloat (pinmodel->scale_origin[i]);
		pheader->eyeposition[i] = LittleFloat (pinmodel->eyeposition[i]);
	}

	// load the skins
	pskintype = (daliasskintype_t *) &pinmodel[1];
	pskintype = Mod_LoadAllSkins (pheader, pheader->numskins, pskintype, mod->flags);

	// load base s and t vertices
	// version holds num_st_verts (this is yucky)
	pinstverts = (stvert_t *) pskintype;

	for (i = 0; i < pheader->version; i++)	// version holds num_st_verts
	{
		pinstverts[i].onseam = LittleLong (pinstverts[i].onseam);
		pinstverts[i].s = LittleLong (pinstverts[i].s);
		pinstverts[i].t = LittleLong (pinstverts[i].t);
	}

	// load triangle lists
	pintriangles = (dtriangle_t *) &pinstverts[pheader->numverts];

	for (i = 0; i < pheader->numtris; i++)
	{
		pintriangles[i].facesfront = LittleLong (pintriangles[i].facesfront);
		pintriangles[i].vertindex[0] = (unsigned short) LittleLong (pintriangles[i].vertindex[0]);
		pintriangles[i].vertindex[1] = (unsigned short) LittleLong (pintriangles[i].vertindex[1]);
		pintriangles[i].vertindex[2] = (unsigned short) LittleLong (pintriangles[i].vertindex[2]);
	}

	// load the frames
	pheader->numposes = 0;
	pframetype = (daliasframetype_t *) &pintriangles[pheader->numtris];

	r_alias_mins[0] = r_alias_mins[1] = r_alias_mins[2] = 32768;
	r_alias_maxs[0] = r_alias_maxs[1] = r_alias_maxs[2] = -32768;

	for (i = 0; i < numframes; i++)
	{
		aliasframetype_t	frametype;

		frametype = LittleLong (pframetype->type);

		if (frametype == ALIAS_SINGLE)
			pframetype = (daliasframetype_t *) Mod_LoadAliasFrame (pheader, pframetype + 1, &pheader->frames[i]);
		else pframetype = (daliasframetype_t *) Mod_LoadAliasGroup (pheader, pframetype + 1, &pheader->frames[i]);
	}

	//Con_Printf("Model is %s\n",mod->name);
	//Con_Printf("   Mins is %5.2f, %5.2f, %5.2f\n",mins[0],mins[1],mins[2]);
	//Con_Printf("   Maxs is %5.2f, %5.2f, %5.2f\n",maxs[0],maxs[1],maxs[2]);

	mod->type = mod_alias;

	// FIXME: do this right
	//	mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
	//	mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;
	mod->mins[0] = (r_alias_mins[0] * pheader->scale[0] + pheader->scale_origin[0]) - 10;
	mod->mins[1] = (r_alias_mins[1] * pheader->scale[1] + pheader->scale_origin[1]) - 10;
	mod->mins[2] = (r_alias_mins[2] * pheader->scale[2] + pheader->scale_origin[2]) - 10;
	mod->maxs[0] = (r_alias_maxs[0] * pheader->scale[0] + pheader->scale_origin[0]) + 10;
	mod->maxs[1] = (r_alias_maxs[1] * pheader->scale[1] + pheader->scale_origin[1]) + 10;
	mod->maxs[2] = (r_alias_maxs[2] * pheader->scale[2] + pheader->scale_origin[2]) + 10;

	// build the draw lists
	if (vid_initialized)
		R_MakeAliasModelDisplayListsIDPO (mod, pheader, poseverts, pinstverts, pintriangles);

	mod->aliashdr = pheader;
}

