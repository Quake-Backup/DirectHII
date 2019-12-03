// gl_main.c

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "d_video.h"
#include "d_main.h"


/*
=======================================================================================================================

MAIN

=======================================================================================================================
*/

void R_DrawWorld (void);
void V_CalcBlend (void);

QMATRIX	r_view_matrix;
QMATRIX	r_proj_matrix;
QMATRIX	r_mvp_matrix;

entity_t	r_worldentity;

vec3_t		modelorg;

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

mplane_t	frustum[4];

int gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models

extern model_t *player_models[NUM_CLASSES];

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

//
// screen size info
//
refdef_t	r_refdef;
refdef_t	r_viewmodel_refdef;

mleaf_t		*r_viewleaf, *r_oldviewleaf;
qboolean	r_dowarp, r_dowarpold;

texture_t	r_notexture_mip;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value


void R_MarkLeaves (void);
void R_DrawWaterSurfaces (void);
void R_DrawParticles (void);

cvar_t	r_clearcolor = {"r_clearcolor", "0"};
cvar_t	r_norefresh = {"r_norefresh", "0"};
cvar_t	r_drawentities = {"r_drawentities", "1"};
cvar_t	r_drawviewmodel = {"r_drawviewmodel", "1"};
cvar_t	r_waterwarp = {"r_waterwarp", "1"};
cvar_t	r_fullbright = {"r_fullbright", "0"};
cvar_t	r_lightmap = {"r_lightmap", "0"};
cvar_t	r_novis = {"r_novis", "0"};
cvar_t	r_wholeframe = {"r_wholeframe", "1", true};

cvar_t	gl_clear = {"gl_clear", "0"};
cvar_t	gl_polyblend = {"gl_polyblend", "1"};

void R_DrawAliasModel (entity_t *e);
void R_DrawBrushModel (entity_t *e, qboolean Translucent);
void R_DrawSpriteModel (entity_t *e);


//==================================================================================

typedef struct sortedent_s {
	entity_t *ent;
	vec_t len;
} sortedent_t;

sortedent_t     cl_transvisedicts[MAX_VISEDICTS];
sortedent_t		cl_transwateredicts[MAX_VISEDICTS];

int				cl_numtransvisedicts;
int				cl_numtranswateredicts;

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int		i;
	qboolean item_trans;
	mleaf_t *pLeaf;

	cl_numtransvisedicts = 0;
	cl_numtranswateredicts = 0;

	if (!r_drawentities.value)
		return;

	// draw sprites seperately, because of alpha blending
	for (i = 0; i < cl_numvisedicts; i++)
	{
		entity_t *e = cl_visedicts[i];

		switch (e->model->type)
		{
		case mod_alias:
			item_trans = ((e->drawflags & DRF_TRANSLUCENT) || (e->model->flags & (EF_TRANSPARENT | EF_HOLEY | EF_SPECIAL_TRANS))) != 0;

			if (!item_trans)
				R_DrawAliasModel (e);

			break;

		case mod_brush:
			item_trans = ((e->drawflags & DRF_TRANSLUCENT)) != 0;

			if (!item_trans)
				R_DrawBrushModel (e, false);

			break;

		case mod_sprite:
			item_trans = true;

			break;

		default:
			item_trans = false;
			break;
		}

		if (item_trans)
		{
			pLeaf = Mod_PointInLeaf (e->origin, cl.worldmodel);

			if (pLeaf->contents != CONTENTS_WATER)
				cl_transvisedicts[cl_numtransvisedicts++].ent = e;
			else cl_transwateredicts[cl_numtranswateredicts++].ent = e;
		}
	}
}


int transCompare (sortedent_t *a1, sortedent_t *a2) { return (a2->len - a1->len); }


void R_DrawTransEntitiesOnList (qboolean inwater)
{
	int i;
	int numents;
	sortedent_t *theents;
	vec3_t result;

	theents = (inwater) ? cl_transwateredicts : cl_transvisedicts;
	numents = (inwater) ? cl_numtranswateredicts : cl_numtransvisedicts;

	for (i = 0; i < numents; i++)
	{
		VectorSubtract (theents[i].ent->origin, r_origin, result);
		theents[i].len = Vector3Dot (result, result);
	}

	qsort ((void *) theents, numents, sizeof (sortedent_t), (sortfunc_t) transCompare);

	for (i = 0; i < numents; i++)
	{
		entity_t *e = theents[i].ent;

		switch (e->model->type)
		{
		case mod_alias:
			R_DrawAliasModel (e);
			break;

		case mod_brush:
			R_DrawBrushModel (e, true);
			break;

		case mod_sprite:
			R_DrawSpriteModel (e);
			break;
		}
	}
}


/*
=============
R_DrawViewModel
=============
*/
void R_DrawViewModel (void)
{
	entity_t *e = &cl.viewent;

	if (scr.timeRefresh) return;
	if (!e->model) return;
	if (!r_drawviewmodel.value) return;
	if (!r_drawentities.value) return;
	if (cl.items & IT_INVISIBILITY) return;
	if (cl.v.health <= 0) return;

	R_DrawAliasModel (e);
}


int SignbitsForPlane (mplane_t *out)
{
	// for fast box on planeside test
	int j, bits = 0;

	for (j = 0; j < 3; j++)
		if (out->normal[j] < 0)
			bits |= 1 << j;

	return bits;
}


void R_ExtractFrustum (mplane_t *f, QMATRIX *m)
{
	int i;

	// extract the frustum from the MVP matrix
	f[0].normal[0] = m->m4x4[0][3] - m->m4x4[0][0];
	f[0].normal[1] = m->m4x4[1][3] - m->m4x4[1][0];
	f[0].normal[2] = m->m4x4[2][3] - m->m4x4[2][0];

	f[1].normal[0] = m->m4x4[0][3] + m->m4x4[0][0];
	f[1].normal[1] = m->m4x4[1][3] + m->m4x4[1][0];
	f[1].normal[2] = m->m4x4[2][3] + m->m4x4[2][0];

	f[2].normal[0] = m->m4x4[0][3] + m->m4x4[0][1];
	f[2].normal[1] = m->m4x4[1][3] + m->m4x4[1][1];
	f[2].normal[2] = m->m4x4[2][3] + m->m4x4[2][1];

	f[3].normal[0] = m->m4x4[0][3] - m->m4x4[0][1];
	f[3].normal[1] = m->m4x4[1][3] - m->m4x4[1][1];
	f[3].normal[2] = m->m4x4[2][3] - m->m4x4[2][1];

	for (i = 0; i < 4; i++)
	{
		f[i].dist = Vector3Dot (r_origin, f[i].normal);
		f[i].signbits = SignbitsForPlane (&f[i]);
	}
}


/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	// don't allow cheats in multiplayer
	if (cl.maxclients > 1)
		Cvar_Set ("r_fullbright", "0");

	R_AnimateLight (cl.time);

	r_framecount++;

	// build the transformation matrix for the given view angles
	Vector3Copy (r_origin, r_refdef.vieworigin);

	// current viewleaf
	r_oldviewleaf = r_viewleaf;
	r_viewleaf = Mod_PointInLeaf (r_origin, cl.worldmodel);

	// underwater warp state
	// this is the same test as was used by the software version
	r_dowarpold = r_dowarp;
	r_dowarp = r_waterwarp.value && (r_viewleaf->contents <= CONTENTS_WATER);

	V_SetContentsColor (r_viewleaf->contents);
	V_CalcBlend ();

	// allow scaling by cvar
	v_blend[3] *= gl_polyblend.value;
}


void R_DrawPolyBlend (void)
{
	if (v_blend[3] > 0)
	{
		D_DrawPolyblend ();
		v_blend[3] = 0; // clear for next frame
	}
}


void R_RenderScene (int width, int height)
{
	// the viewport size depends if we're warping or not
	D_SetViewport (0, 0, width, height, 0, 1);

	// the projection matrix may be only updated when the refdef changes but we do it every frame so that we can do waterwarp
	R_MatrixIdentity (&r_proj_matrix);
	R_MatrixFrustum (&r_proj_matrix, r_refdef.fov_x, r_refdef.fov_y, 4.0f, R_GetFarClip ());

	// modelview is updated every frame; done in reverse so that we can use the new sexy rotation on it
	R_MatrixIdentity (&r_view_matrix);
	R_MatrixCamera (&r_view_matrix, r_refdef.vieworigin, r_refdef.viewangles);

	// compute the global MVP
	R_MatrixMultiply (&r_mvp_matrix, &r_view_matrix, &r_proj_matrix);

	// and extract the frustum from it
	R_ExtractFrustum (frustum, &r_mvp_matrix);

	// take these from the view matrix
	Vector3Set (vpn, -r_view_matrix.m4x4[0][2], -r_view_matrix.m4x4[1][2], -r_view_matrix.m4x4[2][2]);
	Vector3Set (vup, r_view_matrix.m4x4[0][1], r_view_matrix.m4x4[1][1], r_view_matrix.m4x4[2][1]);
	Vector3Set (vright, r_view_matrix.m4x4[0][0], r_view_matrix.m4x4[1][0], r_view_matrix.m4x4[2][0]);

	// setup the shader constants that are going to remain unchanged for the frame; time-based animations, etc
	D_UpdateMainConstants (&r_mvp_matrix, r_refdef.vieworigin, vpn, vright, vup, v_blend, cl.time);

	if (r_viewleaf->contents == CONTENTS_SOLID)
		D_ClearRenderTargets (0x22222222, TRUE, TRUE, TRUE);
	else if (gl_clear.value)
		D_ClearRenderTargets (d_8to24table_rgba[(int) r_clearcolor.value & 255], TRUE, TRUE, TRUE);
	else D_ClearRenderTargets (0, FALSE, TRUE, TRUE);

	R_MarkLeaves ();	// done here so we know if we're in water

	R_DrawWorld ();		// adds static entities to the list

	R_DrawEntitiesOnList ();

	R_DrawParticles ();

	R_DrawTransEntitiesOnList (r_viewleaf->contents == CONTENTS_EMPTY);  // This restores the depth mask

	R_DrawWaterSurfaces ();

	R_DrawTransEntitiesOnList (r_viewleaf->contents != CONTENTS_EMPTY);

	R_DrawViewModel ();
}


/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (void)
{
	if (r_norefresh.value)
		return;

	if (!r_worldentity.model || !cl.worldmodel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	// render normal view
	R_SetupFrame ();

	// a contents shift > 0 implies in water (need to check this...)
	// if we're water-warping we must use a full-screen viewport and we'll reduce it for sb_lines when drawing the warped view back
	if (r_dowarp)
	{
		if (D_BeginWaterWarp ())
		{
			R_RenderScene (vid.width, vid.height);
			D_DoWaterWarp (vid.width, vid.height - sb_lines);
			v_blend[3] = 0; // disable the polyblend pass
			return;
		}
	}

	// normal unwarped view
	R_RenderScene (vid.width, vid.height - sb_lines);
	R_DrawPolyBlend ();
}

