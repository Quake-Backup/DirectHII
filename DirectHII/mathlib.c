/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include "precompiled.h"

#include "matrix.h"
#include "mathlib.h"

#include <xmmintrin.h>
#include <smmintrin.h>


void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	angle = angles[1] * (float) (M_PI * 2 / 360);

	sy = sinf (angle);
	cy = cosf (angle);

	angle = angles[0] * (float) (M_PI * 2 / 360);

	sp = sinf (angle);
	cp = cosf (angle);

	angle = angles[2] * (float) (M_PI * 2 / 360);

	sr = sinf (angle);
	cr = cosf (angle);

	forward[0] = cp * cy;
	forward[1] = cp * sy;
	forward[2] = -sp;

	right[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
	right[1] = (-1 * sr * sp * sy + -1 * cr * cy);
	right[2] = -1 * sr * cp;

	up[0] = (cr * sp * cy + -sr * -sy);
	up[1] = (cr * sp * sy + -sr * cy);
	up[2] = cr * cp;
}


float anglemod (float a)
{
	a = (360.0f / 65536.0f) * ((int) (a * (65536.0f / 360.0f)) & 65535);
	return a;
}


/*
=======================================================

My functions

Note: these are all "left-to-right" functions whereas Quake's originals were "right-to-left"; I prefer "left-to-right" because it matches assignment and precedent order in C

=======================================================
*/

// common utility funcs
float SafeSqrt (float in)
{
	if (in > 0)
		return sqrtf (in);
	else return 0;
}


float *float2 (float a, float b)
{
	static float f[2];

	f[0] = a;
	f[1] = b;

	return f;
}


float *float3 (float a, float b, float c)
{
	static float f[3];

	f[0] = a;
	f[1] = b;
	f[2] = c;

	return f;
}


float *float4 (float a, float b, float c, float d)
{
	static float f[4];

	f[0] = a;
	f[1] = b;
	f[2] = c;
	f[3] = d;

	return f;
}


// vector functions from directq
void Vector2Madf (float *out, const float *vec, const float scale, const float *add)
{
	out[0] = (float) ((double) vec[0] * (double) scale + (double) add[0]);
	out[1] = (float) ((double) vec[1] * (double) scale + (double) add[1]);
}


void Vector2Mad (float *out, const float *vec, const float *scale, const float *add)
{
	out[0] = (float) ((double) vec[0] * (double) scale[0] + (double) add[0]);
	out[1] = (float) ((double) vec[1] * (double) scale[1] + (double) add[1]);
}


void Vector3Madf (float *out, const float *vec, const float scale, const float *add)
{
	out[0] = (float) ((double) vec[0] * (double) scale + (double) add[0]);
	out[1] = (float) ((double) vec[1] * (double) scale + (double) add[1]);
	out[2] = (float) ((double) vec[2] * (double) scale + (double) add[2]);
}


void Vector3Mad (float *out, const float *vec, const float *scale, const float *add)
{
	out[0] = (float) ((double) vec[0] * (double) scale[0] + (double) add[0]);
	out[1] = (float) ((double) vec[1] * (double) scale[1] + (double) add[1]);
	out[2] = (float) ((double) vec[2] * (double) scale[2] + (double) add[2]);
}


void Vector4Madf (float *out, const float *vec, const float scale, const float *add)
{
	out[0] = (float) ((double) vec[0] * (double) scale + (double) add[0]);
	out[1] = (float) ((double) vec[1] * (double) scale + (double) add[1]);
	out[2] = (float) ((double) vec[2] * (double) scale + (double) add[2]);
	out[3] = (float) ((double) vec[3] * (double) scale + (double) add[3]);
}


void Vector4Mad (float *out, const float *vec, const float *scale, const float *add)
{
	out[0] = (float) ((double) vec[0] * (double) scale[0] + (double) add[0]);
	out[1] = (float) ((double) vec[1] * (double) scale[1] + (double) add[1]);
	out[2] = (float) ((double) vec[2] * (double) scale[2] + (double) add[2]);
	out[3] = (float) ((double) vec[3] * (double) scale[3] + (double) add[3]);
}


void Vector2Scalef (float *dst, const float *vec, const float scale)
{
	dst[0] = (float) ((double) vec[0] * (double) scale);
	dst[1] = (float) ((double) vec[1] * (double) scale);
}


void Vector2Scale (float *dst, const float *vec, const float *scale)
{
	dst[0] = (float) ((double) vec[0] * (double) scale[0]);
	dst[1] = (float) ((double) vec[1] * (double) scale[1]);
}


void Vector3Scalef (float *dst, const float *vec, const float scale)
{
	dst[0] = (float) ((double) vec[0] * (double) scale);
	dst[1] = (float) ((double) vec[1] * (double) scale);
	dst[2] = (float) ((double) vec[2] * (double) scale);
}


void Vector3Scale (float *dst, const float *vec, const float *scale)
{
	dst[0] = (float) ((double) vec[0] * (double) scale[0]);
	dst[1] = (float) ((double) vec[1] * (double) scale[1]);
	dst[2] = (float) ((double) vec[2] * (double) scale[2]);
}


void Vector4Scalef (float *dst, const float *vec, const float scale)
{
	dst[0] = (float) ((double) vec[0] * (double) scale);
	dst[1] = (float) ((double) vec[1] * (double) scale);
	dst[2] = (float) ((double) vec[2] * (double) scale);
	dst[3] = (float) ((double) vec[3] * (double) scale);
}


void Vector4Scale (float *dst, const float *vec, const float *scale)
{
	dst[0] = (float) ((double) vec[0] * (double) scale[0]);
	dst[1] = (float) ((double) vec[1] * (double) scale[1]);
	dst[2] = (float) ((double) vec[2] * (double) scale[2]);
	dst[3] = (float) ((double) vec[3] * (double) scale[3]);
}


void Vector2Recipf (float *dst, const float *vec, const float scale)
{
	dst[0] = (float) ((double) vec[0] / (double) scale);
	dst[1] = (float) ((double) vec[1] / (double) scale);
}


void Vector2Recip (float *dst, const float *vec, const float *scale)
{
	dst[0] = (float) ((double) vec[0] / (double) scale[0]);
	dst[1] = (float) ((double) vec[1] / (double) scale[1]);
}


void Vector3Recipf (float *dst, const float *vec, const float scale)
{
	dst[0] = (float) ((double) vec[0] / (double) scale);
	dst[1] = (float) ((double) vec[1] / (double) scale);
	dst[2] = (float) ((double) vec[2] / (double) scale);
}


void Vector3Recip (float *dst, const float *vec, const float *scale)
{
	dst[0] = (float) ((double) vec[0] / (double) scale[0]);
	dst[1] = (float) ((double) vec[1] / (double) scale[1]);
	dst[2] = (float) ((double) vec[2] / (double) scale[2]);
}


void Vector4Recipf (float *dst, const float *vec, const float scale)
{
	dst[0] = (float) ((double) vec[0] / (double) scale);
	dst[1] = (float) ((double) vec[1] / (double) scale);
	dst[2] = (float) ((double) vec[2] / (double) scale);
	dst[3] = (float) ((double) vec[3] / (double) scale);
}


void Vector4Recip (float *dst, const float *vec, const float *scale)
{
	dst[0] = (float) ((double) vec[0] / (double) scale[0]);
	dst[1] = (float) ((double) vec[1] / (double) scale[1]);
	dst[2] = (float) ((double) vec[2] / (double) scale[2]);
	dst[3] = (float) ((double) vec[3] / (double) scale[3]);
}


void Vector2Copy (float *dst, const float *src)
{
	dst[0] = src[0];
	dst[1] = src[1];
}


void Vector3Copy (float *dst, const float *src)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
}


void Vector4Copy (float *dst, const float *src)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	dst[3] = src[3];
}


void Vector2Addf (float *dst, const float *vec1, const float add)
{
	dst[0] = (float) ((double) vec1[0] + (double) add);
	dst[1] = (float) ((double) vec1[1] + (double) add);
}


void Vector2Add (float *dst, const float *vec1, const float *vec2)
{
	dst[0] = (float) ((double) vec1[0] + (double) vec2[0]);
	dst[1] = (float) ((double) vec1[1] + (double) vec2[1]);
}


void Vector2Subtractf (float *dst, const float *vec1, const float sub)
{
	dst[0] = (float) ((double) vec1[0] - (double) sub);
	dst[1] = (float) ((double) vec1[1] - (double) sub);
}


void Vector2Subtract (float *dst, const float *vec1, const float *vec2)
{
	dst[0] = (float) ((double) vec1[0] - (double) vec2[0]);
	dst[1] = (float) ((double) vec1[1] - (double) vec2[1]);
}


void Vector3Addf (float *dst, const float *vec1, const float add)
{
	dst[0] = (float) ((double) vec1[0] + (double) add);
	dst[1] = (float) ((double) vec1[1] + (double) add);
	dst[2] = (float) ((double) vec1[2] + (double) add);
}


void Vector3Add (float *dst, const float *vec1, const float *vec2)
{
	dst[0] = (float) ((double) vec1[0] + (double) vec2[0]);
	dst[1] = (float) ((double) vec1[1] + (double) vec2[1]);
	dst[2] = (float) ((double) vec1[2] + (double) vec2[2]);
}


void Vector3Subtractf (float *dst, const float *vec1, const float sub)
{
	dst[0] = (float) ((double) vec1[0] - (double) sub);
	dst[1] = (float) ((double) vec1[1] - (double) sub);
	dst[2] = (float) ((double) vec1[2] - (double) sub);
}


void Vector3Subtract (float *dst, const float *vec1, const float *vec2)
{
	dst[0] = (float) ((double) vec1[0] - (double) vec2[0]);
	dst[1] = (float) ((double) vec1[1] - (double) vec2[1]);
	dst[2] = (float) ((double) vec1[2] - (double) vec2[2]);
}


void Vector4Addf (float *dst, const float *vec1, const float add)
{
	dst[0] = (float) ((double) vec1[0] + (double) add);
	dst[1] = (float) ((double) vec1[1] + (double) add);
	dst[2] = (float) ((double) vec1[2] + (double) add);
	dst[3] = (float) ((double) vec1[3] + (double) add);
}


void Vector4Add (float *dst, const float *vec1, const float *vec2)
{
	dst[0] = (float) ((double) vec1[0] + (double) vec2[0]);
	dst[1] = (float) ((double) vec1[1] + (double) vec2[1]);
	dst[2] = (float) ((double) vec1[2] + (double) vec2[2]);
	dst[3] = (float) ((double) vec1[3] + (double) vec2[3]);
}


void Vector4Subtractf (float *dst, const float *vec1, const float sub)
{
	dst[0] = (float) ((double) vec1[0] - (double) sub);
	dst[1] = (float) ((double) vec1[1] - (double) sub);
	dst[2] = (float) ((double) vec1[2] - (double) sub);
	dst[3] = (float) ((double) vec1[3] - (double) sub);
}


void Vector4Subtract (float *dst, const float *vec1, const float *vec2)
{
	dst[0] = (float) ((double) vec1[0] - (double) vec2[0]);
	dst[1] = (float) ((double) vec1[1] - (double) vec2[1]);
	dst[2] = (float) ((double) vec1[2] - (double) vec2[2]);
	dst[3] = (float) ((double) vec1[3] - (double) vec2[3]);
}


float Vector2Dot (const float *x, const float *y)
{
	return (float) (((double) x[0] * (double) y[0]) + ((double) x[1] * (double) y[1]));
}


float Vector3Dot (const float *x, const float *y)
{
	// fix up math optimizations that screw things up in Quake
	return (float) (((double) x[0] * (double) y[0]) + ((double) x[1] * (double) y[1]) + ((double) x[2] * (double) y[2]));
}


float Vector4Dot (const float *x, const float *y)
{
	return (float) (((double) x[0] * (double) y[0]) + ((double) x[1] * (double) y[1]) + ((double) x[2] * (double) y[2]) + ((double) x[3] * (double) y[3]));
}


void Vector2Lerpf (float *dst, const float *l1, const float *l2, const float b)
{
	dst[0] = (float) ((double) l1[0] + ((double) l2[0] - (double) l1[0]) * (double) b);
	dst[1] = (float) ((double) l1[1] + ((double) l2[1] - (double) l1[1]) * (double) b);
}


void Vector3Lerpf (float *dst, const float *l1, const float *l2, const float b)
{
	dst[0] = (float) ((double) l1[0] + ((double) l2[0] - (double) l1[0]) * (double) b);
	dst[1] = (float) ((double) l1[1] + ((double) l2[1] - (double) l1[1]) * (double) b);
	dst[2] = (float) ((double) l1[2] + ((double) l2[2] - (double) l1[2]) * (double) b);
}


void Vector4Lerpf (float *dst, const float *l1, const float *l2, const float b)
{
	dst[0] = (float) ((double) l1[0] + ((double) l2[0] - (double) l1[0]) * (double) b);
	dst[1] = (float) ((double) l1[1] + ((double) l2[1] - (double) l1[1]) * (double) b);
	dst[2] = (float) ((double) l1[2] + ((double) l2[2] - (double) l1[2]) * (double) b);
	dst[3] = (float) ((double) l1[3] + ((double) l2[3] - (double) l1[3]) * (double) b);
}


void Vector2Lerp (float *dst, const float *l1, const float *l2, const float *b)
{
	dst[0] = (float) ((double) l1[0] + ((double) l2[0] - (double) l1[0]) * (double) b[0]);
	dst[1] = (float) ((double) l1[1] + ((double) l2[1] - (double) l1[1]) * (double) b[1]);
}


void Vector3Lerp (float *dst, const float *l1, const float *l2, const float *b)
{
	dst[0] = (float) ((double) l1[0] + ((double) l2[0] - (double) l1[0]) * (double) b[0]);
	dst[1] = (float) ((double) l1[1] + ((double) l2[1] - (double) l1[1]) * (double) b[1]);
	dst[2] = (float) ((double) l1[2] + ((double) l2[2] - (double) l1[2]) * (double) b[2]);
}


void Vector4Lerp (float *dst, const float *l1, const float *l2, const float *b)
{
	dst[0] = (float) ((double) l1[0] + ((double) l2[0] - (double) l1[0]) * (double) b[0]);
	dst[1] = (float) ((double) l1[1] + ((double) l2[1] - (double) l1[1]) * (double) b[1]);
	dst[2] = (float) ((double) l1[2] + ((double) l2[2] - (double) l1[2]) * (double) b[2]);
	dst[3] = (float) ((double) l1[3] + ((double) l2[3] - (double) l1[3]) * (double) b[3]);
}


void Vector2Set (float *vec, const float x, const float y)
{
	vec[0] = x;
	vec[1] = y;
}


void Vector3Set (float *vec, const float x, const float y, const float z)
{
	vec[0] = x;
	vec[1] = y;
	vec[2] = z;
}


void Vector4Set (float *vec, const float x, const float y, const float z, const float w)
{
	vec[0] = x;
	vec[1] = y;
	vec[2] = z;
	vec[3] = w;
}


void Vector2Clear (float *vec)
{
	vec[0] = vec[1] = 0.0f;
}


void Vector3Clear (float *vec)
{
	vec[0] = vec[1] = vec[2] = 0.0f;
}


void Vector4Clear (float *vec)
{
	vec[0] = vec[1] = vec[2] = vec[3] = 0.0f;
}


void Vector2Clamp (float *vec, const float clmp)
{
	if (vec[0] > clmp) vec[0] = clmp;
	if (vec[1] > clmp) vec[1] = clmp;
}


void Vector3Clamp (float *vec, const float clmp)
{
	if (vec[0] > clmp) vec[0] = clmp;
	if (vec[1] > clmp) vec[1] = clmp;
	if (vec[2] > clmp) vec[2] = clmp;
}


void Vector4Clamp (float *vec, const float clmp)
{
	if (vec[0] > clmp) vec[0] = clmp;
	if (vec[1] > clmp) vec[1] = clmp;
	if (vec[2] > clmp) vec[2] = clmp;
	if (vec[3] > clmp) vec[3] = clmp;
}


void Vector2Cross (float *cross, const float *v1, const float *v2)
{
	// Sys_Error ("Just what do you think you're doing, Dave?");
}


void Vector3Cross (float *cross, const float *v1, const float *v2)
{
	cross[0] = (float) ((double) v1[1] * (double) v2[2] - (double) v1[2] * (double) v2[1]);
	cross[1] = (float) ((double) v1[2] * (double) v2[0] - (double) v1[0] * (double) v2[2]);
	cross[2] = (float) ((double) v1[0] * (double) v2[1] - (double) v1[1] * (double) v2[0]);
}


void Vector4Cross (float *cross, const float *v1, const float *v2)
{
	// Sys_Error ("Just what do you think you're doing, Dave?");
}


float Vector2Length (const float *v)
{
	return SafeSqrt (Vector2Dot (v, v));
}


float Vector3Length (const float *v)
{
	return SafeSqrt (Vector3Dot (v, v));
}


float Vector4Length (const float *v)
{
	return SafeSqrt (Vector4Dot (v, v));
}


float Vector2Normalize (float *v)
{
	double length = (double) Vector2Dot (v, v);

	if ((length = (double) SafeSqrt ((float) length)) > 0)
	{
		double ilength = 1.0 / length;

		v[0] = (float) ((double) v[0] * ilength);
		v[1] = (float) ((double) v[1] * ilength);
	}

	return (float) length;
}


float Vector3Normalize (float *v)
{
	double length = (double) Vector3Dot (v, v);

	if ((length = (double) SafeSqrt ((float) length)) > 0)
	{
		double ilength = 1.0 / length;

		v[0] = (float) ((double) v[0] * ilength);
		v[1] = (float) ((double) v[1] * ilength);
		v[2] = (float) ((double) v[2] * ilength);
	}

	return (float) length;
}


float Vector4Normalize (float *v)
{
	double length = (double) Vector4Dot (v, v);

	if ((length = (double) SafeSqrt ((float) length)) > 0)
	{
		double ilength = 1.0 / length;

		v[0] = (float) ((double) v[0] * ilength);
		v[1] = (float) ((double) v[1] * ilength);
		v[2] = (float) ((double) v[2] * ilength);
		v[3] = (float) ((double) v[3] * ilength);
	}

	return (float) length;
}


qboolean Vector2Compare (const float *v1, const float *v2)
{
	if (v1[0] != v2[0]) return false;
	if (v1[1] != v2[1]) return false;

	return true;
}


qboolean Vector3Compare (const float *v1, const float *v2)
{
	if (v1[0] != v2[0]) return false;
	if (v1[1] != v2[1]) return false;
	if (v1[2] != v2[2]) return false;

	return true;
}


qboolean Vector4Compare (const float *v1, const float *v2)
{
	if (v1[0] != v2[0]) return false;
	if (v1[1] != v2[1]) return false;
	if (v1[2] != v2[2]) return false;
	if (v1[3] != v2[3]) return false;

	return true;
}


float fclamp (const float val, const float mins, const float maxs)
{
	if (val < mins)
		return mins;
	else if (val > maxs)
		return maxs;
	else return val;
}


float Q_fmin (const float _1, const float _2)
{
	return ((_1 < _2) ? _1 : _2);
}


float Q_fmax (const float _1, const float _2)
{
	return ((_1 > _2) ? _1 : _2);
}


int Q_rint (const float x)
{
	return (x > 0) ? (int) (x + 0.5) : (int) (x - 0.5);
}


float Vector2Dist (const float *v1, const float *v2)
{
	float dist[2];

	Vector2Subtract (dist, v1, v2);

	return Vector2Length (dist);
}


float Vector3Dist (const float *v1, const float *v2)
{
	float dist[3];

	Vector3Subtract (dist, v1, v2);

	return Vector3Length (dist);
}


float Vector4Dist (const float *v1, const float *v2)
{
	float dist[4];

	Vector4Subtract (dist, v1, v2);

	return Vector4Length (dist);
}


float Vector2SquaredDist (const float *v1, const float *v2)
{
	float dist[2];

	Vector2Subtract (dist, v1, v2);

	return Vector2Dot (dist, dist);
}


float Vector3SquaredDist (const float *v1, const float *v2)
{
	float dist[3];

	Vector3Subtract (dist, v1, v2);

	return Vector3Dot (dist, dist);
}


float Vector4SquaredDist (const float *v1, const float *v2)
{
	float dist[4];

	Vector4Subtract (dist, v1, v2);

	return Vector4Dot (dist, dist);
}


void Vector2Inverse (float *v)
{
	v[0] = -v[0];
	v[1] = -v[1];
}


void Vector3Inverse (float *v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}


void Vector4Inverse (float *v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
	v[3] = -v[3];
}


// quake shit function compat
float fastfabs (float _X)
{
	*((long *) &_X) &= 0x7fffffff;
	return (_X);
}


/*
=======================================================

right-to-left versions - these are provided for functions which are used so much in the code that it would be prohibitive
to change them all; they exist for (1) disambiguating the params, and (2) up-casting to double in the maths

=======================================================
*/

void VectorScale (vec3_t in, vec_t scale, vec3_t out)
{
	Vector3Scalef (out, in, scale);
}


void VectorCopy (float *in, float *out)
{
	Vector3Copy (out, in);
}


void VectorAdd (float *vec1, float *vec2, float *out)
{
	Vector3Add (out, vec1, vec2);
}


void VectorSubtract (float *vec1, float *vec2, float *out)
{
	Vector3Subtract (out, vec1, vec2);
}


/*
=======================================================

other utility stuff which just fits hadily here

=======================================================
*/

float R_GetFarClip (void)
{
	// this provides the maximum far clip per quake protocol limits
	float mins[3] = {-4096, -4096, -4096};
	float maxs[3] = {4096, 4096, 4096};
	return Vector3Dist (mins, maxs);
}


