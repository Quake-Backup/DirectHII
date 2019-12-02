// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

extern qboolean	vid_initialized;

model_t	*loadmodel;
char	loadname[32];	// for hunk tags

void Mod_LoadSpriteModel (model_t *mod, void *buffer);
void Mod_LoadBrushModel (model_t *mod, void *buffer);
void Mod_LoadAliasModel (model_t *mod, void *buffer);
void Mod_LoadAliasModelNew (model_t *mod, void *buffer);

model_t *Mod_LoadModel (model_t *mod, qboolean crash);

byte	mod_novis[MAX_MAP_LEAFS / 8];

// if this is changed then MAX_D3D_MODELS in d3d_local.h must also be changed
#define	MAX_MOD_KNOWN	1500

model_t	mod_known[MAX_MOD_KNOWN];
int		mod_numknown;

int entity_file_size;

/*
===============
Mod_Init
===============
*/
void Mod_Init (void)
{
	memset (mod_novis, 0xff, sizeof (mod_novis));
}


/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t *Mod_PointInLeaf (vec3_t p, model_t *model)
{
	mnode_t		*node;
	float		d;
	mplane_t	*plane;

	if (!model || !model->nodes)
		Sys_Error ("Mod_PointInLeaf: bad model");

	node = model->nodes;

	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t *) node;

		plane = node->plane;
		d = Vector3Dot (p, plane->normal) - plane->dist;

		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;	// never reached
}


/*
===================
Mod_DecompressVis
===================
*/
byte *Mod_DecompressVis (byte *in, model_t *model)
{
	static byte	decompressed[MAX_MAP_LEAFS / 8];
	int		c;
	byte	*out;
	int		row;

	row = (model->numleafs + 7) >> 3;
	out = decompressed;

#if 0
	memcpy (out, in, row);
#else
	if (!in)
	{
		// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}

		return decompressed;
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;

		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
#endif

	return decompressed;
}

byte *Mod_LeafPVS (mleaf_t *leaf, model_t *model)
{
	if (leaf == model->leafs)
		return mod_novis;

	return Mod_DecompressVis (leaf->compressed_vis, model);
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll (void)
{
	int		i;
	model_t	*mod;

	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
		mod->needload = true;
}


/*
==================
Mod_FindName

==================
*/
model_t *Mod_FindName (char *name)
{
	int		i;
	model_t	*mod;

	if (!name[0])
		Sys_Error ("Mod_ForName: NULL name");

	// search the currently loaded models
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
		if (!strcmp (mod->name, name))
			return mod;

	// check for overflow
	if (mod_numknown == MAX_MOD_KNOWN)
		Sys_Error ("mod_numknown == MAX_MOD_KNOWN");

	// take a new model
	strcpy (mod->name, name);
	mod->needload = true;
	mod_numknown++;

	return mod;
}


/*
==================
Mod_TouchModel

==================
*/
void Mod_TouchModel (char *name)
{
	Mod_FindName (name);
}


/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
model_t *Mod_LoadModel (model_t *mod, qboolean crash)
{
	unsigned *buf;

	if (!mod->needload)
		return mod;

	// load the file
	buf = (unsigned *) FS_LoadFile (LOAD_HUNK, mod->name);

	if (!buf)
	{
		if (crash)
			Sys_Error ("Mod_NumForName: %s not found", mod->name);

		return NULL;
	}

	// allocate a new model
	COM_FileBase (mod->name, loadname);

	loadmodel = mod;

	// fill it in
	mod->needload = false;

	// call the apropriate loader
	switch (LittleLong (*(unsigned *) buf))
	{
	case RAPOLYHEADER:
		Mod_LoadAliasModelNew (mod, buf);
		break;

	case IDPOLYHEADER:
		Mod_LoadAliasModel (mod, buf);
		break;

	case IDSPRITEHEADER:
		Mod_LoadSpriteModel (mod, buf);
		break;

	default:
		Mod_LoadBrushModel (mod, buf);
		break;
	}

	Hunk_FreeToLowMark (LOAD_HUNK, 0);

	return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName (char *name, qboolean crash)
{
	model_t	*mod = Mod_FindName (name);
	return Mod_LoadModel (mod, crash);
}


void Mod_CenterFromBounds (vec3_t mids, vec3_t mins, vec3_t maxs)
{
	mids[0] = mins[0] + (maxs[0] - mins[0]) * 0.5f;
	mids[1] = mins[1] + (maxs[1] - mins[1]) * 0.5f;
	mids[2] = mins[2] + (maxs[2] - mins[2]) * 0.5f;
}



/*
=================
Mod_RadiusFromBounds
=================
*/
float Mod_RadiusFromBounds (vec3_t mins, vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i = 0; i < 3; i++)
		corner[i] = fastfabs (mins[i]) > fastfabs (maxs[i]) ? fastfabs (mins[i]) : fastfabs (maxs[i]);

	return Vector3Length (corner);
}


/*
==================
Mod_BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int Mod_BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, mplane_t *p)
{
	float	dist1, dist2;
	int		sides;

	// general case
	switch (p->signbits)
	{
	case 0:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;

	case 1:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;

	case 2:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;

	case 3:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;

	case 4:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;

	case 5:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;

	case 6:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;

	case 7:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;

	default:
		// this is an *error* condition but instead of crashing we report it as intersecting
		return 0;
	}

	sides = 0;

	if (dist1 >= p->dist) sides = 1;
	if (dist2 < p->dist) sides |= 2;

	return sides;
}


