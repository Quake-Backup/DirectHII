
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

#define MAX_PARTICLE_INSTANCES		2048

typedef struct partpolyvert_s {
	float position[3];
	unsigned colour;
	float size;
} partpolyvert_t;

static int d3d_ParticleInstancedShader;
static int d3d_ParticleCircleShader;
static int d3d_ParticleSquareShader;
static int d3d_ParticleShader = 0;

ID3D11Buffer *d3d_PartVertexes = NULL;
ID3D11Buffer *d3d_PartInstances = NULL;
ID3D11Buffer *d3d_PartIndexes = NULL;

void D_PartRegister (void)
{
	static unsigned short indexes[6] = {0, 1, 2, 0, 2, 3};
	static float offsets[4][2] = {{-1, -1}, {-1, 1}, {1, 1}, {1, -1}};

	D3D11_INPUT_ELEMENT_DESC layout[] = {
		VDECL ("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 1),
		VDECL ("COLOUR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 1),
		VDECL ("SIZE", 0, DXGI_FORMAT_R32_FLOAT, 0, 1),
		VDECL ("OFFSETS", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0)
	};

	D3D11_BUFFER_DESC instDesc = {
		MAX_PARTICLE_INSTANCES * sizeof (partpolyvert_t),
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_VERTEX_BUFFER,
		D3D11_CPU_ACCESS_WRITE,
		0,
		0
	};

	D3D11_BUFFER_DESC vbDesc = {
		sizeof (offsets),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_VERTEX_BUFFER,
		0,
		0,
		0
	};

	D3D11_BUFFER_DESC ibDesc = {
		sizeof (indexes),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_INDEX_BUFFER,
		0,
		0,
		0
	};

	D3D11_SUBRESOURCE_DATA vsrd = {offsets, 0, 0};
	D3D11_SUBRESOURCE_DATA isrd = {indexes, 0, 0};

	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &vbDesc, &vsrd, &d3d_PartVertexes);
	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &instDesc, NULL, &d3d_PartInstances);
	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &ibDesc, &isrd, &d3d_PartIndexes);

	D_CacheObject ((ID3D11DeviceChild *) d3d_PartVertexes, "d3d_PartVertexes");
	D_CacheObject ((ID3D11DeviceChild *) d3d_PartInstances, "d3d_PartInstances");
	D_CacheObject ((ID3D11DeviceChild *) d3d_PartIndexes, "d3d_PartIndexes");

	d3d_ParticleInstancedShader = D_CreateShaderBundle (IDR_PARTICLESHADER, "ParticleInstancedVS", NULL, "ParticleInstancedPS", DEFINE_LAYOUT (layout));

	d3d_ParticleCircleShader = D_CreateShaderBundleForQuadBatch (IDR_PARTICLESHADER, "ParticleCircleVS", "ParticleCirclePS");
	d3d_ParticleSquareShader = D_CreateShaderBundleForQuadBatch (IDR_PARTICLESHADER, "ParticleSquareVS", "ParticleSquarePS");
	d3d_ParticleShader = d3d_ParticleSquareShader;
}


partpolyvert_t *d_particles = NULL;
int d_firstparticle = 0;
int d_numparticles = 0;


void D_FlushParticles (void)
{
	if (d_particles)
	{
		d3d_Context->lpVtbl->Unmap (d3d_Context, (ID3D11Resource *) d3d_PartInstances, 0);
		d_particles = NULL;
	}

	if (d_numparticles)
	{
		d3d_Context->lpVtbl->DrawIndexedInstanced (d3d_Context, 6, d_numparticles, 0, 0, d_firstparticle);
		d_firstparticle += d_numparticles;
		d_numparticles = 0;
	}
}


void D_BeginParticles (void)
{
	D_SetRenderStates (d3d_BSAlphaBlend, d3d_DSDepthNoWrite, d3d_RSFullCull);
	D_BindShaderBundle (d3d_ParticleInstancedShader);

	D_BindVertexBuffer (0, d3d_PartInstances, sizeof (partpolyvert_t), 0);
	D_BindVertexBuffer (1, d3d_PartVertexes, sizeof (float) * 2, 0);
	D_BindIndexBuffer (d3d_PartIndexes, DXGI_FORMAT_R16_UINT);

	d_particles = NULL;
	d_numparticles = 0;

#if 0
	// H2 particles always alpha blend, even if square; see VID_MakePartTable
	D_SetRenderStates (d3d_BSAlphaBlend, d3d_DSDepthNoWrite, d3d_RSFullCull);
	D_BindShaderBundle (d3d_ParticleShader);
	D_BeginQuadBatch ();
#endif
}

void D_AddParticle (float *origin, unsigned color, float size)
{
	if (d_firstparticle + d_numparticles + 1 >= MAX_PARTICLE_INSTANCES)
	{
		D_FlushParticles ();
		d_firstparticle = 0;
	}

	if (!d_particles)
	{
		D3D11_MAP mode = (d_firstparticle > 0) ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD;
		D3D11_MAPPED_SUBRESOURCE msr;

		if (SUCCEEDED (d3d_Context->lpVtbl->Map (d3d_Context, (ID3D11Resource *) d3d_PartInstances, 0, mode, 0, &msr)))
			d_particles = (partpolyvert_t *) msr.pData + d_firstparticle;
		else Sys_Error ("D_AddParticle : failed to map vertex buffer!");
	}

	d_particles[d_numparticles].position[0] = origin[0];
	d_particles[d_numparticles].position[1] = origin[1];
	d_particles[d_numparticles].position[2] = origin[2];

	d_particles[d_numparticles].colour = color;
	d_particles[d_numparticles].size = size;

	d_numparticles++;

#if 0
	D_CheckQuadBatch ();

	D_QuadVertexPosition3fvColorTexCoord2f (origin, color, -1, -1);
	D_QuadVertexPosition3fvColorTexCoord2f (origin, color, -1, 1);
	D_QuadVertexPosition3fvColorTexCoord2f (origin, color, 1, 1);
	D_QuadVertexPosition3fvColorTexCoord2f (origin, color, 1, -1);
#endif
}

void D_EndParticles (void)
{
	D_FlushParticles ();

#if 0
	D_EndQuadBatch ();
#endif
}



