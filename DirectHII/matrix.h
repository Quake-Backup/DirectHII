
#ifndef MATRIX_H
#define MATRIX_H

// define early so that it's available to everything
__declspec (align (16)) typedef union _QMATRIX
{
	float m4x4[4][4];
	float m16[16];
} QMATRIX;

// multiply in-place rather than construct a new matrix and leave it at that
QMATRIX *R_MatrixIdentity (QMATRIX *m);
QMATRIX *R_MatrixMultiply (QMATRIX *out, QMATRIX *m1, QMATRIX *m2);
QMATRIX *R_MatrixTranslate (QMATRIX *m, float x, float y, float z);
QMATRIX *R_MatrixScale (QMATRIX *m, float x, float y, float z);
QMATRIX *R_MatrixLoad (QMATRIX *dst, QMATRIX *src);
QMATRIX *R_MatrixOrtho (QMATRIX *m, float left, float right, float bottom, float top, float zNear, float zFar);
QMATRIX *R_MatrixFrustum (QMATRIX *m, float fovx, float fovy, float zn, float zf);
QMATRIX *R_MatrixLoadf (QMATRIX *m, float _11, float _12, float _13, float _14, float _21, float _22, float _23, float _24, float _31, float _32, float _33, float _34, float _41, float _42, float _43, float _44);
QMATRIX *R_MatrixMultiplyf (QMATRIX *m, float _11, float _12, float _13, float _14, float _21, float _22, float _23, float _24, float _31, float _32, float _33, float _34, float _41, float _42, float _43, float _44);
QMATRIX *R_MatrixRotate (QMATRIX *m, float p, float y, float r);
QMATRIX *R_MatrixCamera (QMATRIX *m, const float *origin, const float *angles);

float *R_VectorTransform (QMATRIX *m, float *out, float *in);
float *R_VectorInverseTransform (QMATRIX *m, float *out, float *in);

#endif
