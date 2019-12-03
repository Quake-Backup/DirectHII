
#include "precompiled.h"

#include "d_local.h"
#include "d_texture.h"
#include "d_draw.h"
#include "d_image.h"

#define MAX_D3D_TEXTURES 4096

typedef struct d3d_texture_s {
	// D3D texture object
	ID3D11Texture2D *Texture;
	ID3D11ShaderResourceView *SRV;
	ID3D11RenderTargetView *RTV;
	D3D11_TEXTURE2D_DESC Desc;

	// in-engine descriptions
	char Identifier[64];
	int Width, Height, ArraySize;
	unsigned short CRC;
	int Flags;
	int RegistrationSequence;
} d3d_texture_t;


static d3d_texture_t d3d_Textures[MAX_D3D_TEXTURES];
static int tex_RegistrationSequence = 1;


void D_TextureOnLost (void)
{
	// textures created with TEX_RENDERTARGET must be in the default pool and must therefore be recreated on a lost device
	int i;

	for (i = 0; i < MAX_D3D_TEXTURES; i++)
	{
		d3d_texture_t *tex = &d3d_Textures[i];

		// not a texture
		if (!tex->Texture) continue;

		// not disposable
		if (!(tex->Flags & TEX_RENDERTARGET)) continue;

		// OK to dispose of
		SAFE_RELEASE (tex->Texture);
		SAFE_RELEASE (tex->SRV);
		SAFE_RELEASE (tex->RTV);

		memset (tex, 0, sizeof (d3d_texture_t));
	}
}

void D_TextureOnReset (void)
{
	// recreating D3DUSAGE_RENDERTARGET textures is the responsibility of the original creation func
}

void D_TextureOnRelease (void)
{
	int i;

	for (i = 0; i < MAX_D3D_TEXTURES; i++)
	{
		d3d_texture_t *tex = &d3d_Textures[i];

		SAFE_RELEASE (tex->Texture);
		SAFE_RELEASE (tex->SRV);
		SAFE_RELEASE (tex->RTV);
	}

	memset (d3d_Textures, 0, sizeof (d3d_Textures));
}

void D_TextureRegister (void)
{
	Image_Initialize ();
	VID_RegisterHandlers (D_TextureOnRelease, D_TextureOnLost, D_TextureOnReset);
}


static void D_DescribeTexture (D3D11_TEXTURE2D_DESC *Desc, int Width, int Height, int ArraySize, int flags)
{
	// validate the params
	if ((flags & TEX_CUBEMAP) && ArraySize != 6) Sys_Error ("D_DescribeTexture : (flags & TEX_CUBEMAP) && ArraySize != 6");
	if ((flags & TEX_CUBEMAP) && Width != Height) Sys_Error ("D_DescribeTexture : (flags & TEX_CUBEMAP) && Width != Height");

	if (flags & TEX_CUBEMAP)
	{
		if (Width < 1 || Width > D3D10_REQ_TEXTURECUBE_DIMENSION) Sys_Error ("D_DescribeTexture : Width < 1 || Width > D3D10_REQ_TEXTURECUBE_DIMENSION");
		if (Height < 1 || Height > D3D10_REQ_TEXTURECUBE_DIMENSION) Sys_Error ("D_DescribeTexture : Height < 1 || Height > D3D10_REQ_TEXTURECUBE_DIMENSION");
	}
	else
	{
		if (Width < 1 || Width > D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION) Sys_Error ("D_DescribeTexture : Width < 1 || Width > D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION");
		if (Height < 1 || Height > D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION) Sys_Error ("D_DescribeTexture : Height < 1 || Height > D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION");
	}

	if (ArraySize < 1 || ArraySize > D3D10_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) Sys_Error ("D_DescribeTexture : ArraySize < 1 || ArraySize > D3D10_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION");

	// fill it in
	Desc->Width = Width;
	Desc->Height = Height;
	Desc->MipLevels = (flags & TEX_MIPMAP) ? 0 : 1;
	Desc->ArraySize = ArraySize;

	// select the appropriate format
	if (flags & TEX_R32F)
		Desc->Format = DXGI_FORMAT_R32_FLOAT;
	else if (flags & TEX_NOISE)
		Desc->Format = DXGI_FORMAT_R16G16_SNORM;
	else Desc->Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	Desc->SampleDesc.Count = 1;
	Desc->SampleDesc.Quality = 0;

	if (flags & TEX_STAGING)
	{
		Desc->Usage = D3D11_USAGE_STAGING;
		Desc->BindFlags = 0;
		Desc->CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
	}
	else
	{
		Desc->Usage = D3D11_USAGE_DEFAULT;
		Desc->BindFlags = D3D11_BIND_SHADER_RESOURCE | ((flags & TEX_RENDERTARGET) ? D3D11_BIND_RENDER_TARGET : 0);
		Desc->CPUAccessFlags = 0;
	}

	Desc->MiscFlags = (flags & TEX_CUBEMAP) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;
}


int D_GetTexture (char *identifier, int width, int height, int arraysize, int crc, int flags)
{
	int i;

	// texture 0 is never used so that ! tests will work as expected and so that memset-0 will correctly clear to no texture
	for (i = 1; i < MAX_D3D_TEXTURES; i++)
	{
		d3d_texture_t *tex = &d3d_Textures[i];

		// already allocated
		if (tex->Texture) continue;

		// free and available
		strcpy (tex->Identifier, identifier);
		tex->Width = width;
		tex->Height = height;
		tex->ArraySize = arraysize;
		tex->Flags = flags;
		tex->CRC = crc;

		// and use this texture
		tex->RegistrationSequence = tex_RegistrationSequence;

		// describe the texture
		D_DescribeTexture (&tex->Desc, width, height, arraysize, flags);

		// failure is not an option...
		if (FAILED (d3d_Device->lpVtbl->CreateTexture2D (d3d_Device, &tex->Desc, NULL, &tex->Texture))) Sys_Error ("CreateTexture2D failed");
		if (FAILED (d3d_Device->lpVtbl->CreateShaderResourceView (d3d_Device, (ID3D11Resource *) tex->Texture, NULL, &tex->SRV))) Sys_Error ("CreateShaderResourceView failed");

		// optionally create a rendertarget view
		if (!(flags & TEX_RENDERTARGET)) return i;

		// failure is not an option...
		if (FAILED (d3d_Device->lpVtbl->CreateRenderTargetView (d3d_Device, (ID3D11Resource *) tex->Texture, NULL, &tex->RTV))) Sys_Error ("CreateShaderResourceView failed");

		return i;
	}

	// attempt to free any unreferenced textures and try again
	D_FreeUnusedTextures ();
	return D_GetTexture (identifier, width, height, arraysize, crc, flags);
}


int D_FindTexture (char *identifier, int width, int height, int arraysize, int crc, int flags)
{
	int i;

	// texture 0 is never used so that ! tests will work as expected and so that memset-0 will correctly clear to no texture
	for (i = 1; i < MAX_D3D_TEXTURES; i++)
	{
		d3d_texture_t *tex = &d3d_Textures[i];

		// not allocated
		if (!tex->Texture) continue;

		// check for a match
		if (width != tex->Width) continue;
		if (height != tex->Height) continue;
		if (arraysize != tex->ArraySize) continue;
		if (flags != tex->Flags) continue;
		if (crc != tex->CRC) continue;
		if (strcmp (identifier, tex->Identifier)) continue;

		// and reuse this texture
		tex->RegistrationSequence = tex_RegistrationSequence;

		return i;
	}

	// not found
	return 0;
}


void D_LoadTextureData32 (int texnum, int level, int xofs, int yofs, int width, int height, void *data)
{
	D3D11_BOX texbox = {xofs, yofs, 0, xofs + width, yofs + height, 1};
	d3d_Context->lpVtbl->UpdateSubresource (d3d_Context, (ID3D11Resource *) d3d_Textures[texnum].Texture, level, &texbox, data, width << 2, 0);
}


void D_LoadTextureData8 (int texnum, int level, int xofs, int yofs, int width, int height, byte *src, unsigned *palette)
{
	int hunkmark = Hunk_LowMark (LOAD_HUNK);
	unsigned *trans = Image_8to32 (src, width, height, width, palette, 0);

	D_LoadTextureData32 (texnum, level, xofs, yofs, width, height, trans);
	Hunk_FreeToLowMark (LOAD_HUNK, hunkmark);
}


void D_CreateTextureMiplevels (int texnum, unsigned *data, int width, int height, int flags)
{
	D_LoadTextureData32 (texnum, 0, 0, 0, width, height, data);

	if (flags & TEX_MIPMAP)
	{
		int mipnum;

		for (mipnum = 1; width > 1 || height > 1; mipnum++)
		{
			// choose the appropriate filter
			if ((width & 1) || (height & 1))
				data = Image_Resample (data, width, height);
			else data = Image_Mipmap (data, width, height);

			if ((width = width >> 1) < 1) width = 1;
			if ((height = height >> 1) < 1) height = 1;

			D_LoadTextureData32 (texnum, mipnum, 0, 0, width, height, data);
		}
	}
}


void D_FillTexture8 (int texnum, byte *data, int width, int height, int stride, unsigned *palette, int flags)
{
	int hunkmark = Hunk_LowMark (LOAD_HUNK);
	unsigned *trans = Image_8to32 (data, width, height, stride, palette, flags);

	D_FillTexture32 (texnum, trans, width, height, stride, flags);
	Hunk_FreeToLowMark (LOAD_HUNK, hunkmark);
}


void D_FillTexture32 (int texnum, unsigned *data, int width, int height, int stride, int flags)
{
	int hunkmark = Hunk_LowMark (LOAD_HUNK);

	D_CreateTextureMiplevels (texnum, data, width, height, flags);
	Hunk_FreeToLowMark (LOAD_HUNK, hunkmark);
}


unsigned short D_CRCTexture (int width, int height, int stride, byte *data)
{
	int i;
	unsigned short crc = CRC_Init ();

	for (i = 0; i < height; i++)
	{
		crc = CRC_Line (crc, data, width);
		data += stride;
	}

	return crc;
}


int D_LoadTexture (char *identifier, int width, int height, int stride, byte *data, unsigned *palette, int flags)
{
	int texnum = 0;
	unsigned short crc = data ? D_CRCTexture (width, height, stride, data) : 0;

	// see if the texture is allready present
	if (identifier[0])
	{
		// look for a cache match
		if ((texnum = D_FindTexture (identifier, width, height, 1, crc, flags)) != 0)
		{
			// reuse this texture
			return texnum;
		}
	}

	// find a free gltexture_t
	if ((texnum = D_GetTexture (identifier, width, height, 1, crc, flags)) != 0)
	{
		// call the appropriate loader
		if (data && palette)
			D_FillTexture8 (texnum, data, width, height, stride, palette, flags);
		else if (data)
			D_FillTexture32 (texnum, (unsigned *) data, width, height, stride, flags);

		return texnum;
	}

	Sys_Error ("D_LoadTexture: MAX_GLTEXTURES exceeded");
	return 0; // shut up compiler
}


void D_FreeUnusedTextures (void)
{
	int i;

	for (i = 0; i < MAX_D3D_TEXTURES; i++)
	{
		d3d_texture_t *tex = &d3d_Textures[i];

		// not a texture
		if (!tex->Texture) continue;

		// not disposable
		if (!(tex->Flags & TEX_DISPOSABLE)) continue;

		// used in this registration sequence
		if (tex->RegistrationSequence == tex_RegistrationSequence) continue;

		// OK to dispose of
		SAFE_RELEASE (tex->Texture);
		SAFE_RELEASE (tex->SRV);
		SAFE_RELEASE (tex->RTV);

		memset (tex, 0, sizeof (d3d_texture_t));
	}

	// go to the next registration sequence
	tex_RegistrationSequence++;
}


BOOL ScrapAllocBlock (int *Allocated, int w, int h, int *x, int *y)
{
	int i;
	int best = SCRAP_SIZE;

	for (i = 0; i < SCRAP_SIZE - w; i++)
	{
		int best2 = 0;
		int j = 0;

		for (j = 0; j < w; j++)
		{
			if (Allocated[i + j] >= best)
				break;

			if (Allocated[i + j] > best2)
				best2 = Allocated[i + j];
		}

		if (j == w)
		{
			// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > SCRAP_SIZE)
		return FALSE;

	// mark off allocated regions
	for (i = 0; i < w; i++)
		Allocated[*x + i] = best + h;

	return TRUE;
}


int D_LoadScrapTexture (int width, int height, byte *data, unsigned *palette, int *scrap_x, int *scrap_y)
{
	// this will be the current scrap texture
	static int d3d_ScrapTexture = 0;
	static int ScrapAllocated[SCRAP_SIZE];

	// flush any current 2D drawing because the texture will be updated
	D_DrawFlush ();

	if (!d3d_ScrapTexture)
	{
		// clear allocations
		memset (ScrapAllocated, 0, sizeof (ScrapAllocated));

		// and take a new texture for the scrap
		d3d_ScrapTexture = D_GetTexture ("", SCRAP_SIZE, SCRAP_SIZE, 1, 0, TEX_ALPHA | TEX_SCRAP);
	}

	// now try to alloc it; padding by 2 (to satisfy linear filtering) so that we don't need to adjust texcoords
	if (!ScrapAllocBlock (ScrapAllocated, width + 2, height + 2, scrap_x, scrap_y))
	{
		// woops; won't fit in this texture
		d3d_ScrapTexture = 0;

		// call recursively to get a new texture
		return D_LoadScrapTexture (width, height, data, palette, scrap_x, scrap_y);
	}
	else
	{
		// load it on
		D_LoadTextureData8 (d3d_ScrapTexture, 0, *scrap_x, *scrap_y, width, height, data, palette);

		// and this is now a good texture
		return d3d_ScrapTexture;
	}
}


void D_PSSetShaderResource (UINT Slot, int texnum)
{
	static ID3D11ShaderResourceView *SRVs[MAX_TEXTURE_STAGES];

	if (Slot < 0) return;
	if (Slot >= MAX_TEXTURE_STAGES) return;

	if (SRVs[Slot] != d3d_Textures[texnum].SRV)
	{
		D_DrawFlush (); // this can happen during 2D drawing
		d3d_Context->lpVtbl->PSSetShaderResources (d3d_Context, Slot, 1, &d3d_Textures[texnum].SRV);
		SRVs[Slot] = d3d_Textures[texnum].SRV;
	}
}


void D_ClearTextureBindings (void)
{
	UINT stage;

	// texture number 0 is never used so set all tetxure stages to that which will force a recache
	for (stage = 0; stage < MAX_TEXTURE_STAGES; stage++)
		D_PSSetShaderResource (stage, 0);
}


void D_SetRenderTargetTexture (int texnum)
{
	if (d3d_Textures[texnum].RTV)
		D_SetRenderTargetView (d3d_Textures[texnum].RTV);
	else D_RestoreDefaultRenderTargets ();
}


void D_PurgePlayerTextures (void)
{
	int i;

	for (i = 0; i < MAX_D3D_TEXTURES; i++)
	{
		d3d_texture_t *tex = &d3d_Textures[i];

		// not a texture
		if (!tex->Texture) continue;

		// not disposable
		if (!(tex->Flags & TEX_PLAYERTEXTURE)) continue;

		// OK to dispose of
		SAFE_RELEASE (tex->Texture);
		SAFE_RELEASE (tex->SRV);
		SAFE_RELEASE (tex->RTV);

		memset (tex, 0, sizeof (d3d_texture_t));
	}
}


void D_ClearRenderTarget (int texnum, float *color)
{
	d3d_Context->lpVtbl->ClearRenderTargetView (d3d_Context, d3d_Textures[texnum].RTV, color);
}


int D_CreateRenderTargetAtBackbufferSize (void)
{
	ID3D11Texture2D *pBackBuffer = NULL;
	D3D11_TEXTURE2D_DESC descRT;

	// Get a pointer to the back buffer
	if (FAILED (d3d_SwapChain->lpVtbl->GetBuffer (d3d_SwapChain, 0, &IID_ID3D11Texture2D, (LPVOID *) &pBackBuffer)))
	{
		Sys_Error ("D_CreateRenderTargetAtBackbufferSize : d3d_SwapChain->GetBuffer failed");
		return 0;
	}

	// get the description of the backbuffer for creating the new rendertarget from it
	pBackBuffer->lpVtbl->GetDesc (pBackBuffer, &descRT);
	pBackBuffer->lpVtbl->Release (pBackBuffer);

	// and create it
	return D_GetTexture ("", descRT.Width, descRT.Height, 1, 0, TEX_RENDERTARGET);
}

