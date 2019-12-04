
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
	d3d_ParticleCircleShader = D_CreateShaderBundleForQuadBatch (IDR_PARTICLESHADER, "ParticleCircleVS", "ParticleCirclePS");
	d3d_ParticleSquareShader = D_CreateShaderBundleForQuadBatch (IDR_PARTICLESHADER, "ParticleSquareVS", "ParticleSquarePS");
	d3d_ParticleShader = d3d_ParticleSquareShader;
}


void D_BeginParticles (void)
{
	// square particles can potentially expose a faster path by not using alpha blending
	// but we might wish to add particle fade at some time so we can't do it
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



