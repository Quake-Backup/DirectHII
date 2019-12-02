// r_surf.c: surface-related refresh code

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "d_surf.h"
#include "d_main.h"
#include "d_light.h"

msurface_t *r_skychain = NULL;
msurface_t *r_waterchain = NULL;
msurface_t *r_mainwaterchain = NULL;

void R_StoreEfrags (efrag_t **ppefrag);

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
texture_t *R_TextureAnimation (texture_t *base, int frame)
{
	int		reletive;
	int		count;

	if (frame)
	{
		if (base->alternate_anims)
			base = base->alternate_anims;
	}

	if (!base->anim_total)
		return base;

	reletive = (int) (cl.time * 10) % base->anim_total;
	count = 0;

	while (base->anim_min > reletive || base->anim_max <= reletive)
	{
		base = base->anim_next;

		if (!base) Sys_Error ("R_TextureAnimation: broken cycle");
		if (++count > 100) Sys_Error ("R_TextureAnimation: infinite cycle");
	}

	return base;
}


/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
================
R_DrawTextureChains
================
*/
void R_ClearTextureChains (model_t *mod)
{
	int i;
	texture_t *t;

	for (i = 0; i < mod->numtextures; i++)
	{
		if ((t = mod->textures[i]) == NULL) continue;
		t->texturechain = NULL;
	}

	r_skychain = NULL;
	r_waterchain = NULL;
}


unsigned int r_dipindexes[MAX_SURF_INDEXES];
int r_numdipindexes = 0;

void R_BeginSurfaceBatch (void)
{
	r_numdipindexes = 0;
}


void R_EndSurfaceBatch (void)
{
	if (r_numdipindexes)
	{
		D_DrawSurfaceBatch (r_dipindexes, r_numdipindexes);
		R_BeginSurfaceBatch ();
	}
}


void R_AddSurfaceToBatch (msurface_t *surf)
{
	if (r_numdipindexes + surf->numindexes >= MAX_SURF_INDEXES)
	{
		R_EndSurfaceBatch ();
		R_AddSurfaceToBatch (surf);
	}
	else
	{
		int v;
		unsigned int *ndx = &r_dipindexes[r_numdipindexes];

		// write in the indexes
		for (v = 2; v < surf->numedges; v++, ndx += 3)
		{
			ndx[0] = surf->firstvertex;
			ndx[1] = surf->firstvertex + (v - 1);
			ndx[2] = surf->firstvertex + v;
		}

		// accumulate indexes count
		r_numdipindexes += surf->numindexes;
	}
}


void R_DrawTextureChain (msurface_t *surf)
{
	R_BeginSurfaceBatch ();

	for (; surf; surf = surf->texturechain)
		R_AddSurfaceToBatch (surf);

	R_EndSurfaceBatch ();
}


void R_DrawWaterChain (msurface_t *surf)
{
	int LastTexnum = 0;

	R_BeginSurfaceBatch ();

	for (; surf; surf = surf->texturechain)
	{
		if (surf->texinfo->texture->texnum != LastTexnum)
		{
			R_EndSurfaceBatch ();
			D_SetWaterState (surf->texinfo->texture->texnum);
			LastTexnum = surf->texinfo->texture->texnum;
		}

		R_AddSurfaceToBatch (surf);
	}

	R_EndSurfaceBatch ();
}


void R_DrawWaterSurfaces (void)
{
	// only translucent water surfaces should go through this
	if (r_mainwaterchain)
	{
		// go back to the world matrix
		D_SurfBeginDrawingModel (true);
		D_UpdateEntityConstants (&r_mvp_matrix, 0.4f, 1.0f, vec3_origin);

		// and draw the surfaces
		R_DrawWaterChain (r_mainwaterchain);
		r_mainwaterchain = NULL;
	}
}


void R_DrawSkyChain (entity_t *e, msurface_t *chain)
{
	D_SetSkyState ();
	R_DrawTextureChain (chain);
}


void R_DrawTextureChains (entity_t *e, model_t *mod, QMATRIX *localmatrix, float alpha, float abslight)
{
	int i;
	msurface_t *s;
	texture_t *t;
	QMATRIX objmatrix;

	// we need to retain the local matrix for lighting so transform it into objmatrix
	R_MatrixMultiply (&objmatrix, localmatrix, &r_mvp_matrix);

	// and now set it up
	D_SurfBeginDrawingModel (alpha < 1.0f);
	D_UpdateEntityConstants (&objmatrix, alpha, abslight, e->origin);

	// it's assumed that there will always be solid surfs
	for (i = 0; i < mod->numtextures; i++)
	{
		if ((t = mod->textures[i]) == NULL) continue;
		if ((s = t->texturechain) == NULL) continue;

		// retrieve the correct animation
		t = R_TextureAnimation (t, e->frame);

		if (s->flags & SURF_DRAWTURB)
			D_SetWaterState (t->texnum);
		else if ((e->drawflags & MLS_ABSLIGHT) == MLS_ABSLIGHT)
			D_SetAbsLightState (t->texnum);
		else D_SetLightmappedState (t->texnum);

		R_DrawTextureChain (s);
		t->texturechain = NULL;
	}

	// water may or may not exist
	// must be drawn before dlights because dlights will clear the chains
	if (r_waterchain)
	{
		R_DrawWaterChain (r_waterchain);
		r_waterchain = NULL;
	}

	// sky may or may not exist
	// must be drawn before dlights because dlights will clear the chains
	if (r_skychain)
	{
		R_DrawSkyChain (e, r_skychain);
		r_skychain = NULL;
	}

	// now do dynamic lighting for the entity
	if (mod == cl.worldmodel)
		R_PushDlights (e, mod, localmatrix, mod->nodes, r_visframecount);
	else if (mod->firstmodelsurface != 0)
		R_PushDlights (e, mod, localmatrix, mod->nodes + mod->hulls[0].firstclipnode, 0);
	else R_PushDlights (e, mod, localmatrix, mod->nodes, 0);
}


void R_ChainSurface (model_t *mod, msurface_t *surf, float alpha)
{
	if (surf->flags & SURF_DRAWSKY)
	{
		surf->texturechain = r_skychain;
		r_skychain = surf;
		return;
	}

	if (surf->flags & SURF_DRAWTURB)
	{
		if ((surf->flags & SURF_TRANSLUCENT) || (alpha < 1.0f))
		{
			surf->texturechain = r_waterchain;
			r_waterchain = surf;
			return;
		}
	}

	surf->texturechain = surf->texinfo->texture->texturechain;
	surf->texinfo->texture->texturechain = surf;

	// mark the surf as eligible for dlights
	surf->dlightframe = r_dlightframecount;
}


qboolean R_CullBox (vec3_t mins, vec3_t maxs, int clipflags)
{
	// only used by the surf code now
	int i;
	int side;

	for (i = 0; i < 4; i++)
	{
		// don't need to clip against it
		if (!(clipflags & (1 << i))) continue;

		// the frustum planes are *never* axial so there's no point in doing the "fast" pre-test
		side = Mod_BoxOnPlaneSide (mins, maxs, &frustum[i]);

		if (side == 1) clipflags &= ~(1 << i);
		if (side == 2) return true;
	}

	// onscreen
	return false;
}


/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (entity_t *e, qboolean Translucent)
{
	vec3_t		mins, maxs;
	int			i;
	msurface_t	*surf;
	QMATRIX		localmatrix;

	float intensity = 1.0f, alpha_val = 1.0f;
	model_t *clmodel = e->model;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		for (i = 0; i < 3; i++)
		{
			mins[i] = e->origin[i] - clmodel->radius;
			maxs[i] = e->origin[i] + clmodel->radius;
		}
	}
	else
	{
		VectorAdd (e->origin, clmodel->mins, mins);
		VectorAdd (e->origin, clmodel->maxs, maxs);
	}

	if (R_CullBox (mins, maxs, 15))
		return;

	// get the transform in local space so that we can correctly handle dlights
	R_MatrixIdentity (&localmatrix);
	R_MatrixTranslate (&localmatrix, e->origin[0], e->origin[1], e->origin[2]);
	R_MatrixRotate (&localmatrix, e->angles[0], e->angles[1], e->angles[2]);

	R_VectorInverseTransform (&localmatrix, modelorg, r_refdef.vieworigin);
	R_ClearTextureChains (clmodel);

	// now set up to draw
	surf = &clmodel->surfaces[clmodel->firstmodelsurface];

	// draw texture
	for (i = 0; i < clmodel->nummodelsurfaces; i++, surf++)
	{
		// find which side of the node we are on
		mplane_t *pplane = surf->plane;
		float dot = Vector3Dot (modelorg, pplane->normal) - pplane->dist;

		// draw the polygon
		if (((surf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) || (!(surf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (e->drawflags & DRF_TRANSLUCENT)
				R_ChainSurface (clmodel, surf, 0.4f);
			else R_ChainSurface (clmodel, surf, 1.0f);
		}
	}

	if (e->drawflags & DRF_TRANSLUCENT)
	{
		alpha_val = 0.4f;
		intensity = 1.0f;
	}

	if ((e->drawflags & MLS_ABSLIGHT) == MLS_ABSLIGHT)
		intensity = (float) e->abslight / 255.0f;

	R_DrawTextureChains (e, clmodel, &localmatrix, alpha_val, intensity);
}


/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode (mnode_t *node, int clipflags)
{
	int			c, side;
	mplane_t	*plane;
	double		dot;

	if (node->contents == CONTENTS_SOLID) return;
	if (node->visframe != r_visframecount) return;

	if (clipflags)
	{
		for (c = 0; c < 4; c++)
		{
			// don't need to clip against it
			if (!(clipflags & (1 << c))) continue;

			// the frustum planes are *never* axial so there's no point in doing the "fast" pre-test
			side = Mod_BoxOnPlaneSide (node->mins, node->maxs, &frustum[c]);

			if (side == 1) clipflags &= ~(1 << c);	// node is entirely on screen on this side
			if (side == 2) return;	// node is entirely off screen
		}
	}

	// if a leaf node, draw stuff
	if (node->contents < 0)
	{
		mleaf_t *pleaf = (mleaf_t *) node;
		msurface_t **mark = pleaf->firstmarksurface;

		if ((c = pleaf->nummarksurfaces) > 0)
		{
			do
			{
				(*mark)->visframe = r_framecount;
				mark++;
			} while (--c);
		}

		// deal with model fragments in this leaf
		if (pleaf->efrags) R_StoreEfrags (&pleaf->efrags);

		return;
	}

	// node is just a decision point, so go down the apropriate sides
	// find which side of the node we are on
	plane = node->plane;
	dot = Vector3Dot (modelorg, plane->normal) - plane->dist;

	if (dot >= 0)
		side = 0;
	else side = 1;

	// recurse down the children, front side first
	R_RecursiveWorldNode (node->children[side], clipflags);

	// draw stuff
	if ((c = node->numsurfaces) > 0)
	{
		msurface_t *surf = node->surfaces;

		for (; c; c--, surf++)
		{
			if (surf->visframe != r_framecount) continue;
			if ((dot < 0) ^ !!(surf->flags & SURF_PLANEBACK)) continue;
			if (R_CullBox (surf->mins, surf->maxs, clipflags)) continue;

			R_ChainSurface (cl.worldmodel, surf, 1.0f);
		}
	}

	// recurse down the back side
	R_RecursiveWorldNode (node->children[!side], clipflags);
}



/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld (void)
{
	QMATRIX		localmatrix;

	// reset the world entity every frame
	memset (&r_worldentity, 0, sizeof (r_worldentity));

	r_worldentity.model = cl.worldmodel;

	Vector3Set (r_worldentity.origin, 0, 0, 0);
	Vector3Set (r_worldentity.angles, 0, 0, 0);

	Vector3Copy (modelorg, r_refdef.vieworigin);

	// set up entity origin and angles
	R_MatrixIdentity (&localmatrix);

	R_ClearTextureChains (cl.worldmodel);
	R_RecursiveWorldNode (cl.worldmodel->nodes, 15);

	// save out the main water chain for drawing later
	r_mainwaterchain = r_waterchain;
	r_waterchain = NULL;

	R_DrawTextureChains (&r_worldentity, cl.worldmodel, &localmatrix, 1.0f, 1.0f);
}


void R_LeafPVS (mleaf_t *leaf)
{
	byte	*vis;
	mnode_t	*node;
	int		i;
	byte	solid[4096];

	if (!leaf) return;

	if (r_novis.value)
	{
		vis = solid;
		memset (solid, 0xff, (cl.worldmodel->numleafs + 7) >> 3);
	}
	else vis = Mod_LeafPVS (leaf, cl.worldmodel);

	for (i = 0; i < cl.worldmodel->numleafs; i++)
	{
		if (vis[i >> 3] & (1 << (i & 7)))
		{
			node = (mnode_t *) &cl.worldmodel->leafs[i + 1];

			do
			{
				if (node->visframe == r_visframecount)
					break;

				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}


/*
===============
R_MarkLeaves
===============
*/
void R_MarkLeaves (void)
{
	if (r_oldviewleaf == r_viewleaf && !r_novis.value)
		return;

	r_visframecount++;

	if (r_novis.value)
		R_LeafPVS (r_viewleaf);
	else if (r_oldviewleaf && r_oldviewleaf->contents != r_viewleaf->contents)
	{
		R_LeafPVS (r_viewleaf);
		R_LeafPVS (r_oldviewleaf);
	}
	else R_LeafPVS (r_viewleaf);

	r_oldviewleaf = r_viewleaf;
}


/*
================
R_BuildPolygonForSurface
================
*/
void R_BuildPolygonForSurface (model_t *m, msurface_t *surf, brushpolyvert_t *verts)
{
	int i;
	mtexinfo_t *tex = surf->texinfo;

	// reconstruct the polygon
	for (i = 0; i < surf->numedges; i++, verts++)
	{
		int lindex = m->surfedges[surf->firstedge + i];

		if (lindex > 0)
			Vector3Copy (verts->xyz, m->vertexes[m->edges[lindex].v[0]].position);
		else Vector3Copy (verts->xyz, m->vertexes[m->edges[-lindex].v[1]].position);

		if (surf->flags & SURF_DRAWTURB)
		{
			// precalc as much of this as possible so that we don't need a more complex FS
			verts->st[0] = Vector3Dot (verts->xyz, tex->vecs[0]) * 0.015625f;
			verts->st[1] = Vector3Dot (verts->xyz, tex->vecs[1]) * 0.015625f;

			verts->lm[0] = Vector3Dot (verts->xyz, tex->vecs[1]) * M_PI / 64.0f;
			verts->lm[1] = Vector3Dot (verts->xyz, tex->vecs[0]) * M_PI / 64.0f;
		}
		else if (!(surf->flags & SURF_DRAWSKY))
		{
			verts->st[0] = (Vector3Dot (verts->xyz, tex->vecs[0]) + tex->vecs[0][3]) / tex->texture->width;
			verts->st[1] = (Vector3Dot (verts->xyz, tex->vecs[1]) + tex->vecs[1][3]) / tex->texture->height;

			// lightmap texture coordinates
			verts->lm[0] = Vector3Dot (verts->xyz, tex->vecs[0]) + tex->vecs[0][3];
			verts->lm[0] -= surf->texturemins[0];
			verts->lm[0] += surf->light_s * 16;
			verts->lm[0] += 8;
			verts->lm[0] /= LIGHTMAP_SIZE * 16; //tex->texture->width;

			verts->lm[1] = Vector3Dot (verts->xyz, tex->vecs[1]) + tex->vecs[1][3];
			verts->lm[1] -= surf->texturemins[1];
			verts->lm[1] += surf->light_t * 16;
			verts->lm[1] += 8;
			verts->lm[1] /= LIGHTMAP_SIZE * 16; //tex->texture->height;

			// texture array index for this surf's lightmap
			verts->mapnum = surf->lightmaptexturenum;

			// copy over the styles (d3d guarantees to return 0 for out-of-bounds access so this is safe)
			verts->styles[0] = surf->styles[0];
			verts->styles[1] = surf->styles[1];
			verts->styles[2] = surf->styles[2];
			verts->styles[3] = surf->styles[3];
		}
	}
}


/*
==================
R_BuildSurfaces

Builds the lightmap texture
with all the surfaces from all brush models
==================
*/
int r_numsurfaceverts = 0;

void R_BuildSurfaces (void)
{
	int	i, j;
	int mark = Hunk_LowMark (LOAD_HUNK);

	// alloc a buffer to write the verts to and create the VB from
	brushpolyvert_t *surfverts = (brushpolyvert_t *) Hunk_Alloc (LOAD_HUNK, sizeof (brushpolyvert_t) * r_numsurfaceverts);

	// beginning a new map (this is a crap place to set this...)
	r_framecount = 1;

	// initialize lighting
	R_BeginBuildingLightmaps ();

	for (j = 1; j < MAX_MODELS; j++)
	{
		model_t *m = cl.model_precache[j];

		if (!m) break;
		if (m->name[0] == '*') continue;

		for (i = 0; i < m->numsurfaces; i++)
		{
			msurface_t *surf = &m->surfaces[i];

			// now we can create our stuff
			R_CreateSurfaceLightmap (surf);
			R_BuildPolygonForSurface (m, surf, &surfverts[surf->firstvertex]);
		}
	}

	// complete the lightmaps
	R_EndBuildingLightmaps ();

	// recreate the vertex buffer
	D_SurfCreateVertexBuffer (surfverts, r_numsurfaceverts);

	// for the next map
	r_numsurfaceverts = 0;

	// all done
	Hunk_FreeToLowMark (LOAD_HUNK, mark);
}


void R_RegisterSurface (msurface_t *surf)
{
	surf->firstvertex = r_numsurfaceverts;
	r_numsurfaceverts += surf->numedges;
	surf->numindexes = (surf->numedges - 2) * 3;
	surf->lastvertex = r_numsurfaceverts - 1;
}

