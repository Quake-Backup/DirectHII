// gl_main.c

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "particles.h"

#include "d_video.h"
#include "d_surf.h"
#include "d_main.h"
#include "d_particles.h"
#include "d_sprite.h"


/*
=======================================================================================================================

PARTICLES

=======================================================================================================================
*/

void R_DrawParticles (void)
{
	particle_t		*p, *kill;

	for (;;)
	{
		kill = active_particles;

		if (kill && kill->die < cl.time)
		{
			active_particles = kill->next;
			kill->next = free_particles;
			free_particles = kill;
			continue;
		}

		break;
	}

	// nothing to draw
	if (!active_particles) return;

	D_BeginParticles ();

	for (p = active_particles; p; p = p->next)
	{
		for (;;)
		{
			kill = p->next;

			if (kill && kill->die < cl.time)
			{
				p->next = kill->next;
				kill->next = free_particles;
				free_particles = kill;
				continue;
			}

			break;
		}

		if (p->color > 255)
			D_AddParticle (p->org, d_8to24table_part[((int) p->color - 256) & 255]);
		else D_AddParticle (p->org, d_8to24table_rgba[((int) p->color) & 255]);
	}

	D_EndParticles ();
}


/*
=======================================================================================================================

SPRITE

=======================================================================================================================
*/

typedef struct spritedesc_s {
	float	vup[3];
	float	vright[3];
	float	vpn[3];
} spritedesc_t;

/*
================
R_GetSpriteFrame
================
*/
mspriteframe_t *R_GetSpriteFrame (entity_t *e, msprite_t *psprite)
{
	mspritegroup_t	*pspritegroup;
	mspriteframe_t	*pspriteframe;
	int				i, numframes, frame;
	float			*pintervals, fullinterval, targettime, time;

	frame = e->frame;

	if ((frame >= psprite->numframes) || (frame < 0))
	{
		Con_Printf (PRINT_NORMAL, va ("R_DrawSprite: no such frame %d\n", frame));
		frame = 0;
	}

	if (psprite->frames[frame].type == SPR_SINGLE)
	{
		pspriteframe = psprite->frames[frame].frameptr;
	}
	else
	{
		pspritegroup = (mspritegroup_t *) psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes-1];

		time = cl.time + e->syncbase;

		// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
		// are positive, so we don't have to worry about division by 0
		targettime = time - ((int) (time / fullinterval)) * fullinterval;

		for (i = 0; i < (numframes - 1); i++)
		{
			if (pintervals[i] > targettime)
				break;
		}

		pspriteframe = pspritegroup->frames[i];
	}

	return pspriteframe;
}


/*
=================
R_DrawSpriteModel

=================
*/

void R_DrawSpriteModel (entity_t *e)
{
	msprite_t		*psprite = e->model->spritehdr;
	mspriteframe_t	*frame = R_GetSpriteFrame (e, psprite);

	vec3_t			tvec;
	float			dot, angle, sr, cr;
	spritedesc_t	r_spritedesc;
	int				i;

	float			positions[4][3];

	// SPR_FACING_UPRIGHT needs this
	Vector3Subtract (modelorg, r_refdef.vieworigin, e->origin);

	if (psprite->type == SPR_FACING_UPRIGHT)
	{
		// generate the sprite's axes, with vup straight up in worldspace, and
		// r_spritedesc.vright perpendicular to modelorg.
		// This will not work if the view direction is very close to straight up or
		// down, because the cross product will be between two nearly parallel
		// vectors and starts to approach an undefined state, so we don't draw if
		// the two vectors are less than 1 degree apart
		tvec[0] = -modelorg[0];
		tvec[1] = -modelorg[1];
		tvec[2] = -modelorg[2];

		Vector3Normalize (tvec);

		dot = tvec[2];	// same as Vector3Dot (tvec, r_spritedesc.vup) because r_spritedesc.vup is 0, 0, 1

		if ((dot > 0.999848) || (dot < -0.999848))	// cos(1 degree) = 0.999848
			return;

		r_spritedesc.vup[0] = 0;
		r_spritedesc.vup[1] = 0;
		r_spritedesc.vup[2] = 1;

		r_spritedesc.vright[0] = tvec[1];
		r_spritedesc.vright[1] = -tvec[0];
		r_spritedesc.vright[2] = 0;

		Vector3Normalize (r_spritedesc.vright);

		r_spritedesc.vpn[0] = -r_spritedesc.vright[1];
		r_spritedesc.vpn[1] = r_spritedesc.vright[0];
		r_spritedesc.vpn[2] = 0;
	}
	else if (psprite->type == SPR_VP_PARALLEL)
	{
		// generate the sprite's axes, completely parallel to the viewplane. There
		// are no problem situations, because the sprite is always in the same
		// position relative to the viewer
		for (i = 0; i < 3; i++)
		{
			r_spritedesc.vup[i] = vup[i];
			r_spritedesc.vright[i] = vright[i];
			r_spritedesc.vpn[i] = vpn[i];
		}
	}
	else if (psprite->type == SPR_VP_PARALLEL_UPRIGHT)
	{
		// generate the sprite's axes, with vup straight up in worldspace, and
		// r_spritedesc.vright parallel to the viewplane.
		// This will not work if the view direction is very close to straight up or
		// down, because the cross product will be between two nearly parallel
		// vectors and starts to approach an undefined state, so we don't draw if
		// the two vectors are less than 1 degree apart
		dot = vpn[2];	// same as Vector3Dot (vpn, r_spritedesc.vup) because r_spritedesc.vup is 0, 0, 1

		if ((dot > 0.999848) || (dot < -0.999848))	// cos(1 degree) = 0.999848
			return;

		r_spritedesc.vup[0] = 0;
		r_spritedesc.vup[1] = 0;
		r_spritedesc.vup[2] = 1;

		r_spritedesc.vright[0] = vpn[1];
		r_spritedesc.vright[1] = -vpn[0];	//  r_spritedesc.vright)
		r_spritedesc.vright[2] = 0;

		Vector3Normalize (r_spritedesc.vright);

		r_spritedesc.vpn[0] = -r_spritedesc.vright[1];
		r_spritedesc.vpn[1] = r_spritedesc.vright[0];
		r_spritedesc.vpn[2] = 0;
	}
	else if (psprite->type == SPR_ORIENTED)
	{
		// generate the sprite's axes, according to the sprite's world orientation
		AngleVectors (e->angles, r_spritedesc.vpn, r_spritedesc.vright, r_spritedesc.vup);
	}
	else if (psprite->type == SPR_VP_PARALLEL_ORIENTED)
	{
		// generate the sprite's axes, parallel to the viewplane, but rotated in
		// that plane around the center according to the sprite entity's roll
		// angle. So vpn stays the same, but vright and vup rotate
		angle = e->angles[2] * (M_PI * 2 / 360);
		sr = sin (angle);
		cr = cos (angle);

		for (i = 0; i < 3; i++)
		{
			r_spritedesc.vpn[i] = vpn[i];
			r_spritedesc.vright[i] = vright[i] * cr + vup[i] * sr;
			r_spritedesc.vup[i] = vright[i] * -sr + vup[i] * cr;
		}
	}
	else Sys_Error ("R_DrawSprite: Bad sprite type %d", psprite->type);

	Vector3Madf (positions[0], r_spritedesc.vup, frame->down, e->origin);
	Vector3Madf (positions[0], r_spritedesc.vright, frame->left, positions[0]);

	Vector3Madf (positions[1], r_spritedesc.vup, frame->up, e->origin);
	Vector3Madf (positions[1], r_spritedesc.vright, frame->left, positions[1]);

	Vector3Madf (positions[2], r_spritedesc.vup, frame->up, e->origin);
	Vector3Madf (positions[2], r_spritedesc.vright, frame->right, positions[2]);

	Vector3Madf (positions[3], r_spritedesc.vup, frame->down, e->origin);
	Vector3Madf (positions[3], r_spritedesc.vright, frame->right, positions[3]);

	D_DrawSpriteModel (positions, (e->drawflags & DRF_TRANSLUCENT) ? 100 : 255, frame->texnum);
}


/*
=======================================================================================================================

MAIN

=======================================================================================================================
*/

void R_DrawWorld (void);
void R_DrawBrushModel (entity_t *e, qboolean Translucent);
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

cvar_t	r_clearcolor = {"r_clearcolor","0"};
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
void R_DrawSpriteModel (entity_t *e);


//==================================================================================

typedef struct sortedent_s
{
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


int transCompare (sortedent_t *a1, sortedent_t *a2) {return (a2->len - a1->len);}


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
	Vector3Set (vpn,   -r_view_matrix.m4x4[0][2], -r_view_matrix.m4x4[1][2], -r_view_matrix.m4x4[2][2]);
	Vector3Set (vup,    r_view_matrix.m4x4[0][1],  r_view_matrix.m4x4[1][1],  r_view_matrix.m4x4[2][1]);
	Vector3Set (vright, r_view_matrix.m4x4[0][0],  r_view_matrix.m4x4[1][0],  r_view_matrix.m4x4[2][0]);

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

