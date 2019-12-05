
#include "precompiled.h"

#include "d_local.h"
#include "d_main.h"
#include "d_sprite.h"
#include "d_quadbatch.h"
#include "d_video.h"
#include "d_texture.h"

#include "resource.h"

/*
=======================================================================================================================

MAIN

=======================================================================================================================
*/

typedef struct mainconstants_s {
	QMATRIX mvpMatrix;
	float viewOrigin[4]; // padded for cbuffer packing
	float viewForward[4]; // padded for cbuffer packing
	float viewRight[4]; // padded for cbuffer packing
	float viewUp[4]; // padded for cbuffer packing
	float skyScroll[2];
	float skyScale;
	float turbTime;
	float vBlend[4];
	float WarpTime[2];
	float junk[2];
} mainconstants_t;

typedef struct entityconstants_s {
	QMATRIX localMatrix;
	float alphaVal;
	float absLight;
	float junk[2];
	float origin[4]; // padded for cbuffer packing
} entityconstants_t;


ID3D11Buffer *d3d_MainConstants = NULL;
ID3D11Buffer *d3d_EntityConstants = NULL;

int d3d_PolyblendShader = 0;

void D_MainRegister (void)
{
	D3D11_BUFFER_DESC cbMainDesc = {
		sizeof (mainconstants_t),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_CONSTANT_BUFFER,
		0,
		0,
		0
	};

	D3D11_BUFFER_DESC cbEntityDesc = {
		sizeof (entityconstants_t),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_CONSTANT_BUFFER,
		0,
		0,
		0
	};

	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &cbMainDesc, NULL, &d3d_MainConstants);
	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &cbEntityDesc, NULL, &d3d_EntityConstants);

	D_RegisterConstantBuffer (d3d_MainConstants, 1);
	D_RegisterConstantBuffer (d3d_EntityConstants, 2);

	d3d_PolyblendShader = D_CreateShaderBundleForQuadBatch (IDR_DRAWSHADER, "DrawPolyblendVS", "DrawPolyblendPS");
}


float D_CalcSkyScroll (float time, float speed)
{
	speed *= time;
	speed -= (int) speed & ~127;

	return speed * 0.0078125f;
}


void D_UpdateMainConstants (QMATRIX *mvpMatrix, float *viewOrigin, float *viewForward, float *viewRight, float *viewUp, float *vBlend, float time)
{
	mainconstants_t consts;
	double junk[2] = {0, 0};

	R_MatrixLoad (&consts.mvpMatrix, mvpMatrix);

	Vector3Copy (consts.viewOrigin, viewOrigin);
	Vector3Copy (consts.viewForward, viewForward);
	Vector3Copy (consts.viewRight, viewRight);
	Vector3Copy (consts.viewUp, viewUp);

	consts.skyScroll[0] = D_CalcSkyScroll (time, 8.0f);
	consts.skyScroll[1] = D_CalcSkyScroll (time, 16.0f);
	consts.skyScale = 2.953125f;

	// cycle in increments of 2 * M_PI so that the sine warp will wrap correctly
	consts.turbTime = modf ((double) time / (M_PI * 2.0), junk) * (M_PI * 2.0);

	Vector4Copy (consts.vBlend, vBlend);

	consts.WarpTime[0] = modf (time * 0.09f, &junk[0]);
	consts.WarpTime[1] = modf (time * 0.11f, &junk[1]);

	// and update to the cbuffer
	d3d_Context->lpVtbl->UpdateSubresource (d3d_Context, (ID3D11Resource *) d3d_MainConstants, 0, NULL, &consts, 0, 0);
}


void D_UpdateEntityConstants (QMATRIX *localMatrix, float alphaVal, float absLight, float *origin)
{
	entityconstants_t consts;

	R_MatrixLoad (&consts.localMatrix, localMatrix);

	consts.alphaVal = alphaVal;
	consts.absLight = absLight;

	Vector3Copy (consts.origin, origin);

	// and update to the cbuffer
	d3d_Context->lpVtbl->UpdateSubresource (d3d_Context, (ID3D11Resource *) d3d_EntityConstants, 0, NULL, &consts, 0, 0);
}


void D_ClearRenderTargets (unsigned color, BOOL clearcolor, BOOL cleardepth, BOOL clearstencil)
{
	UINT clearflags = 0;

	ID3D11RenderTargetView *RTV = NULL;
	ID3D11DepthStencilView *DSV = NULL;

	d3d_Context->lpVtbl->OMGetRenderTargets (d3d_Context, 1, &RTV, &DSV);

	if (clearcolor)
	{
		byte *rgba = (byte *) &color;
		float clear[4] = {rgba[0] / 255.0f, rgba[1] / 255.0f, rgba[2] / 255.0f, rgba[3] / 255.0f};
		d3d_Context->lpVtbl->ClearRenderTargetView (d3d_Context, RTV, clear);
	}

	if (cleardepth) clearflags |= D3D11_CLEAR_DEPTH;
	if (clearstencil) clearflags |= D3D11_CLEAR_STENCIL;

	d3d_Context->lpVtbl->ClearDepthStencilView (d3d_Context, DSV, clearflags, 1, 1);

	RTV->lpVtbl->Release (RTV);
	DSV->lpVtbl->Release (DSV);
}


void D_SyncPipeline (void)
{
	// forces the pipeline to sync by issuing a query then waiting for it to complete
	ID3D11Query *FinishEvent = NULL;
	D3D11_QUERY_DESC Desc = {D3D11_QUERY_EVENT, 0};

	if (SUCCEEDED (d3d_Device->lpVtbl->CreateQuery (d3d_Device, &Desc, &FinishEvent)))
	{
		d3d_Context->lpVtbl->End (d3d_Context, (ID3D11Asynchronous *) FinishEvent);
		while (d3d_Context->lpVtbl->GetData (d3d_Context, (ID3D11Asynchronous *) FinishEvent, NULL, 0, 0) == S_FALSE);
		SAFE_RELEASE (FinishEvent);
	}
}


void D_DrawPolyblend (void)
{
	D_SetRenderStates (d3d_BSAlphaBlend, d3d_DSDepthNoWrite, d3d_RSNoCull);
	D_BindShaderBundle (d3d_PolyblendShader);

	D_BeginQuadBatch ();

	D_QuadVertexPosition2f (-1, -1);
	D_QuadVertexPosition2f (1, -1);
	D_QuadVertexPosition2f (1, 1);
	D_QuadVertexPosition2f (-1, 1);

	D_EndQuadBatch ();
}


/*
=======================================================================================================================

SPRITES

=======================================================================================================================
*/

int d3d_SpriteShader;

void D_SpriteRegister (void)
{
	d3d_SpriteShader = D_CreateShaderBundleForQuadBatch (IDR_SPRITESHADER, "SpriteVS", "SpritePS");
}

void D_DrawSpriteModel (float positions[4][3], int alpha, int texnum)
{
	DWORD colour = 0xffffff | (alpha << 24);

	D_SetRenderStates (d3d_BSAlphaPreMult, d3d_DSDepthNoWrite, d3d_RSFullCull);
	D_BindShaderBundle (d3d_SpriteShader);
	D_PSSetShaderResource (0, texnum);

	D_BeginQuadBatch ();

	D_QuadVertexPosition3fvColorTexCoord2f (positions[0], colour, 0, 1);
	D_QuadVertexPosition3fvColorTexCoord2f (positions[1], colour, 0, 0);
	D_QuadVertexPosition3fvColorTexCoord2f (positions[2], colour, 1, 0);
	D_QuadVertexPosition3fvColorTexCoord2f (positions[3], colour, 1, 1);

	D_EndQuadBatch ();
}


