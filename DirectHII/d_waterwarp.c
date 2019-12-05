
#include "precompiled.h"

#include "d_local.h"
#include "d_quadbatch.h"
#include "d_video.h"
#include "d_texture.h"

#include "resource.h"

/*
=======================================================================================================================

UNWERWATER WARP

=======================================================================================================================
*/

#define NOISESIZE	32

static int d3d_WaterwarpTexture;
static int d3d_WaterwarpNoise;

static int d3d_WaterwarpShader;

void D_WaterwarpOnLost (void)
{
}

void D_WaterwarpOnReset (void)
{
	d3d_WaterwarpTexture = D_CreateRenderTargetAtBackbufferSize ();
}

void D_WaterwarpOnRelease (void)
{
}

void D_WaterwarpRegister (void)
{
	// d3d11 doesn't have an equivalent of D3DFMT_V8U8 so we use DXGI_FORMAT_R16G16_SNORM instead
	if ((d3d_WaterwarpNoise = D_GetTexture ("", NOISESIZE, NOISESIZE, 1, 0, TEX_NOISE)) != 0)
	{
		int x, y;
		int mark = Hunk_LowMark (LOAD_HUNK);
		unsigned *dst = (unsigned *) Hunk_Alloc (LOAD_HUNK, NOISESIZE * NOISESIZE * 4);
		unsigned *data = dst; // preserve the original pointer so that we can pass it to D_LoadTextureData32

		for (y = 0; y < NOISESIZE; y++)
		{
			short *vu = (short *) dst;

			for (x = 0; x < NOISESIZE; x++)
			{
				*vu++ = (rand () & 65535) - 32768;
				*vu++ = (rand () & 65535) - 32768;
			}

			dst += NOISESIZE;
		}

		D_LoadTextureData32 (d3d_WaterwarpNoise, 0, 0, 0, NOISESIZE, NOISESIZE, data);
		Hunk_FreeToLowMark (LOAD_HUNK, mark);
	}

	d3d_WaterwarpShader = D_CreateShaderBundleForQuadBatch (IDR_WATERWARP, "WaterWarpVS", "WaterWarpPS");

	VID_RegisterHandlers (D_WaterwarpOnRelease, D_WaterwarpOnLost, D_WaterwarpOnReset);
}

qboolean D_BeginWaterWarp (void)
{
	if (d3d_WaterwarpTexture)
	{
		D_SetRenderTargetTexture (d3d_WaterwarpTexture);
		return true;
	}
	else return false;
}

void D_DoWaterWarp (int width, int height)
{
	float s = (float) width / (float) height;
	float t = 1.0f;

	D_RestoreDefaultRenderTargets ();

	// and now we can draw it
	D_SetViewport (0, 0, width, height, 0, 0);

	D_BindShaderBundle (d3d_WaterwarpShader);
	D_SetRenderStates (d3d_BSNone, d3d_DSNoDepth, d3d_RSNoCull);
	D_PSSetShaderResource (0, d3d_WaterwarpTexture);
	D_PSSetShaderResource (4, d3d_WaterwarpNoise);

	D_BeginQuadBatch ();

	s *= 0.5f;
	t *= 0.5f;

	D_QuadVertexPosition2fColorTexCoord2f (-1, -1, 0xffffffff, 0, 0);
	D_QuadVertexPosition2fColorTexCoord2f (1, -1, 0xffffffff, s, 0);
	D_QuadVertexPosition2fColorTexCoord2f (1, 1, 0xffffffff, s, t);
	D_QuadVertexPosition2fColorTexCoord2f (-1, 1, 0xffffffff, 0, t);

	D_EndQuadBatch ();
}


