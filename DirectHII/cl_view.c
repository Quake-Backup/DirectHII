// view.c -- player eye positioning

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/

cvar_t	scr_ofsx = {"scr_ofsx", "0", false};
cvar_t	scr_ofsy = {"scr_ofsy", "0", false};
cvar_t	scr_ofsz = {"scr_ofsz", "0", false};

cvar_t	cl_rollspeed = {"cl_rollspeed", "200"};
cvar_t	cl_rollangle = {"cl_rollangle", "2.0"};

cvar_t	cl_bob = {"cl_bob", "0.02", false};
cvar_t	cl_bobcycle = {"cl_bobcycle", "0.6", false};
cvar_t	cl_bobup = {"cl_bobup", "0.5", false};

cvar_t	v_kicktime = {"v_kicktime", "0.5", false};
cvar_t	v_kickroll = {"v_kickroll", "0.6", false};
cvar_t	v_kickpitch = {"v_kickpitch", "0.6", false};

cvar_t	v_iyaw_cycle = {"v_iyaw_cycle", "2", false};
cvar_t	v_iroll_cycle = {"v_iroll_cycle", "0.5", false};
cvar_t	v_ipitch_cycle = {"v_ipitch_cycle", "1", false};
cvar_t	v_iyaw_level = {"v_iyaw_level", "0.3", false};
cvar_t	v_iroll_level = {"v_iroll_level", "0.1", false};
cvar_t	v_ipitch_level = {"v_ipitch_level", "0.3", false};

cvar_t	v_idlescale = {"v_idlescale", "0", false};

cvar_t	crosshair = {"crosshair", "1", true};
cvar_t	crosshaircolor = {"crosshaircolor", "254", true};
cvar_t	cl_crossx = {"cl_crossx", "0", false};
cvar_t	cl_crossy = {"cl_crossy", "0", false};

float	v_dmg_time, v_dmg_roll, v_dmg_pitch;

float	v_oldstepz, v_newstepz;
float	v_stepchangetime;

extern	int			in_forward, in_forward2, in_back;
extern	cvar_t	sv_idealrollscale;


/*
===============
V_CalcRoll

Used by view and sv_user
===============
*/
vec3_t	forward, right, up;

float V_CalcRoll (vec3_t angles, vec3_t velocity)
{
	float	sign;
	float	side;
	float	value;

	AngleVectors (angles, forward, right, up);
	side = Vector3Dot (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fastfabs (side);

	value = cl_rollangle.value;
	//	if (cl.inwater)
	//		value *= 6;

	if (side < cl_rollspeed.value)
		side = side * value / cl_rollspeed.value;
	else
		side = value;

	return side * sign;

}


/*
===============
V_CalcBob

===============
*/
float V_CalcBob (void)
{
	float	bob;
	float	cycle;

	cycle = cl.time - (int) (cl.time / cl_bobcycle.value) * cl_bobcycle.value;
	cycle /= cl_bobcycle.value;

	if (cycle < cl_bobup.value)
		cycle = M_PI * cycle / cl_bobup.value;
	else
		cycle = M_PI + M_PI * (cycle - cl_bobup.value) / (1.0 - cl_bobup.value);

	// bob is proportional to velocity in the xy plane
	// (don't count Z, or jumping messes it up)

	bob = sqrt (cl.velocity[0] * cl.velocity[0] + cl.velocity[1] * cl.velocity[1]) * cl_bob.value;
	bob = bob * 0.3 + bob * 0.7 * sin (cycle);

	if (bob > 4)
		bob = 4;
	else if (bob < -7)
		bob = -7;

	return bob;

}


/*
==============================================================================

PALETTE FLASHES

==============================================================================
*/


cshift_t	cshift_empty = {{130, 80, 50}, 0};
cshift_t	cshift_water = {{130, 80, 50}, 128};
cshift_t	cshift_slime = {{0, 25, 5}, 150};
cshift_t	cshift_lava = {{255, 80, 0}, 150};

cvar_t		v_gamma = {"gamma", "1", true};

float		v_blend[4];		// rgba 0.0 - 1.0


/*
===============
V_ParseDamage
===============
*/
void V_ParseDamage (void)
{
	int		armor, blood;
	vec3_t	from;
	int		i;
	vec3_t	forward, right, up;
	entity_t	*ent;
	float	side;
	float	count;

	armor = MSG_ReadByte ();
	blood = MSG_ReadByte ();

	for (i = 0; i < 3; i++)
		from[i] = MSG_ReadCoord ();

	count = blood * 0.5 + armor * 0.5;

	if (count < 10)
		count = 10;

	cl.faceanimtime = cl.time + 0.2;		// but sbar face into pain frame

	cl.cshifts[CSHIFT_DAMAGE].percent += 10 * count;

	if (cl.cshifts[CSHIFT_DAMAGE].percent < 0) cl.cshifts[CSHIFT_DAMAGE].percent = 0;
	if (cl.cshifts[CSHIFT_DAMAGE].percent > 150) cl.cshifts[CSHIFT_DAMAGE].percent = 150;

	if (armor > blood)
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 200;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 100;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 100;
	}
	else if (armor)
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 220;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 50;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 50;
	}
	else
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 255;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 0;
	}

	// update initial percent and time so that we can fade off correctly
	cl.cshifts[CSHIFT_DAMAGE].initialpercent = cl.cshifts[CSHIFT_DAMAGE].percent;
	cl.cshifts[CSHIFT_DAMAGE].inittime = cl.time;

	// calculate view angle kicks
	ent = &cl_entities[cl.viewentity];

	VectorSubtract (from, ent->origin, from);
	Vector3Normalize (from);

	AngleVectors (ent->angles, forward, right, up);

	side = Vector3Dot (from, right);
	v_dmg_roll = count * side * v_kickroll.value;

	side = Vector3Dot (from, forward);
	v_dmg_pitch = count * side * v_kickpitch.value;

	v_dmg_time = cl.time + v_kicktime.value;
}


/*
==================
V_cshift_f
==================
*/
void V_cshift_f (void)
{
	cl.cshifts[CSHIFT_EXTRA].destcolor[0] = atoi (Cmd_Argv (1));
	cl.cshifts[CSHIFT_EXTRA].destcolor[1] = atoi (Cmd_Argv (2));
	cl.cshifts[CSHIFT_EXTRA].destcolor[2] = atoi (Cmd_Argv (3));
	cl.cshifts[CSHIFT_EXTRA].percent = atoi (Cmd_Argv (4));
}


/*
==================
V_BonusFlash_f

When you run over an item, the server sends this command
==================
*/
void V_BonusFlash_f (void)
{
	cl.cshifts[CSHIFT_BONUS].destcolor[0] = 215;
	cl.cshifts[CSHIFT_BONUS].destcolor[1] = 186;
	cl.cshifts[CSHIFT_BONUS].destcolor[2] = 69;
	cl.cshifts[CSHIFT_BONUS].percent = 50;
	cl.cshifts[CSHIFT_BONUS].initialpercent = cl.cshifts[CSHIFT_BONUS].percent;
	cl.cshifts[CSHIFT_BONUS].inittime = cl.time;
}

void V_DarkFlash_f (void)
{
	cl.cshifts[CSHIFT_BONUS].destcolor[0] = 0;
	cl.cshifts[CSHIFT_BONUS].destcolor[1] = 0;
	cl.cshifts[CSHIFT_BONUS].destcolor[2] = 0;
	cl.cshifts[CSHIFT_BONUS].percent = 255;
	cl.cshifts[CSHIFT_BONUS].initialpercent = cl.cshifts[CSHIFT_BONUS].percent;
	cl.cshifts[CSHIFT_BONUS].inittime = cl.time;
}

void V_WhiteFlash_f (void)
{
	cl.cshifts[CSHIFT_BONUS].destcolor[0] = 255;
	cl.cshifts[CSHIFT_BONUS].destcolor[1] = 255;
	cl.cshifts[CSHIFT_BONUS].destcolor[2] = 255;
	cl.cshifts[CSHIFT_BONUS].percent = 255;
	cl.cshifts[CSHIFT_BONUS].initialpercent = cl.cshifts[CSHIFT_BONUS].percent;
	cl.cshifts[CSHIFT_BONUS].inittime = cl.time;
}

/*
=============
V_SetContentsColor

Underwater, lava, etc each has a color shift
=============
*/
void V_SetContentsColor (int contents)
{
	switch (contents)
	{
	case CONTENTS_EMPTY:
	case CONTENTS_SOLID:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_empty;
		break;

	case CONTENTS_LAVA:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_lava;
		break;

	case CONTENTS_SLIME:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_slime;
		break;

	default:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_water;
	}
}

/*
=============
V_CalcPowerupCshift
=============
*/
void V_CalcPowerupCshift (void)
{
	// allowing multiple simultaneous powerups
	if ((int) cl.v.artifact_active & ART_HASTE)
	{
		cl.cshifts[CSHIFT_HASTE].destcolor[0] = 0;
		cl.cshifts[CSHIFT_HASTE].destcolor[1] = 255;
		cl.cshifts[CSHIFT_HASTE].destcolor[2] = 255;
		cl.cshifts[CSHIFT_HASTE].percent = 30;
	}
	else cl.cshifts[CSHIFT_HASTE].percent = 0;

	if ((int) cl.v.artifact_active & ART_TOMEOFPOWER)
	{
		cl.cshifts[CSHIFT_TOMEOFPOWER].destcolor[0] = 255;
		cl.cshifts[CSHIFT_TOMEOFPOWER].destcolor[1] = 0;
		cl.cshifts[CSHIFT_TOMEOFPOWER].destcolor[2] = 255;
		cl.cshifts[CSHIFT_TOMEOFPOWER].percent = 30;
	}
	else cl.cshifts[CSHIFT_TOMEOFPOWER].percent = 0;

	if ((int) cl.v.artifact_active & ARTFLAG_DIVINE_INTERVENTION)
	{
		cl.cshifts[CSHIFT_INTERVENTION].destcolor[0] = 255;
		cl.cshifts[CSHIFT_INTERVENTION].destcolor[1] = 255;
		cl.cshifts[CSHIFT_INTERVENTION].destcolor[2] = 255;
		cl.cshifts[CSHIFT_INTERVENTION].percent = 256;
	}
	else cl.cshifts[CSHIFT_INTERVENTION].percent = 0;

	if ((int) cl.v.artifact_active & ARTFLAG_FROZEN)
	{
		cl.cshifts[CSHIFT_FROZEN].destcolor[0] = 20;
		cl.cshifts[CSHIFT_FROZEN].destcolor[1] = 70;
		cl.cshifts[CSHIFT_FROZEN].destcolor[2] = 255;
		cl.cshifts[CSHIFT_FROZEN].percent = 65;
	}
	else cl.cshifts[CSHIFT_FROZEN].percent = 0;

	if ((int) cl.v.artifact_active & ARTFLAG_STONED)
	{
		cl.cshifts[CSHIFT_STONED].destcolor[0] = 205;
		cl.cshifts[CSHIFT_STONED].destcolor[1] = 205;
		cl.cshifts[CSHIFT_STONED].destcolor[2] = 205;
		cl.cshifts[CSHIFT_STONED].percent = 11000;
	}
	else cl.cshifts[CSHIFT_STONED].percent = 0;

	if ((int) cl.v.artifact_active & ART_INVISIBILITY)
	{
		cl.cshifts[CSHIFT_INVISIBLE].destcolor[0] = 100;
		cl.cshifts[CSHIFT_INVISIBLE].destcolor[1] = 100;
		cl.cshifts[CSHIFT_INVISIBLE].destcolor[2] = 100;
		cl.cshifts[CSHIFT_INVISIBLE].percent = 100;
	}
	else cl.cshifts[CSHIFT_INVISIBLE].percent = 0;

	if ((int) cl.v.artifact_active & ART_INVINCIBILITY)
	{
		cl.cshifts[CSHIFT_INVINCIBLE].destcolor[0] = 255;
		cl.cshifts[CSHIFT_INVINCIBLE].destcolor[1] = 255;
		cl.cshifts[CSHIFT_INVINCIBLE].destcolor[2] = 0;
		cl.cshifts[CSHIFT_INVINCIBLE].percent = 30;
	}
	else cl.cshifts[CSHIFT_INVINCIBLE].percent = 0;
}

/*
=============
V_CalcBlend
=============
*/
void V_CalcBlend (void)
{
	int j;
	float r = 0, g = 0, b = 0, a = 0, a2;

	for (j = 0; j < NUM_CSHIFTS; j++)
	{
		// Set percent for grayscale
		if (cl.cshifts[j].percent > 10000) cl.cshifts[j].percent = 80;

		// don't add cases
		if (!(cl.cshifts[j].percent > 0)) continue;
		if (!(a2 = cl.cshifts[j].percent / 255.0)) continue;

		a = a + a2 * (1 - a);
		a2 = a2 / a;
		r = r * (1 - a2) + cl.cshifts[j].destcolor[0] * a2;
		g = g * (1 - a2) + cl.cshifts[j].destcolor[1] * a2;
		b = b * (1 - a2) + cl.cshifts[j].destcolor[2] * a2;
	}

	v_blend[0] = r / 255.0;
	v_blend[1] = g / 255.0;
	v_blend[2] = b / 255.0;
	v_blend[3] = a;

	if (v_blend[3] > 1) v_blend[3] = 1;
	if (v_blend[3] < 0) v_blend[3] = 0;
}


/*
=============
V_UpdatePalette
=============
*/
void V_UpdatePalette (void)
{
	// provide a red tint when we take damage
	cl.cshifts[CSHIFT_HEALTH].destcolor[0] = 255;
	cl.cshifts[CSHIFT_HEALTH].destcolor[1] = 0;
	cl.cshifts[CSHIFT_HEALTH].destcolor[2] = 0;

	if (cl.v.max_health > 0)
	{
		if (cl.v.health < 0)
			cl.cshifts[CSHIFT_HEALTH].percent = 50;
		else cl.cshifts[CSHIFT_HEALTH].percent = 50 - (cl.v.health / cl.v.max_health) * 50;
	}
	else cl.cshifts[CSHIFT_HEALTH].percent = 0;

	V_CalcPowerupCshift ();

	// this should never happen
	if (cl.cshifts[CSHIFT_DAMAGE].inittime > cl.time) cl.cshifts[CSHIFT_DAMAGE].inittime = cl.time;
	if (cl.cshifts[CSHIFT_BONUS].inittime > cl.time) cl.cshifts[CSHIFT_BONUS].inittime = cl.time;

	// cshift drops are based on absolute time since the shift was initiated; this is to allow SCR_UpdateScreen to be called multiple times between cl.time changes
	cl.cshifts[CSHIFT_DAMAGE].percent = cl.cshifts[CSHIFT_DAMAGE].initialpercent - (cl.time - cl.cshifts[CSHIFT_DAMAGE].inittime) * 150;
	cl.cshifts[CSHIFT_BONUS].percent = cl.cshifts[CSHIFT_BONUS].initialpercent - (cl.time - cl.cshifts[CSHIFT_BONUS].inittime) * 100;

	// and lower-bound it so that the blend calc doesn't mess up
	if (cl.cshifts[CSHIFT_DAMAGE].percent <= 0) cl.cshifts[CSHIFT_DAMAGE].percent = 0;
	if (cl.cshifts[CSHIFT_BONUS].percent <= 0) cl.cshifts[CSHIFT_BONUS].percent = 0;

	// and update the blend
	V_CalcBlend ();
}


/*
==============================================================================

VIEW RENDERING

==============================================================================
*/

float angledelta (float a)
{
	a = anglemod (a);

	if (a > 180)
		a -= 360;

	return a;
}

/*
==================
CalcGunAngle
==================
*/
void CalcGunAngle (void)
{
	float	yaw, pitch, move;
	static float oldyaw = 0;
	static float oldpitch = 0;

	yaw = r_refdef.viewangles[1];
	pitch = -r_refdef.viewangles[0];

	yaw = angledelta (yaw - r_refdef.viewangles[1]) * 0.4;

	if (yaw > 10)
		yaw = 10;

	if (yaw < -10)
		yaw = -10;

	pitch = angledelta (-pitch - r_refdef.viewangles[0]) * 0.4;

	if (pitch > 10)
		pitch = 10;

	if (pitch < -10)
		pitch = -10;

	move = (cl.time - cl.oldtime) * 20;

	if (yaw > oldyaw)
	{
		if (oldyaw + move < yaw)
			yaw = oldyaw + move;
	}
	else
	{
		if (oldyaw - move > yaw)
			yaw = oldyaw - move;
	}

	if (pitch > oldpitch)
	{
		if (oldpitch + move < pitch)
			pitch = oldpitch + move;
	}
	else
	{
		if (oldpitch - move > pitch)
			pitch = oldpitch - move;
	}

	oldyaw = yaw;
	oldpitch = pitch;

	cl.viewent.angles[1] = r_refdef.viewangles[1] + yaw;
	cl.viewent.angles[0] = -(r_refdef.viewangles[0] + pitch);

	cl.viewent.angles[2] -= v_idlescale.value * sin (cl.time * v_iroll_cycle.value) * v_iroll_level.value;
	cl.viewent.angles[0] -= v_idlescale.value * sin (cl.time * v_ipitch_cycle.value) * v_ipitch_level.value;
	cl.viewent.angles[1] -= v_idlescale.value * sin (cl.time * v_iyaw_cycle.value) * v_iyaw_level.value;
}

/*
==============
V_BoundOffsets
==============
*/
void V_BoundOffsets (void)
{
	entity_t	*ent;

	ent = &cl_entities[cl.viewentity];

	// absolutely bound refresh reletive to entity clipping hull
	// so the view can never be inside a solid wall

	if (r_refdef.vieworigin[0] < ent->origin[0] - 14)
		r_refdef.vieworigin[0] = ent->origin[0] - 14;
	else if (r_refdef.vieworigin[0] > ent->origin[0] + 14)
		r_refdef.vieworigin[0] = ent->origin[0] + 14;

	if (r_refdef.vieworigin[1] < ent->origin[1] - 14)
		r_refdef.vieworigin[1] = ent->origin[1] - 14;
	else if (r_refdef.vieworigin[1] > ent->origin[1] + 14)
		r_refdef.vieworigin[1] = ent->origin[1] + 14;

	if (r_refdef.vieworigin[2] < ent->origin[2] - 0)
		r_refdef.vieworigin[2] = ent->origin[2] - 0;
	else if (r_refdef.vieworigin[2] > ent->origin[2] + 86)
		r_refdef.vieworigin[2] = ent->origin[2] + 86;
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
void V_AddIdle (void)
{
	r_refdef.viewangles[2] += v_idlescale.value * sin (cl.time * v_iroll_cycle.value) * v_iroll_level.value;
	r_refdef.viewangles[0] += v_idlescale.value * sin (cl.time * v_ipitch_cycle.value) * v_ipitch_level.value;
	r_refdef.viewangles[1] += v_idlescale.value * sin (cl.time * v_iyaw_cycle.value) * v_iyaw_level.value;
}


/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
void V_CalcViewRoll (void)
{
	float	side;

	if (v_kicktime.value)
	{
		side = V_CalcRoll (cl_entities[cl.viewentity].angles, cl.velocity);
		r_refdef.viewangles[2] += side;

		if (v_dmg_time > cl.time)
		{
			r_refdef.viewangles[2] += (v_dmg_time - cl.time) / v_kicktime.value * v_dmg_roll;
			r_refdef.viewangles[0] += (v_dmg_time - cl.time) / v_kicktime.value * v_dmg_pitch;
		}
	}

	if (cl.v.health <= 0)
	{
		r_refdef.viewangles[2] = 80;	// dead view angle
		return;
	}
}


/*
==================
V_CalcIntermissionRefdef

==================
*/
void V_CalcIntermissionRefdef (void)
{
	entity_t	*ent, *view;
	float		old;

	// ent is the player model (visible when out of body)
	ent = &cl_entities[cl.viewentity];
	// view is the weapon model (only visible from inside body)
	view = &cl.viewent;

	VectorCopy (ent->origin, r_refdef.vieworigin);
	VectorCopy (ent->angles, r_refdef.viewangles);
	view->model = NULL;
	r_refdef.vieworigin[2] += cl.viewheight;

	// allways idle in intermission
	old = v_idlescale.value;
	v_idlescale.value = 1;
	V_AddIdle ();
	v_idlescale.value = old;
}

/*
==================
V_CalcRefdef

==================
*/
void V_CalcRefdef (void)
{
	entity_t	*ent, *view;
	int			i;
	vec3_t		forward, right, up;
	vec3_t		angles;
	float		bob;
	static float oldz = 0;

	// ent is the player model (visible when out of body)
	ent = &cl_entities[cl.viewentity];

	// view is the weapon model (only visible from inside body)
	view = &cl.viewent;

	// transform the view offset by the model's matrix to get the offset from
	// model origin for the view
	ent->angles[1] = cl.viewangles[1];	// the model should face the view dir
	ent->angles[0] = -cl.viewangles[0];	// the model should face the view dir

	if (cl.v.movetype != MOVETYPE_FLY)
		bob = V_CalcBob ();
	else  // no bobbing when you fly
		bob = 1;

	// refresh position
	VectorCopy (ent->origin, r_refdef.vieworigin);
	r_refdef.vieworigin[2] += cl.viewheight + bob;

	// never let it sit exactly on a node line, because a water plane can
	// dissapear when viewed with the eye exactly on it.
	// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	r_refdef.vieworigin[0] += 1.0 / 32;
	r_refdef.vieworigin[1] += 1.0 / 32;
	r_refdef.vieworigin[2] += 1.0 / 32;

	VectorCopy (cl.viewangles, r_refdef.viewangles);
	V_CalcViewRoll ();
	V_AddIdle ();

	// offsets
	angles[0] = -ent->angles[0];	// because entity pitches are actually backward
	angles[1] = ent->angles[1];
	angles[2] = ent->angles[2];

	AngleVectors (angles, forward, right, up);

	for (i = 0; i < 3; i++)
		r_refdef.vieworigin[i] += scr_ofsx.value * forward[i] + scr_ofsy.value * right[i] + scr_ofsz.value * up[i];

	V_BoundOffsets ();

	// set up gun position
	VectorCopy (cl.viewangles, view->angles);

	CalcGunAngle ();

	VectorCopy (ent->origin, view->origin);
	view->origin[2] += cl.viewheight;

	for (i = 0; i < 3; i++)
	{
		view->origin[i] += forward[i] * bob * 0.4;
		//		view->origin[i] += right[i]*bob*0.4;
		//		view->origin[i] += up[i]*bob*0.8;
	}

	view->origin[2] += bob;

	// fudge position around to keep amount of weapon visible
	// roughly equal with different FOV
	view->origin[2] += 1.5;

	view->model = cl.model_precache[cl.stats[STAT_WEAPON]];
	view->frame = cl.stats[STAT_WEAPONFRAME];
	view->colormap = vid.colormap;

	// Place weapon in powered up mode
	if ((ent->drawflags & MLS_MASKIN) == MLS_POWERMODE)
		view->drawflags = (view->drawflags & MLS_MASKOUT) | MLS_POWERMODE;
	else
		view->drawflags = (view->drawflags & MLS_MASKOUT) | 0;

	// set up the refresh position
	VectorAdd (r_refdef.viewangles, cl.punchangle, r_refdef.viewangles);

	// smooth out stair step ups
	if (cl.onground && ent->origin[2] - v_newstepz > 0)
	{
		v_newstepz = v_oldstepz + (cl.time - v_stepchangetime) * 160;

		if (v_newstepz > ent->origin[2])
		{
			v_stepchangetime = cl.time;
			v_newstepz = v_oldstepz = ent->origin[2];
		}

		if (ent->origin[2] - v_newstepz > 12)
		{
			v_stepchangetime = cl.time;
			v_newstepz = v_oldstepz = ent->origin[2] - 12;
		}

		r_refdef.vieworigin[2] += v_newstepz - ent->origin[2];
		view->origin[2] += v_newstepz - ent->origin[2];
	}
	else
	{
		v_oldstepz = v_newstepz = ent->origin[2];
		v_stepchangetime = cl.time;
	}
}


/*
==================
V_RenderView

The player's clipping box goes from (-16 -16 -24) to (16 16 32) from
the entity origin, so any view position inside that will be valid
==================
*/
void V_RenderView (void)
{
	if (con_forcedup)
		return;

	// don't allow cheats in multiplayer
	if (cl.maxclients > 1)
	{
		Cvar_Set ("scr_ofsx", "0");
		Cvar_Set ("scr_ofsy", "0");
		Cvar_Set ("scr_ofsz", "0");
	}

	if (cl.intermission)
	{
		// intermission / finale rendering
		V_CalcIntermissionRefdef ();
	}
	else
	{
		if (!cl.paused /* && (sv.maxclients > 1 || key_dest == key_game) */)
			V_CalcRefdef ();
	}

	R_RenderView ();
}


//============================================================================

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	Cmd_AddCommand ("v_cshift", V_cshift_f);
	Cmd_AddCommand ("bf", V_BonusFlash_f);
	Cmd_AddCommand ("df", V_DarkFlash_f);
	Cmd_AddCommand ("wf", V_WhiteFlash_f);

	Cvar_RegisterVariable (&v_iyaw_cycle, NULL);
	Cvar_RegisterVariable (&v_iroll_cycle, NULL);
	Cvar_RegisterVariable (&v_ipitch_cycle, NULL);
	Cvar_RegisterVariable (&v_iyaw_level, NULL);
	Cvar_RegisterVariable (&v_iroll_level, NULL);
	Cvar_RegisterVariable (&v_ipitch_level, NULL);

	Cvar_RegisterVariable (&v_idlescale, NULL);
	Cvar_RegisterVariable (&crosshair, NULL);
	Cvar_RegisterVariable (&crosshaircolor, NULL);
	Cvar_RegisterVariable (&cl_crossx, NULL);
	Cvar_RegisterVariable (&cl_crossy, NULL);

	Cvar_RegisterVariable (&scr_ofsx, NULL);
	Cvar_RegisterVariable (&scr_ofsy, NULL);
	Cvar_RegisterVariable (&scr_ofsz, NULL);
	Cvar_RegisterVariable (&cl_rollspeed, NULL);
	Cvar_RegisterVariable (&cl_rollangle, NULL);
	Cvar_RegisterVariable (&cl_bob, NULL);
	Cvar_RegisterVariable (&cl_bobcycle, NULL);
	Cvar_RegisterVariable (&cl_bobup, NULL);

	Cvar_RegisterVariable (&v_kicktime, NULL);
	Cvar_RegisterVariable (&v_kickroll, NULL);
	Cvar_RegisterVariable (&v_kickpitch, NULL);

	Cvar_RegisterVariable (&v_gamma, NULL);
}


void V_NewMap (void)
{
	int i;

	for (i = 0; i < NUM_CSHIFTS; i++)
	{
		cl.cshifts[i].initialpercent = 0;
		cl.cshifts[i].inittime = 0;
		cl.cshifts[i].percent = 0;
	}

	v_dmg_time = 0;
	v_oldstepz = v_newstepz = 0;
	v_stepchangetime = 0;
}

