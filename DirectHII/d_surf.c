
#include "precompiled.h"

#include "d_local.h"
#include "d_surf.h"
#include "d_light.h"
#include "d_texture.h"

#include "resource.h"

ID3D11Buffer *d3d_SurfVertexes = NULL;
ID3D11Buffer *d3d_SurfIndexes = NULL;

int FirstSurfIndex = 0;

int d3d_SolidSkyTexture;
int d3d_AlphaSkyTexture;

int d3d_SurfLightmapShader;
int d3d_SurfAbsLightShader;
int d3d_SurfDynamicShader;
int d3d_SurfDrawSkyShader;
int d3d_SurfDrawTurbShader;


void D_SurfOnLost (void)
{
}

void D_SurfOnReset (void)
{
}

void D_SurfOnRelease (void)
{
	SAFE_RELEASE (d3d_SurfVertexes);
	SAFE_RELEASE (d3d_SurfIndexes);
}

void D_SurfRegister (void)
{
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		VDECL ("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0),
		VDECL ("TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0),
		VDECL ("LIGHTMAP", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0),
		VDECL ("MAPNUM", 0, DXGI_FORMAT_R32_FLOAT, 0, 0),
		VDECL ("STYLES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 0)
	};

	D3D11_BUFFER_DESC ibDesc = {
		MAX_SURF_INDEXES * sizeof (unsigned int),
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_INDEX_BUFFER,
		D3D11_CPU_ACCESS_WRITE,
		0,
		0
	};

	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &ibDesc, NULL, &d3d_SurfIndexes);

	d3d_SurfLightmapShader = D_CreateShaderBundle (IDR_SURFSHADER, "SurfLightmapVS", NULL, "SurfLightmapPS", DEFINE_LAYOUT (layout));
	d3d_SurfAbsLightShader = D_CreateShaderBundle (IDR_SURFSHADER, "SurfAbsLightVS", NULL, "SurfAbsLightPS", DEFINE_LAYOUT (layout));
	d3d_SurfDrawTurbShader = D_CreateShaderBundle (IDR_SURFSHADER, "SurfDrawTurbVS", NULL, "SurfDrawTurbPS", DEFINE_LAYOUT (layout));
	d3d_SurfDrawSkyShader = D_CreateShaderBundle (IDR_SURFSHADER, "SurfDrawSkyVS", NULL, "SurfDrawSkyPS", DEFINE_LAYOUT (layout));
	d3d_SurfDynamicShader = D_CreateShaderBundle (IDR_SURFSHADER, "SurfDynamicVS", "SurfDynamicGS", "GenericDynamicPS", DEFINE_LAYOUT (layout));

	// create the sky textures; subsequent loads will just fill them
	d3d_SolidSkyTexture = D_GetTexture ("solidSky", 128, 128, 1, 0, TEX_RGBA8);
	d3d_AlphaSkyTexture = D_GetTexture ("alphaSky", 128, 128, 1, 0, TEX_ALPHA);

	VID_RegisterHandlers (D_SurfOnRelease, D_SurfOnLost, D_SurfOnReset);
}


void D_SurfCreateVertexBuffer (brushpolyvert_t *verts, int numvertexes)
{
	D3D11_BUFFER_DESC vbDesc = {
		sizeof (brushpolyvert_t) * numvertexes,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_VERTEX_BUFFER,
		0,
		0,
		0
	};

	// alloc a buffer to write the verts to and create the VB from
	D3D11_SUBRESOURCE_DATA srd = {verts, 0, 0};

	// release the old vertex buffer
	SAFE_RELEASE (d3d_SurfVertexes);

	// create the new vertex buffer
	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &vbDesc, &srd, &d3d_SurfVertexes);
}


void D_SurfBeginDrawingModel (qboolean alpha)
{
	if (alpha)
		D_SetRenderStates (d3d_BSAlphaBlend, d3d_DSDepthNoWrite, d3d_RSFullCull);
	else D_SetRenderStates (d3d_BSNone, d3d_DSFullDepth, d3d_RSFullCull);

	D_BindVertexBuffer (0, d3d_SurfVertexes, sizeof (brushpolyvert_t), 0);
	D_BindIndexBuffer (d3d_SurfIndexes, DXGI_FORMAT_R32_UINT);
}


void D_SetWaterState (int texnum)
{
	D_PSSetShaderResource (0, texnum);
	D_BindShaderBundle (d3d_SurfDrawTurbShader);
}


void D_SetSkyState (void)
{
	D_SetRenderStates (d3d_BSNone, d3d_DSFullDepth, d3d_RSFullCull);
	D_BindShaderBundle (d3d_SurfDrawSkyShader);
	D_PSSetShaderResource (2, d3d_SolidSkyTexture);
	D_PSSetShaderResource (3, d3d_AlphaSkyTexture);
}


void D_SetLightmappedState (int texnum)
{
	D_PSSetShaderResource (0, texnum);
	D_BindShaderBundle (d3d_SurfLightmapShader);
}


void D_SetAbsLightState (int texnum)
{
	D_PSSetShaderResource (0, texnum);
	D_BindShaderBundle (d3d_SurfAbsLightShader);
}


void D_SetDynamicLightSurfState (int texnum)
{
	D_PSSetShaderResource (0, texnum);
	D_BindShaderBundle (d3d_SurfDynamicShader);
}


void D_LoadSkyTextures (byte *src, unsigned *palette)
{
	D_FillTexture8 (d3d_SolidSkyTexture, &src[128], 128, 128, 256, palette, TEX_RGBA8);
	D_FillTexture8 (d3d_AlphaSkyTexture, src, 128, 128, 256, palette, TEX_ALPHA);
}


void D_DrawSurfaceBatch (unsigned *indexes, int numindexes)
{
	if (numindexes)
	{
		D3D11_MAP mode = D3D11_MAP_WRITE_NO_OVERWRITE;
		D3D11_MAPPED_SUBRESOURCE msr;

		if (FirstSurfIndex + numindexes >= MAX_SURF_INDEXES)
		{
			mode = D3D11_MAP_WRITE_DISCARD;
			FirstSurfIndex = 0;
		}

		if (SUCCEEDED (d3d_Context->lpVtbl->Map (d3d_Context, (ID3D11Resource *) d3d_SurfIndexes, 0, mode, 0, &msr)))
		{
			memcpy ((unsigned int *) msr.pData + FirstSurfIndex, indexes, numindexes * sizeof (unsigned int));
			d3d_Context->lpVtbl->Unmap (d3d_Context, (ID3D11Resource *) d3d_SurfIndexes, 0);
		}

		d3d_Context->lpVtbl->DrawIndexed (d3d_Context, numindexes, FirstSurfIndex, 0);

		FirstSurfIndex += numindexes;
		numindexes = 0;
	}
}


