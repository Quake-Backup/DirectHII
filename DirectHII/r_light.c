// r_light.c

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "d_surf.h"
#include "d_light.h"
#include "d_mesh.h"

int	r_dlightframecount = 1;

//==========================================================================
//
// R_AnimateLight
//
//==========================================================================

void R_AnimateLight (double time)
{
	int i;
	int v;
	int c;
	int defaultLocus;
	int locusHz[3];

	defaultLocus = locusHz[0] = (int) (time * 10);
	locusHz[1] = (int) (time * 20);
	locusHz[2] = (int) (time * 30);

	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		if (!cl_lightstyle[i].length)
		{
			// No style def
			d_lightstylevalue[i] = 264;
			continue;
		}

		c = cl_lightstyle[i].map[0];

		if (c == '1' || c == '2' || c == '3')
		{
			// Explicit anim rate
			if (cl_lightstyle[i].length == 1)
			{
				// Bad style def
				d_lightstylevalue[i] = 264;
				continue;
			}

			v = locusHz[c - '1'] % (cl_lightstyle[i].length - 1);
			d_lightstylevalue[i] = (cl_lightstyle[i].map[v + 1] - 'a') * 22;
			continue;
		}

		// Default anim rate (10 Hz)
		v = defaultLocus % cl_lightstyle[i].length;
		d_lightstylevalue[i] = (cl_lightstyle[i].map[v] - 'a') * 22;
	}

	D_AnimateLight (d_lightstylevalue);
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

mplane_t		*lightplane;
vec3_t			lightspot;

qboolean RecursiveLightPoint (mnode_t *node, vec3_t start, vec3_t end, vec3_t shadelight)
{
	float		front, back, frac;
	int			side;
	mplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			s, t, ds, dt;
	int			i;
	mtexinfo_t	*tex;
	byte		*lightmap;
	unsigned	scale;
	int			maps;

	if (node->contents < 0)
		return false;		// didn't hit anything

	// calculate mid point
	// FIXME: optimize for axial
	plane = node->plane;
	front = Vector3Dot (start, plane->normal) - plane->dist;
	back = Vector3Dot (end, plane->normal) - plane->dist;
	side = front < 0;

	if ((back < 0) == side)
		return RecursiveLightPoint (node->children[side], start, end, shadelight);

	frac = front / (front - back);
	mid[0] = start[0] + (end[0] - start[0]) * frac;
	mid[1] = start[1] + (end[1] - start[1]) * frac;
	mid[2] = start[2] + (end[2] - start[2]) * frac;

	// go down front side
	if (RecursiveLightPoint (node->children[side], start, mid, shadelight)) return true;		// hit something
	if ((back < 0) == side) return false;		// didn't hit anuthing

	// check for impact on this node
	Vector3Copy (lightspot, mid);
	lightplane = plane;

	surf = node->surfaces;

	for (i = 0; i < node->numsurfaces; i++, surf++)
	{
		if (surf->flags & SURF_DRAWTILED)
			continue;	// no lightmaps

		tex = surf->texinfo;

		s = Vector3Dot (mid, tex->vecs[0]) + tex->vecs[0][3];
		t = Vector3Dot (mid, tex->vecs[1]) + tex->vecs[1][3];;

		if (s < surf->texturemins[0] || t < surf->texturemins[1])
			continue;

		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];

		if (ds > surf->extents[0] || dt > surf->extents[1])
			continue;

		// clear to no light
		Vector3Set (shadelight, 0.0f, 0.0f, 0.0f);

		ds >>= 4;
		dt >>= 4;

		if ((lightmap = surf->samples) != NULL)
		{
			lightmap += dt * ((surf->extents[0] >> 4) + 1) + ds;

			for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
			{
				scale = d_lightstylevalue[surf->styles[maps]];

				shadelight[0] += lightmap[0] * scale;
				shadelight[1] += lightmap[0] * scale;
				shadelight[2] += lightmap[0] * scale;

				lightmap += ((surf->extents[0] >> 4) + 1) * ((surf->extents[1] >> 4) + 1);
			}
		}

		return true;
	}

	// go down back side
	return RecursiveLightPoint (node->children[!side], mid, end, shadelight);
}


void R_LightPoint (vec3_t p, float *shadelight)
{
	vec3_t		end;

	if (!cl.worldmodel->lightdata)
	{
		Vector3Set (shadelight, 255.0f, 255.0f, 255.0f);
		return;
	}

	// the downtrace is adjusted to the worldmodel bounds so that it will always hit something
	end[0] = p[0];
	end[1] = p[1];
	end[2] = cl.worldmodel->mins[2] - 10.0f;

	if (!RecursiveLightPoint (cl.worldmodel->nodes, p, end, shadelight))
		Vector3Set (shadelight, 0.0f, 0.0f, 0.0f);

	// bring back to the normal range
	Vector3Scalef (shadelight, shadelight, (1.0f / 256.0f));
}


/*
=============================================================================

LIGHTMAP ALLOCATION

this uses the optimized allocator from Q2 which only checks one lightmap at a time rather than
walking the full set of lightmaps for each surf.  this should help loading times for bigger maps
in exchange for a trade-off of some extra unused space in the lightmaps.

a lot of this should move to r_light

=============================================================================
*/

typedef struct lighttexel_s {
	byte styles[4];
} lighttexel_t;

// allocations can be thrown away when lightmap building is completed
lighttexel_t *lm_data[MAX_LIGHTMAPS];


/*
===============
D_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
void R_BuildLightMap (msurface_t *surf)
{
	int s, t, maps;
	int smax = (surf->extents[0] >> 4) + 1;
	int tmax = (surf->extents[1] >> 4) + 1;
	byte *lightmap = surf->samples;

	// alloc storage for the lightmap if required
	if (!lm_data[surf->lightmaptexturenum])
		lm_data[surf->lightmaptexturenum] = (lighttexel_t *) Hunk_Alloc (LOAD_HUNK, LIGHTMAP_SIZE * LIGHTMAP_SIZE * sizeof (lighttexel_t));

	if (!cl.worldmodel->lightdata || r_fullbright.value)
	{
		for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
		{
			// take a pointer to the lightmap dest
			lighttexel_t *dest = lm_data[surf->lightmaptexturenum] + (surf->light_t * LIGHTMAP_SIZE) + surf->light_s;
			int lm_values[MAXLIGHTMAPS] = {128, 0, 0, 0}; // correct for overbright range

			for (t = 0; t < tmax; t++)
			{
				for (s = 0; s < smax; s++)
					dest[s].styles[maps] = lm_values[maps];

				dest += LIGHTMAP_SIZE;
			}
		}
	}
	else if (lightmap)
	{
		for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
		{
			// take a pointer to the lightmap dest
			lighttexel_t *dest = lm_data[surf->lightmaptexturenum] + (surf->light_t * LIGHTMAP_SIZE) + surf->light_s;

			for (t = 0; t < tmax; t++)
			{
				for (s = 0; s < smax; s++)
					dest[s].styles[maps] = lightmap[s];

				dest += LIGHTMAP_SIZE;
				lightmap += smax;
			}
		}
	}
}


static int lm_allocated[LIGHTMAP_SIZE];
static int lm_currenttexture = 0;

static void LM_InitBlock (void)
{
	memset (lm_allocated, 0, sizeof (lm_allocated));
}


static void LM_FinishBlock (void)
{
	if (!lm_data[lm_currenttexture])
		Sys_Error ("LM_FinishBlock : NULL lm_data[lm_currenttexture]");

	if (++lm_currenttexture >= MAX_LIGHTMAPS)
		Sys_Error ("LM_FinishBlock : MAX_LIGHTMAPS exceeded");
}


void R_CreateSurfaceLightmap (msurface_t *surf)
{
	int	i;
	int best = LIGHTMAP_SIZE;

	int smax = (surf->extents[0] >> 4) + 1;
	int tmax = (surf->extents[1] >> 4) + 1;

	if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
		return;

	for (i = 0; i < LIGHTMAP_SIZE - smax; i++)
	{
		int j;
		int best2 = 0;

		for (j = 0; j < smax; j++)
		{
			if (lm_allocated[i + j] >= best)
				break;

			if (lm_allocated[i + j] > best2)
				best2 = lm_allocated[i + j];
		}

		if (j == smax)
		{
			// this is a valid spot
			surf->light_s = i;
			surf->light_t = best = best2;
		}
	}

	if (best + tmax > LIGHTMAP_SIZE)
	{
		// go to a new lightmap and call recursively to use the first allocation in it
		LM_FinishBlock ();
		LM_InitBlock ();
		R_CreateSurfaceLightmap (surf);
	}
	else
	{
		// done; we have an allocation now
		for (i = 0; i < smax; i++)
			lm_allocated[surf->light_s + i] = best + tmax;

		// and build it
		surf->lightmaptexturenum = lm_currenttexture;
		R_BuildLightMap (surf);
	}
}


void R_BeginBuildingLightmaps (void)
{
	// begin building lightmaps
	LM_InitBlock ();
	lm_currenttexture = 0;
	memset (lm_data, 0, sizeof (lm_data));
}


void R_EndBuildingLightmaps (void)
{
	// finish the last lightmap block that was filled
	LM_FinishBlock ();

	// create the lightmap textures, reusing existing textures wherever possible
	D_CreateLightmapTexture (lm_data, lm_currenttexture);

	// any further attempts to access these are errors
	memset (lm_data, 0, sizeof (lm_data));
	lm_currenttexture = 0;
}


qboolean R_SurfaceDLImpact (msurface_t *surf, dlight_t *dl, float dist)
{
	int s, t;
	float impact[3], l;

	impact[0] = dl->transformed[0] - surf->plane->normal[0] * dist;
	impact[1] = dl->transformed[1] - surf->plane->normal[1] * dist;
	impact[2] = dl->transformed[2] - surf->plane->normal[2] * dist;

	// clamp center of light to corner and check brightness
	l = Vector3Dot (impact, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3] - surf->texturemins[0];
	s = l + 0.5; if (s < 0) s = 0; else if (s > surf->extents[0]) s = surf->extents[0];
	s = l - s;

	l = Vector3Dot (impact, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3] - surf->texturemins[1];
	t = l + 0.5; if (t < 0) t = 0; else if (t > surf->extents[1]) t = surf->extents[1];
	t = l - t;

	// compare to minimum light
	return ((s * s + t * t + dist * dist) < (dl->radius * dl->radius));
}


void R_MarkLights (dlight_t *dl, int bit, mnode_t *node, int visframe)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;

	if (node->contents < 0) return;

	// don't bother tracing nodes that will not be visible; this is *far* more effective than any
	// tricksy recursion "optimization" (which a decent compiler will automatically do for you anyway)
	if (node->visframe != visframe) return;

	splitplane = node->plane;
	dist = Vector3Dot (dl->transformed, splitplane->normal) - splitplane->dist;

	if (dist > dl->radius)
	{
		R_MarkLights (dl, bit, node->children[0], visframe);
		return;
	}

	if (dist < -dl->radius)
	{
		R_MarkLights (dl, bit, node->children[1], visframe);
		return;
	}

	// mark the polygons
	for (i = 0, surf = node->surfaces; i < node->numsurfaces; i++, surf++)
	{
		// no lightmaps on these surface types
		if (surf->flags & (SURF_DRAWTURB | SURF_DRAWSKY)) continue;

		// omit surfaces not marked in the current render
		if (surf->dlightframe != r_dlightframecount) continue;

		if (R_SurfaceDLImpact (surf, dl, dist))
		{
			// add dlight surf to texture chain
			// if sorting by texture, just store it out
			surf->texturechain = surf->texinfo->texture->texturechain;
			surf->texinfo->texture->texturechain = surf;

			// the DL has some surfaces now
			dl->numsurfaces++;
		}
	}

	R_MarkLights (dl, bit, node->children[0], visframe);
	R_MarkLights (dl, bit, node->children[1], visframe);
}


void R_DrawDLightChains (entity_t *e, model_t *mod, dlight_t *dl)
{
	int		i;
	msurface_t	*s;
	texture_t	*t;

	D_SetupDynamicLight (dl->transformed, dl->radius);

	// now draw them all
	for (i = 0; i < mod->numtextures; i++)
	{
		if ((t = mod->textures[i]) == NULL) continue;
		if ((s = t->texturechain) == NULL) continue;

		// retrieve the correct animation
		t = R_TextureAnimation (t, e->frame);

		// only solid/lightmapped surfs were added to the dlight chain
		D_SetDynamicLightSurfState (t->texnum);

		R_DrawTextureChain (s);

		// and clear the texture chain
		t->texturechain = NULL;
	}
}


void R_PushDlights (entity_t *e, model_t *mod, QMATRIX *localMatrix, mnode_t *headnode, int visframe)
{
	int	i;

	for (i = 0; i < MAX_DLIGHTS; i++)
	{
		dlight_t *dl = &cl_dlights[i];

		if (dl->die < cl.time || dl->radius < dl->minlight) continue;

		// dl initially hits no surfaces and all texture chains are cleared
		R_ClearTextureChains (mod);
		dl->numsurfaces = 0;

		// move the light into entity local space
		R_VectorInverseTransform (localMatrix, dl->transformed, dl->origin);

		// and mark it
		R_MarkLights (dl, 1 << i, headnode, visframe);

		// did it hit anything???
		if (dl->numsurfaces)
		{
			// draw surfaces hit by this dlight
			R_DrawDLightChains (e, mod, dl);
			dl->numsurfaces = 0;
		}
	}

	// go to a new dlight frame after each push so that we don't carry over lights from the previous
	r_dlightframecount++;
}


qboolean R_AliasLightInteraction (entity_t *e, model_t *mod, dlight_t *dl)
{
	float dist;

	// derive a sphere from the model so that we can do a simple sphere-sphere collision test
	vec3_t vec, mids;
	float rad = Mod_RadiusFromBounds (mod->mins, mod->maxs);

	Mod_CenterFromBounds (mids, mod->mins, mod->maxs);
	Vector3Add (mids, mids, e->origin);
	Vector3Subtract (vec, mids, dl->origin);
	dist = Vector3Length (vec);

	return (dist <= (rad + dl->radius));
}


void R_AliasDlights (entity_t *e, aliashdr_t *hdr, QMATRIX *localMatrix)
{
	int	i;

	for (i = 0; i < MAX_DLIGHTS; i++)
	{
		dlight_t *dl = &cl_dlights[i];

		// don't bother with dlights that have been culled
		if (dl->die < cl.time || dl->radius < dl->minlight) continue;

		if (R_AliasLightInteraction (e, e->model, dl))
		{
			// move the light into entity local space
			R_VectorInverseTransform (localMatrix, dl->transformed, dl->origin);

			// set up the light
			D_SetupDynamicLight (dl->transformed, dl->radius);

			// set up the shaders
			D_SetAliasDynamicState ();

			// and draw it
			D_DrawAliasPolyset (e->model->modelnum);
		}
	}

	// go to a new dlight frame for each push so that we don't carry over lights from the previous
	r_dlightframecount++;
}


