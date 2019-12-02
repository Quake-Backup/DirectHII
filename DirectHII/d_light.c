
#include "precompiled.h"

#include "d_local.h"
#include "d_light.h"
#include "d_texture.h"

#include "resource.h"

#define NUMVERTEXNORMALS 162

// padded for HLSL usage
// to do - reconstruct this mathematically
float d_avertexnormals[NUMVERTEXNORMALS][4] = {
	{-0.525731f, 0.000000f, 0.850651f}, {-0.442863f, 0.238856f, 0.864188f}, {-0.295242f, 0.000000f, 0.955423f}, {-0.309017f, 0.500000f, 0.809017f}, {-0.162460f, 0.262866f, 0.951056f}, {0.000000f, 0.000000f, 1.000000f},
	{0.000000f, 0.850651f, 0.525731f}, {-0.147621f, 0.716567f, 0.681718f}, {0.147621f, 0.716567f, 0.681718f}, {0.000000f, 0.525731f, 0.850651f}, {0.309017f, 0.500000f, 0.809017f}, {0.525731f, 0.000000f, 0.850651f},
	{0.295242f, 0.000000f, 0.955423f}, {0.442863f, 0.238856f, 0.864188f}, {0.162460f, 0.262866f, 0.951056f}, {-0.681718f, 0.147621f, 0.716567f}, {-0.809017f, 0.309017f, 0.500000f}, {-0.587785f, 0.425325f, 0.688191f},
	{-0.850651f, 0.525731f, 0.000000f}, {-0.864188f, 0.442863f, 0.238856f}, {-0.716567f, 0.681718f, 0.147621f}, {-0.688191f, 0.587785f, 0.425325f}, {-0.500000f, 0.809017f, 0.309017f}, {-0.238856f, 0.864188f, 0.442863f},
	{-0.425325f, 0.688191f, 0.587785f}, {-0.716567f, 0.681718f, -0.147621f}, {-0.500000f, 0.809017f, -0.309017f}, {-0.525731f, 0.850651f, 0.000000f}, {0.000000f, 0.850651f, -0.525731f}, {-0.238856f, 0.864188f, -0.442863f},
	{0.000000f, 0.955423f, -0.295242f}, {-0.262866f, 0.951056f, -0.162460f}, {0.000000f, 1.000000f, 0.000000f}, {0.000000f, 0.955423f, 0.295242f}, {-0.262866f, 0.951056f, 0.162460f}, {0.238856f, 0.864188f, 0.442863f},
	{0.262866f, 0.951056f, 0.162460f}, {0.500000f, 0.809017f, 0.309017f}, {0.238856f, 0.864188f, -0.442863f}, {0.262866f, 0.951056f, -0.162460f}, {0.500000f, 0.809017f, -0.309017f}, {0.850651f, 0.525731f, 0.000000f},
	{0.716567f, 0.681718f, 0.147621f}, {0.716567f, 0.681718f, -0.147621f}, {0.525731f, 0.850651f, 0.000000f}, {0.425325f, 0.688191f, 0.587785f}, {0.864188f, 0.442863f, 0.238856f}, {0.688191f, 0.587785f, 0.425325f},
	{0.809017f, 0.309017f, 0.500000f}, {0.681718f, 0.147621f, 0.716567f}, {0.587785f, 0.425325f, 0.688191f}, {0.955423f, 0.295242f, 0.000000f}, {1.000000f, 0.000000f, 0.000000f}, {0.951056f, 0.162460f, 0.262866f},
	{0.850651f, -0.525731f, 0.000000f}, {0.955423f, -0.295242f, 0.000000f}, {0.864188f, -0.442863f, 0.238856f}, {0.951056f, -0.162460f, 0.262866f}, {0.809017f, -0.309017f, 0.500000f}, {0.681718f, -0.147621f, 0.716567f},
	{0.850651f, 0.000000f, 0.525731f}, {0.864188f, 0.442863f, -0.238856f}, {0.809017f, 0.309017f, -0.500000f}, {0.951056f, 0.162460f, -0.262866f}, {0.525731f, 0.000000f, -0.850651f}, {0.681718f, 0.147621f, -0.716567f},
	{0.681718f, -0.147621f, -0.716567f}, {0.850651f, 0.000000f, -0.525731f}, {0.809017f, -0.309017f, -0.500000f}, {0.864188f, -0.442863f, -0.238856f}, {0.951056f, -0.162460f, -0.262866f}, {0.147621f, 0.716567f, -0.681718f},
	{0.309017f, 0.500000f, -0.809017f}, {0.425325f, 0.688191f, -0.587785f}, {0.442863f, 0.238856f, -0.864188f}, {0.587785f, 0.425325f, -0.688191f}, {0.688191f, 0.587785f, -0.425325f}, {-0.147621f, 0.716567f, -0.681718f},
	{-0.309017f, 0.500000f, -0.809017f}, {0.000000f, 0.525731f, -0.850651f}, {-0.525731f, 0.000000f, -0.850651f}, {-0.442863f, 0.238856f, -0.864188f}, {-0.295242f, 0.000000f, -0.955423f}, {-0.162460f, 0.262866f, -0.951056f},
	{0.000000f, 0.000000f, -1.000000f}, {0.295242f, 0.000000f, -0.955423f}, {0.162460f, 0.262866f, -0.951056f}, {-0.442863f, -0.238856f, -0.864188f}, {-0.309017f, -0.500000f, -0.809017f}, {-0.162460f, -0.262866f, -0.951056f},
	{0.000000f, -0.850651f, -0.525731f}, {-0.147621f, -0.716567f, -0.681718f}, {0.147621f, -0.716567f, -0.681718f}, {0.000000f, -0.525731f, -0.850651f}, {0.309017f, -0.500000f, -0.809017f}, {0.442863f, -0.238856f, -0.864188f},
	{0.162460f, -0.262866f, -0.951056f}, {0.238856f, -0.864188f, -0.442863f}, {0.500000f, -0.809017f, -0.309017f}, {0.425325f, -0.688191f, -0.587785f}, {0.716567f, -0.681718f, -0.147621f}, {0.688191f, -0.587785f, -0.425325f},
	{0.587785f, -0.425325f, -0.688191f}, {0.000000f, -0.955423f, -0.295242f}, {0.000000f, -1.000000f, 0.000000f}, {0.262866f, -0.951056f, -0.162460f}, {0.000000f, -0.850651f, 0.525731f}, {0.000000f, -0.955423f, 0.295242f},
	{0.238856f, -0.864188f, 0.442863f}, {0.262866f, -0.951056f, 0.162460f}, {0.500000f, -0.809017f, 0.309017f}, {0.716567f, -0.681718f, 0.147621f}, {0.525731f, -0.850651f, 0.000000f}, {-0.238856f, -0.864188f, -0.442863f},
	{-0.500000f, -0.809017f, -0.309017f}, {-0.262866f, -0.951056f, -0.162460f}, {-0.850651f, -0.525731f, 0.000000f}, {-0.716567f, -0.681718f, -0.147621f}, {-0.716567f, -0.681718f, 0.147621f}, {-0.525731f, -0.850651f, 0.000000f},
	{-0.500000f, -0.809017f, 0.309017f}, {-0.238856f, -0.864188f, 0.442863f}, {-0.262866f, -0.951056f, 0.162460f}, {-0.864188f, -0.442863f, 0.238856f}, {-0.809017f, -0.309017f, 0.500000f}, {-0.688191f, -0.587785f, 0.425325f},
	{-0.681718f, -0.147621f, 0.716567f}, {-0.442863f, -0.238856f, 0.864188f}, {-0.587785f, -0.425325f, 0.688191f}, {-0.309017f, -0.500000f, 0.809017f}, {-0.147621f, -0.716567f, 0.681718f}, {-0.425325f, -0.688191f, 0.587785f},
	{-0.162460f, -0.262866f, 0.951056f}, {0.442863f, -0.238856f, 0.864188f}, {0.162460f, -0.262866f, 0.951056f}, {0.309017f, -0.500000f, 0.809017f}, {0.147621f, -0.716567f, 0.681718f}, {0.000000f, -0.525731f, 0.850651f},
	{0.425325f, -0.688191f, 0.587785f}, {0.587785f, -0.425325f, 0.688191f}, {0.688191f, -0.587785f, 0.425325f}, {-0.955423f, 0.295242f, 0.000000f}, {-0.951056f, 0.162460f, 0.262866f}, {-1.000000f, 0.000000f, 0.000000f},
	{-0.850651f, 0.000000f, 0.525731f}, {-0.955423f, -0.295242f, 0.000000f}, {-0.951056f, -0.162460f, 0.262866f}, {-0.864188f, 0.442863f, -0.238856f}, {-0.951056f, 0.162460f, -0.262866f}, {-0.809017f, 0.309017f, -0.500000f},
	{-0.864188f, -0.442863f, -0.238856f}, {-0.951056f, -0.162460f, -0.262866f}, {-0.809017f, -0.309017f, -0.500000f}, {-0.681718f, 0.147621f, -0.716567f}, {-0.681718f, -0.147621f, -0.716567f}, {-0.850651f, 0.000000f, -0.525731f},
	{-0.688191f, 0.587785f, -0.425325f}, {-0.587785f, 0.425325f, -0.688191f}, {-0.425325f, 0.688191f, -0.587785f}, {-0.425325f, -0.688191f, -0.587785f}, {-0.587785f, -0.425325f, -0.688191f}, {-0.688191f, -0.587785f, -0.425325f}
};


int d3d_LightmapTexture;

typedef struct cbdlight_s {
	// this is the stuff needed for drawing the light and it goes in a cbuffer
	float origin[3];
	float radius;
	float rgb[3];
	float minlight;	// cbuffer padding; might use later
} cbdlight_t;

ID3D11Buffer *d3d_DLightConstants = NULL;

ID3D11Buffer *d3d_LightStyles = NULL;
ID3D11ShaderResourceView *d3d_LightStyleSRV = NULL;

// lightnormals are read on the GPU so that we can use a skinnier vertex format and save loads of memory
ID3D11Buffer *d3d_LightNormalsTex = NULL;
ID3D11ShaderResourceView *d3d_LightNormalsSRV = NULL;


void D_CreateLightStylesResources (void)
{
	float lightstyles[MAX_LIGHTSTYLES];

	D3D11_BUFFER_DESC LightStylesDesc = {
		sizeof (lightstyles),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE,
		0,
		0,
		0
	};

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = MAX_LIGHTSTYLES;

	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &LightStylesDesc, NULL, &d3d_LightStyles);
	d3d_Device->lpVtbl->CreateShaderResourceView (d3d_Device, (ID3D11Resource *) d3d_LightStyles, &srvDesc, &d3d_LightStyleSRV);

	D_CacheObject ((ID3D11DeviceChild *) d3d_LightStyles, "d3d_LightStyles");
	D_CacheObject ((ID3D11DeviceChild *) d3d_LightStyleSRV, "d3d_LightStyleSRV");
}

void D_CreateLightNormalsResources (void)
{
	D3D11_BUFFER_DESC LightNormalsDesc = {
		sizeof (d_avertexnormals),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE,
		0,
		0,
		0
	};

	D3D11_SUBRESOURCE_DATA srd = {d_avertexnormals, 0, 0};
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = NUMVERTEXNORMALS;

	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &LightNormalsDesc, &srd, &d3d_LightNormalsTex);
	d3d_Device->lpVtbl->CreateShaderResourceView (d3d_Device, (ID3D11Resource *) d3d_LightNormalsTex, &srvDesc, &d3d_LightNormalsSRV);

	D_CacheObject ((ID3D11DeviceChild *) d3d_LightNormalsTex, "d3d_LightNormalsTex");
	D_CacheObject ((ID3D11DeviceChild *) d3d_LightNormalsSRV, "d3d_LightNormalsSRV");
}


void D_LightRegister (void)
{
	D3D11_BUFFER_DESC cbDLightDesc = {
		sizeof (cbdlight_t),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_CONSTANT_BUFFER,
		0,
		0,
		0
	};

	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &cbDLightDesc, NULL, &d3d_DLightConstants);
	D_RegisterConstantBuffer (d3d_DLightConstants, 4);

	D_CreateLightStylesResources ();
	D_CreateLightNormalsResources ();
}


void D_CreateLightmapTexture (lighttexel_t *data[MAX_LIGHTMAPS], int numlightmaps)
{
	int i;

	// attempt to reuse it if possible, otherwise get a new one; lightmaps not created in this sequence will be released by TEX_DISPOSABLE
	if ((d3d_LightmapTexture = D_FindTexture ("lightmap", LIGHTMAP_SIZE, LIGHTMAP_SIZE, numlightmaps, 0, TEX_ALPHA | TEX_DISPOSABLE)) == 0)
		d3d_LightmapTexture = D_GetTexture ("lightmap", LIGHTMAP_SIZE, LIGHTMAP_SIZE, numlightmaps, 0, TEX_ALPHA | TEX_DISPOSABLE);

	// and load the new data on
	for (i = 0; i < numlightmaps; i++)
		D_LoadTextureData32 (d3d_LightmapTexture, i, 0, 0, LIGHTMAP_SIZE, LIGHTMAP_SIZE, data[i]);
}


void D_AnimateLight (int *styles)
{
	int i;
	float lightstyles[MAX_LIGHTSTYLES];

	ID3D11ShaderResourceView *SRVs[] = {d3d_LightStyleSRV, d3d_LightNormalsSRV};

	for (i = 0; i < MAX_LIGHTSTYLES; i++)
		lightstyles[i] = ((float) styles[i] / 264.0f) * 2.0f;

	d3d_Context->lpVtbl->UpdateSubresource (d3d_Context, (ID3D11Resource *) d3d_LightStyles, 0, NULL, lightstyles, 0, 0);

	// and the styles
	d3d_Context->lpVtbl->VSSetShaderResources (d3d_Context, 0, 2, SRVs);

	// binding the lightmap texture here too
	D_PSSetShaderResource (1, d3d_LightmapTexture);
}


void D_SetupDynamicLight (float *origin, float intensity)
{
	cbdlight_t consts;

	Vector3Copy (consts.origin, origin);
	consts.radius = intensity;
	Vector3Set (consts.rgb, 1, 1, 1);
	consts.minlight = 0;

	d3d_Context->lpVtbl->UpdateSubresource (d3d_Context, (ID3D11Resource *) d3d_DLightConstants, 0, NULL, &consts, 0, 0);

	// states - we can't use a multipass equal test over alpha surfaces because they won't have written depth to begin with
	D_SetRenderStates (d3d_BSAdditive, d3d_DSDepthNoWrite, d3d_RSFullCull);
}


