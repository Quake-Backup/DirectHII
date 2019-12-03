// gl_main.c

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "d_sprite.h"


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
		fullinterval = pintervals[numframes - 1];

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


