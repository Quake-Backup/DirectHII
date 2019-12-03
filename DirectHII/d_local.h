
// D3D objects should never be accessed by non-d_*.c code!
#include <d3d11.h>

#include "mathlib.h"
#include "matrix.h"

// if this is changed then MAX_MOD_KNOWN in model.c must also be changed
#define MAX_D3D_MODELS	1500

// shared with C code
void Sys_Error (char *error, ...);
void VID_Printf (char *msg);
unsigned short CRC_Init (void);
unsigned short CRC_Block (byte *start, int count);
unsigned short CRC_Line (unsigned short crc, byte *start, int count);
char *va (char *format, ...);


// memory
#include "zone.h"

// system
void Sys_SendKeyEvents (void);

// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// main objects
extern ID3D11Device *d3d_Device;
extern ID3D11DeviceContext *d3d_Context;
extern IDXGISwapChain *d3d_SwapChain;

typedef void (*vid_handler_t) (void);

// cacheable objects are persistent for the run of the app and may be stored here for easier disposal
void D_CacheObject (ID3D11DeviceChild *Object, const char *name);

void VID_RegisterHandlers (vid_handler_t OnRelease, vid_handler_t OnLost, vid_handler_t OnReset);
void VID_DefaultHandler (void);
void VID_HandleOnLost (void);
void VID_HandleOnReset (void);
void VID_HandleOnRelease (void);

void D_ShaderRegister (void);
void D_StateRegister (void);
void D_DrawRegister (void);
void D_SpriteRegister (void);
void D_PartRegister (void);
void D_WaterwarpRegister (void);
void D_TextureRegister (void);
void D_SurfRegister (void);
void D_LightRegister (void);
void D_MainRegister (void);
void D_AliasRegister (void);
void D_QuadBatchRegister (void);

void D_SyncPipeline (void);
void D_ClearRenderTarget (int texnum, float *color);
int D_CreateRenderTargetAtBackbufferSize (void);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// helper macros
#define VDECL(name, index, fmt, slot, steprate) { \
	name, \
	index, \
	fmt, \
	slot, \
	D3D11_APPEND_ALIGNED_ELEMENT, \
	((steprate > 0) ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA), \
	steprate \
}

#define DEFINE_LAYOUT(lo) lo, sizeof (lo) / sizeof (lo[0])

#define SAFE_RELEASE(obj) {if (obj) {obj->lpVtbl->Release (obj); obj = NULL;}}


void D_SetInputLayout (ID3D11InputLayout *il);
void D_SetVertexShader (ID3D11VertexShader *vs);
void D_SetGeometryShader (ID3D11GeometryShader *gs);
void D_SetPixelShader (ID3D11PixelShader *ps);


void D_ClearState (void);

#define MAX_TEXTURE_STAGES	16


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// shaders
int D_CreateShaderBundle (int resourceID, const char *vsentry, const char *gsentry, const char *psentry, D3D11_INPUT_ELEMENT_DESC *layout, int numlayout);
void D_BindShaderBundle (int sb);
void D_RegisterConstantBuffer (ID3D11Buffer *cBuffer, int slot);
void D_BindConstantBuffers (void);

void D_PSSetShaderResource (UINT Slot, int texnum);

void D_BindSamplers (void);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// vertex and index buffers
void D_BindVertexBuffer (UINT Slot, ID3D11Buffer *Buffer, UINT Stride, UINT Offset);
void D_BindIndexBuffer (ID3D11Buffer *Buffer, DXGI_FORMAT Format);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// states

extern ID3D11BlendState *d3d_BSNone;
extern ID3D11BlendState *d3d_BSAlphaBlend;
extern ID3D11BlendState *d3d_BSAlphaReverse;
extern ID3D11BlendState *d3d_BSAlphaPreMult;
extern ID3D11BlendState *d3d_BSAdditive;

extern ID3D11DepthStencilState *d3d_DSFullDepth;
extern ID3D11DepthStencilState *d3d_DSDepthNoWrite;
extern ID3D11DepthStencilState *d3d_DSNoDepth;

extern ID3D11RasterizerState *d3d_RSFullCull;
extern ID3D11RasterizerState *d3d_RSNoCull;

void D_SetRenderStates (ID3D11BlendState *bs, ID3D11DepthStencilState *ds, ID3D11RasterizerState *rs);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// waterwarp

void D_SetRenderTargetTexture (int texnum);
void D_SetRenderTargetView (ID3D11RenderTargetView *RTV);
void D_RestoreDefaultRenderTargets (void);


