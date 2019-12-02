// mathlib.h
#pragma once

#ifndef qboolean_defined
#define qboolean_defined
typedef enum {false, true}	qboolean;
#endif

#define max2(a, b) (((a) > (b)) ? (a) : (b))
#define min2(a, b) (((a) < (b)) ? (a) : (b))
#define max3(a, b, c) (max2 (max2 (a, b), c))
#define min3(a, b, c) (min2 (min2 (a, b), c))

typedef float vec_t;
typedef vec_t vec3_t[3];

extern	vec3_t	vec3_origin;

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif


#define nanmask (255 << 23)
#define	IS_NAN(x) (((*(int *) &x) & nanmask) == nanmask)


#define DEG2RAD(a) ((a) * M_PI) / 180.0
#define RAD2DEG(a) ((a) * 180.0) / M_PI

void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
float anglemod (float a);

float SafeSqrt (float in);
float *float2 (float a, float b);
float *float3 (float a, float b, float c);
float *float4 (float a, float b, float c, float d);

void Vector2Madf (float *out, const float *vec, const float scale, const float *add);
void Vector2Mad (float *out, const float *vec, const float *scale, const float *add);
void Vector3Madf (float *out, const float *vec, const float scale, const float *add);
void Vector3Mad (float *out, const float *vec, const float *scale, const float *add);
void Vector4Madf (float *out, const float *vec, const float scale, const float *add);
void Vector4Mad (float *out, const float *vec, const float *scale, const float *add);

void Vector2Scalef (float *dst, const float *vec, const float scale);
void Vector2Scale (float *dst, const float *vec, const float *scale);
void Vector3Scalef (float *dst, const float *vec, const float scale);
void Vector3Scale (float *dst, const float *vec, const float *scale);
void Vector4Scalef (float *dst, const float *vec, const float scale);
void Vector4Scale (float *dst, const float *vec, const float *scale);

void Vector2Recipf (float *dst, const float *vec, const float scale);
void Vector2Recip (float *dst, const float *vec, const float *scale);
void Vector3Recipf (float *dst, const float *vec, const float scale);
void Vector3Recip (float *dst, const float *vec, const float *scale);
void Vector4Recipf (float *dst, const float *vec, const float scale);
void Vector4Recip (float *dst, const float *vec, const float *scale);

void Vector2Copy (float *dst, const float *src);
void Vector3Copy (float *dst, const float *src);
void Vector4Copy (float *dst, const float *src);

void Vector2Addf (float *dst, const float *vec1, const float add);
void Vector2Add (float *dst, const float *vec1, const float *vec2);
void Vector3Addf (float *dst, const float *vec1, const float add);
void Vector3Add (float *dst, const float *vec1, const float *vec2);
void Vector4Addf (float *dst, const float *vec1, const float add);
void Vector4Add (float *dst, const float *vec1, const float *vec2);

void Vector2Subtractf (float *dst, const float *vec1, const float sub);
void Vector2Subtract (float *dst, const float *vec1, const float *vec2);
void Vector3Subtractf (float *dst, const float *vec1, const float sub);
void Vector3Subtract (float *dst, const float *vec1, const float *vec2);
void Vector4Subtractf (float *dst, const float *vec1, const float sub);
void Vector4Subtract (float *dst, const float *vec1, const float *vec2);

float Vector2Dot (const float *x, const float *y);
float Vector3Dot (const float *x, const float *y);
float Vector4Dot (const float *x, const float *y);

void Vector2Lerpf (float *dst, const float *l1, const float *l2, const float b);
void Vector3Lerpf (float *dst, const float *l1, const float *l2, const float b);
void Vector4Lerpf (float *dst, const float *l1, const float *l2, const float b);
void Vector2Lerp (float *dst, const float *l1, const float *l2, const float *b);
void Vector3Lerp (float *dst, const float *l1, const float *l2, const float *b);
void Vector4Lerp (float *dst, const float *l1, const float *l2, const float *b);

void Vector2Set (float *vec, const float x, const float y);
void Vector3Set (float *vec, const float x, const float y, const float z);
void Vector4Set (float *vec, const float x, const float y, const float z, const float w);

void Vector2Clear (float *vec);
void Vector3Clear (float *vec);
void Vector4Clear (float *vec);

void Vector2Clamp (float *vec, const float clmp);
void Vector3Clamp (float *vec, const float clmp);
void Vector4Clamp (float *vec, const float clmp);

void Vector2Cross (float *cross, const float *v1, const float *v2);
void Vector3Cross (float *cross, const float *v1, const float *v2);
void Vector4Cross (float *cross, const float *v1, const float *v2);

float Vector2Length (const float *v);
float Vector3Length (const float *v);
float Vector4Length (const float *v);

float Vector2Normalize (float *v);
float Vector3Normalize (float *v);
float Vector4Normalize (float *v);

qboolean Vector2Compare (const float *v1, const float *v2);
qboolean Vector3Compare (const float *v1, const float *v2);
qboolean Vector4Compare (const float *v1, const float *v2);

float fclamp (const float val, const float mins, const float maxs);
float fmin (const float _1, const float _2);
float fmax (const float _1, const float _2);

int Q_rint (const float x);

float Vector2Dist (const float *v1, const float *v2);
float Vector3Dist (const float *v1, const float *v2);
float Vector4Dist (const float *v1, const float *v2);

float Vector2SquaredDist (const float *v1, const float *v2);
float Vector3SquaredDist (const float *v1, const float *v2);
float Vector4SquaredDist (const float *v1, const float *v2);

void Vector2Inverse (float *v);
void Vector3Inverse (float *v);
void Vector4Inverse (float *v);

float fastfabs (float _X);

void VectorScale (vec3_t in, vec_t scale, vec3_t out);
void VectorCopy (float *in, float *out);
void VectorAdd (float *vec1, float *vec2, float *out);
void VectorSubtract (float *vec1, float *vec2, float *out);

float R_GetFarClip (void);

