
// implements a quad batcher based on a limited subset of an OpenGL-like "immediate mode" suitable for use by particles, draw, etc
#include "precompiled.h"

#include "d_local.h"
#include "d_quadbatch.h"


typedef struct quadpolyvert_s {
	float position[3];
	DWORD color;
	float texcoord[2];
} quadpolyvert_t;

// these should be sized such that MAX_DRAW_INDEXES = (MAX_DRAW_VERTS / 4) * 6
#define MAX_QUAD_VERTS		0x400
#define MAX_QUAD_INDEXES	0x600

#if (MAX_QUAD_VERTS / 4) * 6 != MAX_QUAD_INDEXES
#error (MAX_QUAD_VERTS / 4) * 6 != MAX_QUAD_INDEXES
#endif

// because we don't know in advance how much we're going to draw, we first transfer to a system memory buffer, then
// transfer that to the vertex buffer
static quadpolyvert_t d_quadverts[MAX_QUAD_VERTS];

static int d_firstquadvert = 0;
static int d_numquadverts = 0;

static ID3D11Buffer *d3d_QuadVertexes = NULL;
static ID3D11Buffer *d3d_QuadIndexes = NULL;


void D_QuadBatchRegister (void)
{
	D3D11_BUFFER_DESC vbDesc = {
		sizeof (quadpolyvert_t) * MAX_QUAD_VERTS,
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_VERTEX_BUFFER,
		D3D11_CPU_ACCESS_WRITE,
		0,
		0
	};

	D3D11_BUFFER_DESC ibDesc = {
		sizeof (unsigned short) * MAX_QUAD_INDEXES,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_INDEX_BUFFER,
		0,
		0,
		0
	};

	int i;
	int mark = Hunk_LowMark (LOAD_HUNK);
	unsigned short *ndx = Hunk_Alloc (LOAD_HUNK, sizeof (unsigned short) * MAX_QUAD_INDEXES);
	D3D11_SUBRESOURCE_DATA srd = {ndx, 0, 0};

	for (i = 0; i < MAX_QUAD_VERTS; i += 4, ndx += 6)
	{
		ndx[0] = i + 0;
		ndx[1] = i + 1;
		ndx[2] = i + 2;

		ndx[3] = i + 0;
		ndx[4] = i + 2;
		ndx[5] = i + 3;
	}

	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &vbDesc, NULL, &d3d_QuadVertexes);
	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &ibDesc, &srd, &d3d_QuadIndexes);

	Hunk_FreeToLowMark (LOAD_HUNK, mark);

	D_CacheObject ((ID3D11DeviceChild *) d3d_QuadIndexes, "d3d_QuadIndexes");
	D_CacheObject ((ID3D11DeviceChild *) d3d_QuadVertexes, "d3d_QuadVertexes");
}


int D_CreateShaderBundleForQuadBatch (int resourceID, const char *vsentry, const char *psentry)
{
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		VDECL ("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0),
		VDECL ("COLOUR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0),
		VDECL ("TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0)
	};

	return D_CreateShaderBundle (resourceID, vsentry, NULL, psentry, DEFINE_LAYOUT (layout));
}


void D_BeginQuadBatch (void)
{
	D_CheckQuadBatch ();

	D_BindVertexBuffer (0, d3d_QuadVertexes, sizeof (quadpolyvert_t), 0);
	D_BindIndexBuffer (d3d_QuadIndexes, DXGI_FORMAT_R16_UINT);

	d_numquadverts = 0;
}


void D_QuadVertexPosition3fvColorTexCoord2f (float *xyz, DWORD color, float s, float t)
{
	d_quadverts[d_numquadverts].texcoord[0] = s;
	d_quadverts[d_numquadverts].texcoord[1] = t;

	d_quadverts[d_numquadverts].color = color;

	d_quadverts[d_numquadverts].position[0] = xyz[0];
	d_quadverts[d_numquadverts].position[1] = xyz[1];
	d_quadverts[d_numquadverts].position[2] = xyz[2];

	d_numquadverts++;
}


void D_QuadVertexPosition2fColorTexCoord2f (float x, float y, DWORD color, float s, float t)
{
	d_quadverts[d_numquadverts].texcoord[0] = s;
	d_quadverts[d_numquadverts].texcoord[1] = t;

	d_quadverts[d_numquadverts].color = color;

	d_quadverts[d_numquadverts].position[0] = x;
	d_quadverts[d_numquadverts].position[1] = y;
	d_quadverts[d_numquadverts].position[2] = 0;

	d_numquadverts++;
}


void D_QuadVertexPosition2fColor (float x, float y, DWORD color)
{
	d_quadverts[d_numquadverts].color = color;

	d_quadverts[d_numquadverts].position[0] = x;
	d_quadverts[d_numquadverts].position[1] = y;
	d_quadverts[d_numquadverts].position[2] = 0;

	d_numquadverts++;
}


void D_QuadVertexPosition2f (float x, float y)
{
	d_quadverts[d_numquadverts].position[0] = x;
	d_quadverts[d_numquadverts].position[1] = y;
	d_quadverts[d_numquadverts].position[2] = 0;

	d_numquadverts++;
}


void D_CheckQuadBatch (void)
{
	if (d_numquadverts + 4 >= MAX_QUAD_VERTS)
	{
		D_EndQuadBatch ();
		d_numquadverts = 0;
	}
}


void D_EndQuadBatch (void)
{
	if (d_numquadverts)
	{
		D3D11_MAP mode = D3D11_MAP_WRITE_NO_OVERWRITE;
		D3D11_MAPPED_SUBRESOURCE msr;

		if (d_firstquadvert + d_numquadverts >= MAX_QUAD_VERTS)
		{
			mode = D3D11_MAP_WRITE_DISCARD;
			d_firstquadvert = 0;
		}

		if (SUCCEEDED (d3d_Context->lpVtbl->Map (d3d_Context, (ID3D11Resource *) d3d_QuadVertexes, 0, mode, 0, &msr)))
		{
			memcpy ((quadpolyvert_t *) msr.pData + d_firstquadvert, d_quadverts, d_numquadverts * sizeof (quadpolyvert_t));
			d3d_Context->lpVtbl->Unmap (d3d_Context, (ID3D11Resource *) d3d_QuadVertexes, 0);
		}

		d3d_Context->lpVtbl->DrawIndexed (d3d_Context, (d_numquadverts >> 2) * 6, 0, d_firstquadvert);

		d_firstquadvert += d_numquadverts;
		d_numquadverts = 0;
	}
}


