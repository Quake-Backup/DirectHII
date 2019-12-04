
#include "precompiled.h"

#include "d_local.h"
#include "d_particles.h"
#include "d_quadbatch.h"

#include "resource.h"


/*
=======================================================================================================================

PARTICLES

Split this off because it will be going to instanced particles.

=======================================================================================================================
*/

static int d3d_ParticleCircleShader;
static int d3d_ParticleSquareShader;
static int d3d_ParticleShader = 0;

void D_PartRegister (void)
{
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		VDECL ("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 1),
		VDECL ("COLOUR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 1),
		VDECL ("SIZE", 0, DXGI_FORMAT_R32_FLOAT, 0, 1),
		VDECL ("OFFSETS", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0)
	};

	D_CreateShaderBundle (IDR_PARTICLESHADER, "ParticleInstancedVS", NULL, "ParticleInstancedPS", DEFINE_LAYOUT (layout));

	d3d_ParticleCircleShader = D_CreateShaderBundleForQuadBatch (IDR_PARTICLESHADER, "ParticleCircleVS", "ParticleCirclePS");
	d3d_ParticleSquareShader = D_CreateShaderBundleForQuadBatch (IDR_PARTICLESHADER, "ParticleSquareVS", "ParticleSquarePS");
	d3d_ParticleShader = d3d_ParticleSquareShader;
}


void D_BeginParticles (void)
{
	// H2 particles always alpha blend, even if square; see VID_MakePartTable
	D_SetRenderStates (d3d_BSAlphaBlend, d3d_DSDepthNoWrite, d3d_RSFullCull);
	D_BindShaderBundle (d3d_ParticleShader);
	D_BeginQuadBatch ();
}

void D_AddParticle (float *origin, unsigned color, int size)
{
	D_CheckQuadBatch ();

	D_QuadVertexPosition3fvColorTexCoord2f (origin, color, -1, -1);
	D_QuadVertexPosition3fvColorTexCoord2f (origin, color, -1, 1);
	D_QuadVertexPosition3fvColorTexCoord2f (origin, color, 1, 1);
	D_QuadVertexPosition3fvColorTexCoord2f (origin, color, 1, -1);
}

void D_EndParticles (void)
{
	D_EndQuadBatch ();
}



