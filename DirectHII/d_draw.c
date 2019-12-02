
#include "precompiled.h"

#include "d_local.h"
#include "d_draw.h"
#include "d_video.h"
#include "d_quadbatch.h"
#include "d_texture.h"

#include "resource.h"

static int d3d_DrawTexturedShader;
static int d3d_DrawColouredShader;
static int d3d_DrawFinalizeShader;

static int d3d_DrawRenderTarget;

static ID3D11Buffer *d3d_DrawConstants = NULL;

typedef struct drawconstants_s {
	QMATRIX OrthoMatrix;
	float gamma;
	float contrast;
	float junk[2];
} drawconstants_t;


void D_DrawOnLost (void)
{
}

void D_DrawOnReset (void)
{
	d3d_DrawRenderTarget = D_CreateRenderTargetAtBackbufferSize ();
}

void D_DrawOnRelease (void)
{
}


void D_DrawRegister (void)
{
	D3D11_BUFFER_DESC cbDesc = {
		sizeof (drawconstants_t),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_CONSTANT_BUFFER,
		0,
		0,
		0
	};

	// buffers
	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &cbDesc, NULL, &d3d_DrawConstants);
	D_RegisterConstantBuffer (d3d_DrawConstants, 0);

	// shaders
	d3d_DrawTexturedShader = D_CreateShaderBundleForQuadBatch (IDR_DRAWSHADER, "DrawTexturedVS", "DrawTexturedPS");
	d3d_DrawColouredShader = D_CreateShaderBundleForQuadBatch (IDR_DRAWSHADER, "DrawColouredVS", "DrawColouredPS");
	d3d_DrawFinalizeShader = D_CreateShaderBundleForQuadBatch (IDR_DRAWSHADER, "DrawFinalizeVS", "DrawFinalizePS");

	VID_RegisterHandlers (D_DrawOnRelease, D_DrawOnLost, D_DrawOnReset);
}


void D_UpdateDrawConstants (int width, int height, float gammaval, float contrastval)
{
	drawconstants_t consts;

	R_MatrixIdentity (&consts.OrthoMatrix);
	R_MatrixOrtho (&consts.OrthoMatrix, 0, width, height, 0, -1, 1);

	consts.gamma = gammaval;
	consts.contrast = contrastval;

	d3d_Context->lpVtbl->UpdateSubresource (d3d_Context, (ID3D11Resource *) d3d_DrawConstants, 0, NULL, &consts, 0, 0);
}


void D_DrawSet2D (int width, int height)
{
	float clearcolor[4] = {0, 0, 0, 0};

	// because we want to support variable scaling levels, we draw the 2D GUI to a separate RT (with pre-multiplied alpha adding in via shader code, and
	// the appropriate blend mode set, so that destination alpha 1 remains 1 even if subsequently blended over) then blast that back to the main RT.
	// this approach resolves problems such as texture scaling, seams, leakage and bleeding, etc.
	D_SetRenderTargetTexture (d3d_DrawRenderTarget);
	D_ClearRenderTarget (d3d_DrawRenderTarget, clearcolor);

	D_SetViewport (0, 0, width, height, 0, 0);
	D_SetRenderStates (d3d_BSAlphaPreMult, d3d_DSNoDepth, d3d_RSNoCull);
	D_BeginQuadBatch ();
}


void D_DrawTexturedQuad (float x, float y, float w, float h, unsigned colour, float sl, float sh, float tl, float th, int texnum)
{
	D_CheckQuadBatch ();
	D_PSSetShaderResource (0, texnum);
	D_BindShaderBundle (d3d_DrawTexturedShader);

	D_QuadVertexPosition2fColorTexCoord2f (x, y, colour, sl, tl);
	D_QuadVertexPosition2fColorTexCoord2f (x + w, y, colour, sh, tl);
	D_QuadVertexPosition2fColorTexCoord2f (x + w, y + h, colour, sh, th);
	D_QuadVertexPosition2fColorTexCoord2f (x, y + h, colour, sl, th);
}


void D_DrawColouredQuad (float x, float y, float w, float h, unsigned colour)
{
	D_CheckQuadBatch ();
	D_BindShaderBundle (d3d_DrawColouredShader);

	D_QuadVertexPosition2fColor (x, y, colour);
	D_QuadVertexPosition2fColor (x + w, y, colour);
	D_QuadVertexPosition2fColor (x + w, y + h, colour);
	D_QuadVertexPosition2fColor (x, y + h, colour);
}


void D_DrawFlush (void)
{
	// flush the quad batch
	D_EndQuadBatch ();
}


void D_DrawEnd2D (int width, int height, float s, float t)
{
	// finish anything left over
	D_EndQuadBatch ();

	// switch back to the main RT
	D_RestoreDefaultRenderTargets ();

	// set up our states...
	D_SetViewport (0, 0, width, height, 0, 0);
	D_SetRenderStates (d3d_BSAlphaPreMult, d3d_DSNoDepth, d3d_RSNoCull);

	D_PSSetShaderResource (0, d3d_DrawRenderTarget);
	D_BindShaderBundle (d3d_DrawFinalizeShader);

	// ...and draw the 2D RT back to the main RT
	D_BeginQuadBatch ();

	D_QuadVertexPosition2fColorTexCoord2f (-1, -1, 0xffffffff, 0, t);
	D_QuadVertexPosition2fColorTexCoord2f ( 1, -1, 0xffffffff, s, t);
	D_QuadVertexPosition2fColorTexCoord2f ( 1,  1, 0xffffffff, s, 0);
	D_QuadVertexPosition2fColorTexCoord2f (-1,  1, 0xffffffff, 0, 0);

	D_EndQuadBatch ();
}


