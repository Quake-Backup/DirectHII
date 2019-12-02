
#include "precompiled.h"

#include "d_local.h"
#include "d_mesh.h"
#include "d_main.h"
#include "d_texture.h"

#include "resource.h"


typedef struct aliasbuffers_s {
	ID3D11Buffer *PolyVerts;
	ID3D11Buffer *TexCoords;
	ID3D11Buffer *Indexes;

	int NumMesh;
	int NumIndexes;
	int NumPoses;

	char Name[256];
	int RegistrationSequence;
} aliasbuffers_t;

static aliasbuffers_t d3d_AliasBuffers[MAX_D3D_MODELS];
static int mdl_RegistrationSequence = 1;

static int d3d_MeshLightmapShader;
static int d3d_MeshDynamicShader;

ID3D11Buffer *d3d_MeshConstants;


void D_AliasOnLost (void)
{
}

void D_AliasOnReset (void)
{
}

void D_AliasOnRelease (void)
{
	int i;

	for (i = 0; i < MAX_D3D_MODELS; i++)
	{
		aliasbuffers_t *set = &d3d_AliasBuffers[i];

		SAFE_RELEASE (set->PolyVerts);
		SAFE_RELEASE (set->TexCoords);
		SAFE_RELEASE (set->Indexes);

		memset (set, 0, sizeof (aliasbuffers_t));
	}
}


void D_AliasRegister (void)
{
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		VDECL ("PREVTRIVERTX", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 0),
		VDECL ("CURRTRIVERTX", 0, DXGI_FORMAT_R8G8B8A8_UINT, 1, 0),
		VDECL ("TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0)
	};

	D3D11_BUFFER_DESC cbPerMeshDesc = {
		sizeof (meshconstants_t),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_CONSTANT_BUFFER,
		0,
		0,
		0
	};

	d3d_MeshLightmapShader = D_CreateShaderBundle (IDR_MESHSHADER, "MeshLightmapVS", NULL, "MeshLightmapPS", DEFINE_LAYOUT (layout));
	d3d_MeshDynamicShader = D_CreateShaderBundle (IDR_MESHSHADER, "MeshDynamicVS", NULL, "GenericDynamicPS", DEFINE_LAYOUT (layout));

	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &cbPerMeshDesc, NULL, &d3d_MeshConstants);
	D_RegisterConstantBuffer (d3d_MeshConstants, 3);

	VID_RegisterHandlers (D_AliasOnRelease, D_AliasOnLost, D_AliasOnReset);
}


void D_FreeUnusedAliasBuffers (void)
{
	int i;

	for (i = 0; i < MAX_D3D_MODELS; i++)
	{
		aliasbuffers_t *set = &d3d_AliasBuffers[i];

		if (set->RegistrationSequence != mdl_RegistrationSequence)
		{
			SAFE_RELEASE (set->PolyVerts);
			SAFE_RELEASE (set->TexCoords);
			SAFE_RELEASE (set->Indexes);

			memset (set, 0, sizeof (aliasbuffers_t));
		}
	}

	mdl_RegistrationSequence++;
}


int D_FindAliasBufferSet (char *name)
{
	int i;

	// try to find an existing buffer with this name
	for (i = 0; i < MAX_D3D_MODELS; i++)
	{
		aliasbuffers_t *set = &d3d_AliasBuffers[i];

		if (!set->PolyVerts) continue;
		if (!set->TexCoords) continue;
		if (!set->Indexes) continue;

		if (!strcmp (name, set->Name))
		{
			// mark as active
			set->RegistrationSequence = mdl_RegistrationSequence;
			return i;
		}
	}

	// didn't find it
	return -1;
}


int D_GetAliasBufferSet (char *name)
{
	int i;

	// find the first free buffer
	for (i = 0; i < MAX_D3D_MODELS; i++)
	{
		aliasbuffers_t *set = &d3d_AliasBuffers[i];

		if (set->PolyVerts) continue;
		if (set->TexCoords) continue;
		if (set->Indexes) continue;

		// cache the name so that we'll find it next time too
		strcpy (set->Name, name);

		// mark as active
		set->RegistrationSequence = mdl_RegistrationSequence;
		return i;
	}

	// didn't find a buffer
	Sys_Error ("D_GetAliasBufferSet : couldn't find a free buffer set!");
	return -1;
}


void D_MakeAliasPolyverts (int modelnum, int numposes, int nummesh, aliaspolyvert_t *polyverts)
{
	aliasbuffers_t *set = &d3d_AliasBuffers[modelnum];

	D3D11_BUFFER_DESC vbDesc = {
		sizeof (aliaspolyvert_t) * numposes * nummesh,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_VERTEX_BUFFER,
		0,
		0,
		0
	};

	// alloc a buffer to write the verts to and create the VB from
	D3D11_SUBRESOURCE_DATA srd = {polyverts, 0, 0};

	// create the new vertex buffer
	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &vbDesc, &srd, &set->PolyVerts);
	set->NumMesh = nummesh;
	set->NumPoses = numposes;
}


void D_MakeAliasTexCoords (int modelnum, int numtexcoords, aliastexcoord_t *texcoords)
{
	aliasbuffers_t *set = &d3d_AliasBuffers[modelnum];

	D3D11_BUFFER_DESC vbDesc = {
		sizeof (aliastexcoord_t) * numtexcoords,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_VERTEX_BUFFER,
		0,
		0,
		0
	};

	// alloc a buffer to write the verts to and create the VB from
	D3D11_SUBRESOURCE_DATA srd = {texcoords, 0, 0};

	// create the new vertex buffer
	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &vbDesc, &srd, &set->TexCoords);
}


void D_MakeAliasIndexes (int modelnum, int numindexes, unsigned short *indexes)
{
	aliasbuffers_t *set = &d3d_AliasBuffers[modelnum];

	D3D11_BUFFER_DESC ibDesc = {
		sizeof (unsigned short) * numindexes,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_INDEX_BUFFER,
		0,
		0,
		0
	};

	// alloc a buffer to write the verts to and create the VB from
	D3D11_SUBRESOURCE_DATA srd = {indexes, 0, 0};

	// create the new vertex buffer
	d3d_Device->lpVtbl->CreateBuffer (d3d_Device, &ibDesc, &srd, &set->Indexes);
	set->NumIndexes = numindexes;
}


void D_SetAliasSkin (int texnum)
{
	D_PSSetShaderResource (0, texnum);
}


void D_SetAliasPoses (int modelnum, int posenum0, int posenum1)
{
	aliasbuffers_t *set = &d3d_AliasBuffers[modelnum];

	// get stream offsets for each pose
	int StreamOffset0 = posenum0 * sizeof (aliaspolyvert_t) * set->NumMesh;
	int StreamOffset1 = posenum1 * sizeof (aliaspolyvert_t) * set->NumMesh;

	D_BindVertexBuffer (0, set->PolyVerts, sizeof (aliaspolyvert_t), StreamOffset0);
	D_BindVertexBuffer (1, set->PolyVerts, sizeof (aliaspolyvert_t), StreamOffset1);
	D_BindVertexBuffer (2, set->TexCoords, sizeof (aliastexcoord_t), 0);

	D_BindIndexBuffer (set->Indexes, DXGI_FORMAT_R16_UINT);
}


void D_SetupAliasModel (QMATRIX *localMatrix, float alphaVal, float *origin, qboolean alphaEnable, qboolean alphaReverse)
{
	// select the correct render states
	if (alphaEnable)
	{
		if (alphaReverse)
			D_SetRenderStates (d3d_BSAlphaReverse, d3d_DSDepthNoWrite, d3d_RSNoCull);
		else D_SetRenderStates (d3d_BSAlphaBlend, d3d_DSDepthNoWrite, d3d_RSFullCull);
	}
	else D_SetRenderStates (d3d_BSNone, d3d_DSFullDepth, d3d_RSFullCull);

	// update the constants here too
	D_UpdateEntityConstants (localMatrix, alphaVal, 1.0f, origin);
}


void D_DrawAliasPolyset (int modelnum)
{
	aliasbuffers_t *set = &d3d_AliasBuffers[modelnum];
	d3d_Context->lpVtbl->DrawIndexed (d3d_Context, set->NumIndexes, 0, 0);
}


void D_DrawAliasModel (int modelnum)
{
	D_BindShaderBundle (d3d_MeshLightmapShader);
	D_DrawAliasPolyset (modelnum);
}


void D_SetAliasCBuffer (meshconstants_t *consts)
{
	// and update to the cbuffer
	d3d_Context->lpVtbl->UpdateSubresource (d3d_Context, (ID3D11Resource *) d3d_MeshConstants, 0, NULL, consts, 0, 0);
}


void D_SetAliasDynamicState (void)
{
	D_BindShaderBundle (d3d_MeshDynamicShader);
}

