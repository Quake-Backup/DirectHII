
#include "precompiled.h"

#include "d_local.h"
#include "d_video.h"
#include "d_image.h"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")

// this just needs to be included anywhere....
#pragma comment (lib, "dxguid.lib")


/*
=========================================================================================================================================================================

MAIN OBJECTS

=========================================================================================================================================================================
*/

// main d3d objects
ID3D11Device *d3d_Device = NULL;
ID3D11DeviceContext *d3d_Context = NULL;
IDXGISwapChain *d3d_SwapChain = NULL;

ID3D11RenderTargetView *d3d_RenderTarget = NULL;
ID3D11DepthStencilView *d3d_DepthBuffer = NULL;


void D_SetViewport (int x, int y, int w, int h, float zn, float zf)
{
	D3D11_VIEWPORT vp = {x, y, w, h, zn, zf};
	d3d_Context->lpVtbl->RSSetViewports (d3d_Context, 1, &vp);
}


/*
=========================================================================================================================================================================

OBJECT LIFETIME MANAGEMENT

=========================================================================================================================================================================
*/

#define MAX_VID_HANDLERS 128

static vid_handler_t vid_OnReleaseHandlers[MAX_VID_HANDLERS];
static vid_handler_t vid_OnLostHandlers[MAX_VID_HANDLERS];
static vid_handler_t vid_OnResetHandlers[MAX_VID_HANDLERS];
static int vid_numhandlers = 0;

void VID_DefaultHandler (void)
{
	// the default handler does nothing and may be used as a placeholder so that all handlers are non-NULL
}


void VID_RegisterHandlers (vid_handler_t OnRelease, vid_handler_t OnLost, vid_handler_t OnReset)
{
	if (vid_numhandlers < MAX_VID_HANDLERS)
	{
		vid_OnReleaseHandlers[vid_numhandlers] = OnRelease;
		vid_OnLostHandlers[vid_numhandlers] = OnLost;
		vid_OnResetHandlers[vid_numhandlers] = OnReset;
		vid_numhandlers++;
	}
}


void VID_HandleOnLost (void)
{
	int i;

	if (!d3d_Device) return;
	if (!d3d_Context) return;
	if (!d3d_SwapChain) return;

	for (i = 0; i < vid_numhandlers; i++)
	{
		if (vid_OnLostHandlers[i]) vid_OnLostHandlers[i] ();
	}
}


void VID_HandleOnReset (void)
{
	int i;

	if (!d3d_Device) return;
	if (!d3d_Context) return;
	if (!d3d_SwapChain) return;

	for (i = 0; i < vid_numhandlers; i++)
	{
		if (vid_OnResetHandlers[i]) vid_OnResetHandlers[i] ();
	}
}


void VID_HandleOnRelease (void)
{
	int i;

	if (!d3d_Device) return;
	if (!d3d_Context) return;
	if (!d3d_SwapChain) return;

	for (i = 0; i < vid_numhandlers; i++)
	{
		if (vid_OnReleaseHandlers[i]) vid_OnReleaseHandlers[i] ();
	}
}


// object cache lets us store out objects on creation so that indivdual routines don't need to destroy them
#define MAX_OBJECT_CACHE	1024

typedef struct d3dobject_s {
	ID3D11DeviceChild *Object;
	char *name;
} d3dobject_t;

d3dobject_t ObjectCache[MAX_OBJECT_CACHE];
int NumObjectCache = 0;

void D_CacheObject (ID3D11DeviceChild *Object, const char *name)
{
	if (!Object)
		return;
	else if (NumObjectCache < MAX_OBJECT_CACHE)
	{
		ObjectCache[NumObjectCache].Object = Object;
		ObjectCache[NumObjectCache].name = (char *) Zone_Alloc (strlen (name) + 1);
		strcpy (ObjectCache[NumObjectCache].name, name);

		NumObjectCache++;
	}
	else Sys_Error ("R_CacheObject : object cache overflow!");
}


void D_ReleaseObjectCache (void)
{
	int i;

	for (i = 0; i < MAX_OBJECT_CACHE; i++)
		SAFE_RELEASE (ObjectCache[i].Object);

	memset (ObjectCache, 0, sizeof (ObjectCache));
}


/*
=========================================================================================================================================================================

VIDEO MODE ENUMERATION AND MANAGEMENT

Video modes are stored as an array of DXGI_MODE_DESC.
Video modes are NOT exposed to non-D_ code; if information about a mode is needed, a getter function will return it by modenum.
Mode 0 is the windowed mode and is updated "live" with values for whatever the current windowed mode is.
Modes 1 onwards are fullscreen modes.

=========================================================================================================================================================================
*/

#define MAX_VIDEO_MODES 1024 // some day this won't be enough...

DXGI_MODE_DESC d3d_VideoModes[MAX_VIDEO_MODES];
int d3d_NumVideoModes = 0;


int D_TryEnumerateModes (IDXGIOutput *output, DXGI_MODE_DESC *ModeList, DXGI_FORMAT fmt)
{
	UINT NumModes = 0;
	const UINT EnumFlags = (DXGI_ENUM_MODES_SCALING | DXGI_ENUM_MODES_INTERLACED);

	// get modes on this adapter (note this is called twice per design) - note that the first time ModeList must be NULL or it will return 0 modes
	if (SUCCEEDED (output->lpVtbl->GetDisplayModeList (output, fmt, EnumFlags, &NumModes, NULL)))
	{
		if (SUCCEEDED (output->lpVtbl->GetDisplayModeList (output, fmt, EnumFlags, &NumModes, ModeList)))
			return NumModes;
		else return 0;
	}
	else return 0;
}


IDXGIOutput *D_GetOutput (IDXGIAdapter *d3d_Adapter)
{
	int i;

	for (i = 0; ; i++)
	{
		IDXGIOutput *d3d_Output = NULL;
		DXGI_OUTPUT_DESC Desc;

		if ((d3d_Adapter->lpVtbl->EnumOutputs (d3d_Adapter, i, &d3d_Output)) == DXGI_ERROR_NOT_FOUND) break;

		if (FAILED (d3d_Output->lpVtbl->GetDesc (d3d_Output, &Desc)))
		{
			SAFE_RELEASE (d3d_Output);
			continue;
		}

		if (!Desc.AttachedToDesktop)
		{
			SAFE_RELEASE (d3d_Output);
			continue;
		}

		if (Desc.Rotation == DXGI_MODE_ROTATION_ROTATE90 || Desc.Rotation == DXGI_MODE_ROTATION_ROTATE180 || Desc.Rotation == DXGI_MODE_ROTATION_ROTATE270)
		{
			SAFE_RELEASE (d3d_Output);
			continue;
		}

		// this is a valid output now
		return d3d_Output;
	}

	// not found
	return NULL;
}


void D_EnumerateVideoModes (void)
{
	int i, j;
	IDXGIFactory *pFactory = NULL;
	IDXGIOutput *d3d_Output = NULL;
	IDXGIAdapter *d3d_Adapter = NULL;
	UINT NumModes = 0;
	int MinWindowedWidth, MinWindowedHeight;
	int MaxWindowedWidth, MaxWindowedHeight;
	DXGI_MODE_DESC *EnumModes = (DXGI_MODE_DESC *) Hunk_Alloc (LOAD_HUNK, 0);

	// fixme - enum these properly
	if (FAILED (CreateDXGIFactory (&IID_IDXGIFactory, (void **) &pFactory)))
		Sys_Error ("D_EnumerateVideoModes : CreateDXGIFactory failed");

	if ((pFactory->lpVtbl->EnumAdapters (pFactory, 0, &d3d_Adapter)) == DXGI_ERROR_NOT_FOUND)
		Sys_Error ("D_EnumerateVideoModes : IDXGIFactory failed to enumerate Adapter 0");

	if ((d3d_Output = D_GetOutput (d3d_Adapter)) == NULL)
		Sys_Error ("D_EnumerateVideoModes : IDXGIFactory failed to enumerate Outputs on Adapter 0");

	// enumerate the modes, starting at mode 1 because mode 0 is the windowed mode
	if ((NumModes = D_TryEnumerateModes (d3d_Output, EnumModes, DXGI_FORMAT_R8G8B8A8_UNORM)) == 0)
		Sys_Error ("D_EnumerateVideoModes : Failed to enumerate any 32bpp modes!");

	// make sure our objects are properly killed
	SAFE_RELEASE (d3d_Output);
	SAFE_RELEASE (d3d_Adapter);
	SAFE_RELEASE (pFactory);

	// cleanup the modes by removing unspecified modes where a specified option or options exists
	// then copy them out to the final array, starting at 1 because mode 0 is the windowed mode
	for (i = 0, d3d_NumVideoModes = 1; i < NumModes; i++)
	{
		DXGI_MODE_DESC *mode = &EnumModes[i];
		BOOL deadmode = FALSE;

		if (mode->Scaling == DXGI_MODE_SCALING_UNSPECIFIED)
		{
			for (j = 0; j < NumModes; j++)
			{
				DXGI_MODE_DESC *mode2 = &EnumModes[j];

				if (mode->Format != mode2->Format) continue;
				if (mode->Height != mode2->Height) continue;
				if (mode->RefreshRate.Denominator != mode2->RefreshRate.Denominator) continue;
				if (mode->RefreshRate.Numerator != mode2->RefreshRate.Numerator) continue;
				if (mode->ScanlineOrdering != mode2->ScanlineOrdering) continue;
				if (mode->Width != mode2->Width) continue;

				if (mode2->Scaling != DXGI_MODE_SCALING_UNSPECIFIED)
				{
					deadmode = TRUE;
					break;
				}
			}
		}

		if (mode->ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED)
		{
			for (j = 0; j < NumModes; j++)
			{
				DXGI_MODE_DESC *mode2 = &EnumModes[j];

				if (mode->Format != mode2->Format) continue;
				if (mode->Height != mode2->Height) continue;
				if (mode->RefreshRate.Denominator != mode2->RefreshRate.Denominator) continue;
				if (mode->RefreshRate.Numerator != mode2->RefreshRate.Numerator) continue;
				if (mode->Scaling != mode2->Scaling) continue;
				if (mode->Width != mode2->Width) continue;

				if (mode2->ScanlineOrdering != DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED)
				{
					deadmode = TRUE;
					break;
				}
			}
		}

		if (deadmode) continue;

		// this is a real mode now - copy it out
		memcpy (&d3d_VideoModes[d3d_NumVideoModes], &EnumModes[i], sizeof (DXGI_MODE_DESC));
		d3d_NumVideoModes++;
	}

	// set up the initial values
	MaxWindowedWidth = d3d_VideoModes[d3d_NumVideoModes - 1].Width;
	MaxWindowedHeight = d3d_VideoModes[d3d_NumVideoModes - 1].Height;
	MinWindowedWidth = d3d_VideoModes[1].Width;
	MinWindowedHeight = d3d_VideoModes[1].Height;

	if (d3d_NumVideoModes > 2)
	{
		for (i = d3d_NumVideoModes - 1; i; i--)
		{
			if (d3d_VideoModes[i].Width < MaxWindowedWidth)
			{
				MaxWindowedWidth = d3d_VideoModes[i].Width;
				break;
			}
		}

		for (i = d3d_NumVideoModes - 1; i; i--)
		{
			if (d3d_VideoModes[i].Height < MaxWindowedHeight)
			{
				MaxWindowedHeight = d3d_VideoModes[i].Height;
				break;
			}
		}
	}

	// set up the windowed mode with the mode info
	memcpy (&d3d_VideoModes[0], &d3d_VideoModes[d3d_NumVideoModes - 1], sizeof (DXGI_MODE_DESC));

	d3d_VideoModes[0].Width = MinWindowedWidth;
	d3d_VideoModes[0].Height = MinWindowedHeight;
}


int D_FindVideoMode (int width, int height, qboolean windowed)
{
	// if neither width nor height are specified, if fullscreen, then the simple case is the best mode available
	if (!width && !height && !windowed)
		return d3d_NumVideoModes - 1;

	if (windowed)
	{
		// mode 0 is windowed; the values were already filled-in at startup so only replace them if requested to do so
		if (width) d3d_VideoModes[0].Width = width;
		if (height) d3d_VideoModes[0].Height = height;
		return 0;
	}
	else
	{
		// try to find the first mode where the dimensions equal or exceed requested width and height
		int i;

		for (i = 0; i < d3d_NumVideoModes; i++)
			if (d3d_VideoModes[i].Width >= width && d3d_VideoModes[i].Height >= height)
				return i;

		// not found; try to find one where where the dimensions equal or exceed requested width
		for (i = 0; i < d3d_NumVideoModes; i++)
			if (d3d_VideoModes[i].Width >= width)
				return i;

		// not found; try to find one where where the dimensions equal or exceed requested height
		for (i = 0; i < d3d_NumVideoModes; i++)
			if (d3d_VideoModes[i].Height >= height)
				return i;

		// not found; just take the best mode available
		return d3d_NumVideoModes - 1;
	}
}


int D_GetModeWidth (int modenum)
{
	if (modenum < 0)
		return d3d_VideoModes[0].Width;
	else if (modenum >= d3d_NumVideoModes)
		return d3d_VideoModes[d3d_NumVideoModes - 1].Width;
	else return d3d_VideoModes[modenum].Width;
}


int D_GetModeHeight (int modenum)
{
	if (modenum < 0)
		return d3d_VideoModes[0].Height;
	else if (modenum >= d3d_NumVideoModes)
		return d3d_VideoModes[d3d_NumVideoModes - 1].Height;
	else return d3d_VideoModes[modenum].Height;
}


char *D_GetModeDescription (int mode)
{
	if (mode < 0)
		return D_GetModeDescription (0);

	if (mode >= d3d_NumVideoModes)
		return D_GetModeDescription (d3d_NumVideoModes - 1);

	if (mode > 0)
		return va ("[%ix%i Fullscreen]", d3d_VideoModes[mode].Width, d3d_VideoModes[mode].Height);
	else return va ("[%ix%i Windowed]", d3d_VideoModes[mode].Width, d3d_VideoModes[mode].Height);
}


/*
=========================================================================================================================================================================

VIDEO STARTUP AND DEVICE CREATION

=========================================================================================================================================================================
*/

void D_CreateFrameBuffer (void)
{
	ID3D11Texture2D *pBackBuffer = NULL;
	D3D11_TEXTURE2D_DESC descRT;

	ID3D11Texture2D *pDepthStencil = NULL;
	D3D11_TEXTURE2D_DESC descDepth;

	// Get a pointer to the back buffer
	if (FAILED (d3d_SwapChain->lpVtbl->GetBuffer (d3d_SwapChain, 0, &IID_ID3D11Texture2D, (LPVOID *) &pBackBuffer)))
	{
		Sys_Error ("d3d_SwapChain->GetBuffer failed");
		return;
	}

	// get the description of the backbuffer for creating the depth buffer from it
	pBackBuffer->lpVtbl->GetDesc (pBackBuffer, &descRT);

	// Create a render-target view
	d3d_Device->lpVtbl->CreateRenderTargetView (d3d_Device, (ID3D11Resource *) pBackBuffer, NULL, &d3d_RenderTarget);
	pBackBuffer->lpVtbl->Release (pBackBuffer);

	// create the depth buffer with the same dimensions and multisample levels as the backbuffer
	descDepth.Width = descRT.Width;
	descDepth.Height = descRT.Height;
	descDepth.SampleDesc.Count = descRT.SampleDesc.Count;
	descDepth.SampleDesc.Quality = descRT.SampleDesc.Quality;

	// fill in the rest of it
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;

	// and create it
	if (SUCCEEDED (d3d_Device->lpVtbl->CreateTexture2D (d3d_Device, &descDepth, NULL, &pDepthStencil)))
	{
		// Create the depth stencil view
		d3d_Device->lpVtbl->CreateDepthStencilView (d3d_Device, (ID3D11Resource *) pDepthStencil, NULL, &d3d_DepthBuffer);		pDepthStencil->lpVtbl->Release (pDepthStencil);
	}

	// and set them as active
	D_RestoreDefaultRenderTargets ();
}


void D_CreateD3DDevice (HWND hWnd, int modenum)
{
	DXGI_SWAP_CHAIN_DESC sd;
	IDXGIFactory *pFactory = NULL;

	// we don't support any d3d9 feature levels
	D3D_FEATURE_LEVEL FeatureLevels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};

	memset (&sd, 0, sizeof (sd));
	memcpy (&sd.BufferDesc, &d3d_VideoModes[modenum], sizeof (DXGI_MODE_DESC));

	sd.BufferCount = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// always initially create a windowed swapchain at the desired resolution, then switch to fullscreen post-creation if it was requested
	sd.Windowed = TRUE;

	// now we try to create it
	if (FAILED (D3D11CreateDeviceAndSwapChain (
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		0, //D3D11_CREATE_DEVICE_DEBUG,
		FeatureLevels,
		sizeof (FeatureLevels) / sizeof (FeatureLevels[0]),
		D3D11_SDK_VERSION,
		&sd,
		&d3d_SwapChain,
		&d3d_Device,
		NULL,
		&d3d_Context)))
	{
		Sys_Error ("D3D11CreateDeviceAndSwapChain failed");
		return;
	}

	// now we disable stuff that we want to handle ourselves
	if (SUCCEEDED (d3d_SwapChain->lpVtbl->GetParent (d3d_SwapChain, &IID_IDXGIFactory, (void **) &pFactory)))
	{
		pFactory->lpVtbl->MakeWindowAssociation (pFactory, hWnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
		pFactory->lpVtbl->Release (pFactory);
	}

	// create the initial frame buffer that we're going to use for all of our rendering
	D_CreateFrameBuffer ();

	// load all of our initial objects
	// each subsystem creates it's objects then registers it's handlers, following which the reset handler runs to complete object creation
	D_ShaderRegister ();
	D_StateRegister ();
	D_DrawRegister ();
	D_SpriteRegister ();
	D_PartRegister ();
	D_WaterwarpRegister ();
	D_TextureRegister ();
	D_SurfRegister ();
	D_LightRegister ();
	D_MainRegister ();
	D_AliasRegister ();
	D_QuadBatchRegister ();

	// now run the reset handler to create the rest of our objects
	VID_HandleOnReset ();
}


/*
=========================================================================================================================================================================

MODE CHANGING

=========================================================================================================================================================================
*/

void D_HandleAltEnter (HWND hWnd, DWORD WindowStyle, DWORD ExWindowStyle)
{
	/*
	D3D9_REMOVED
	static D3DPRESENT_PARAMETERS d3d_LastPresentParameters;

	// to do: if switching to fullscreen, store out the last windowed resolution so that we can switch back to it
	if (d3d_PresentParameters.Windowed)
	{
		memcpy (&d3d_LastPresentParameters, &d3d_PresentParameters, sizeof (D3DPRESENT_PARAMETERS));
		D_ResetD3DDevice (hWnd, d3d_DesktopMode.Width, d3d_DesktopMode.Height, TRUE, FALSE);
	}
	else
		D_ResetD3DDevice (hWnd, 800, 600, FALSE, FALSE);

	// center the screen if windowed
	if (d3d_PresentParameters.Windowed)
	{
		RECT WorkArea;
		RECT WindowRect;

		SetRect (&WindowRect, 0, 0, d3d_PresentParameters.BackBufferWidth, d3d_PresentParameters.BackBufferHeight);
		AdjustWindowRectEx (&WindowRect, WindowStyle, FALSE, ExWindowStyle);
		SystemParametersInfo (SPI_GETWORKAREA, 0, &WorkArea, 0);

		SetWindowPos (
			hWnd,
			HWND_TOP,
			WorkArea.left + ((WorkArea.right - WorkArea.left) - (WindowRect.right - WindowRect.left)) / 2,
			WorkArea.top + ((WorkArea.bottom - WorkArea.top) - (WindowRect.bottom - WindowRect.top)) / 2,
			(WindowRect.right - WindowRect.left),
			(WindowRect.bottom - WindowRect.top),
			0
		);
	}
	*/
}


/*
=========================================================================================================================================================================

VIDEO SHUTDOWN

=========================================================================================================================================================================
*/

void D_DestroyD3DDevice (void)
{
	if (d3d_Device && d3d_SwapChain && d3d_Context)
	{
		float clearColor[] = {0, 0, 0, 0};

		// DXGI prohibits destruction of a swapchain when in a fullscreen mode so go back to windowed first
		d3d_SwapChain->lpVtbl->SetFullscreenState (d3d_SwapChain, FALSE, NULL);

		// clear to black
		d3d_Context->lpVtbl->ClearRenderTargetView (d3d_Context, d3d_RenderTarget, clearColor);
		d3d_Context->lpVtbl->ClearDepthStencilView (d3d_Context, d3d_DepthBuffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 1);

		// and run a present to make them show
		D_RestoreDefaultRenderTargets ();
		d3d_SwapChain->lpVtbl->Present (d3d_SwapChain, 0, 0);

		// ensure that the window paints
		Sys_SendKeyEvents ();
		Sleep (5);

		// finally chuck out all cached state in the context
		d3d_Context->lpVtbl->OMSetRenderTargets (d3d_Context, 0, NULL, NULL);
		d3d_Context->lpVtbl->ClearState (d3d_Context);
		d3d_Context->lpVtbl->Flush (d3d_Context);
	}

	// handle special objects
	VID_HandleOnLost ();
	VID_HandleOnRelease ();

	// handle cacheable objects
	D_ReleaseObjectCache ();

	// destroy main objects
	SAFE_RELEASE (d3d_DepthBuffer);
	SAFE_RELEASE (d3d_RenderTarget);

	SAFE_RELEASE (d3d_SwapChain);
	SAFE_RELEASE (d3d_Context);
	SAFE_RELEASE (d3d_Device);
}


/*
=========================================================================================================================================================================

MAIN SCENE/RENDERTARGETS

=========================================================================================================================================================================
*/

void D_BeginRendering (void)
{
	// bind everything that needs to be bound once-per-frame
	D_BindConstantBuffers ();
	D_BindSamplers ();

	// everything in all draws is drawn as an indexed triangle list, even if it's ultimately a strip or a single tri, so
	// this can be set-and-forget once per frame
	d3d_Context->lpVtbl->IASetPrimitiveTopology (d3d_Context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}


void D_EndRendering (int syncInterval)
{
	d3d_SwapChain->lpVtbl->Present (d3d_SwapChain, syncInterval, 0);
}


void D_CaptureScreenshot (char *checkname, float gammaval)
{
	int i;
	byte inversegamma[256];
	ID3D11Texture2D *pBackBuffer = NULL;
	ID3D11Texture2D *pScreenShot = NULL;
	D3D11_TEXTURE2D_DESC descRT;
	D3D11_MAPPED_SUBRESOURCE msr;

	// Get a pointer to the back buffer
	if (FAILED (d3d_SwapChain->lpVtbl->GetBuffer (d3d_SwapChain, 0, &IID_ID3D11Texture2D, (LPVOID *) &pBackBuffer)))
	{
		VID_Printf ("d3d_SwapChain->GetBuffer failed");
		return;
	}

	// get the description of the backbuffer for creating the depth buffer from it
	pBackBuffer->lpVtbl->GetDesc (pBackBuffer, &descRT);

	// set the stuff we want to differ
	descRT.Usage = D3D11_USAGE_STAGING;
	descRT.BindFlags = 0;
	descRT.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;

	// create a copy of the back buffer
	if (FAILED (d3d_Device->lpVtbl->CreateTexture2D (d3d_Device, &descRT, NULL, &pScreenShot)))
	{
		VID_Printf ("SCR_GetScreenData : failed to create scratch texture\n");
		goto failed;
	}

	// copy over the back buffer to the screenshot texture
	d3d_Context->lpVtbl->CopyResource (d3d_Context, (ID3D11Resource *) pScreenShot, (ID3D11Resource *) pBackBuffer);
	D_SyncPipeline ();

	if (FAILED (d3d_Context->lpVtbl->Map (d3d_Context, (ID3D11Resource *) pScreenShot, 0, D3D11_MAP_READ_WRITE, 0, &msr)))
	{
		VID_Printf ("SCR_GetScreenData : failed to map scratch texture\n");
		goto failed;
	}

	// build an inverse gamma table
	if (gammaval > 0)
	{
		if (gammaval < 1 || gammaval > 1)
			for (i = 0; i < 256; i++) inversegamma[i] = Image_GammaVal8to8 (i, 1.0f / gammaval);
		else
			for (i = 0; i < 256; i++) inversegamma[i] = i;
	}
	else
		for (i = 0; i < 256; i++) inversegamma[i] = 255 - i;

	// now convert it down
	Image_CollapseRowPitch ((unsigned *) msr.pData, descRT.Width, descRT.Height, msr.RowPitch >> 2);
	Image_Compress32To24RGBtoBGR ((byte *) msr.pData, descRT.Width, descRT.Height);

	// apply inverse gamma
	Image_ApplyTranslationRGB ((byte *) msr.pData, descRT.Width * descRT.Height, inversegamma, inversegamma, inversegamma);

	// and write it out
	Image_WriteDataToTGA (checkname, msr.pData, descRT.Width, descRT.Height, 24);

	// unmap, and...
	d3d_Context->lpVtbl->Unmap (d3d_Context, (ID3D11Resource *) pScreenShot, 0);

	// ...done
	VID_Printf (va ("Wrote %s\n", checkname));

failed:;
	// clean up
	SAFE_RELEASE (pScreenShot);
	SAFE_RELEASE (pBackBuffer);
}


/*
D3D9_REMOVED
int D_GetDisplayWidth (void) {return d3d_PresentParameters.BackBufferWidth;}
int D_GetDisplayHeight (void) {return d3d_PresentParameters.BackBufferHeight;}
*/

void D_SetRenderTargetView (ID3D11RenderTargetView *RTV)
{
	// and set them as active
	d3d_Context->lpVtbl->OMSetRenderTargets (d3d_Context, 1, &RTV, d3d_DepthBuffer);
}


void D_RestoreDefaultRenderTargets (void)
{
	// and set them as active
	d3d_Context->lpVtbl->OMSetRenderTargets (d3d_Context, 1, &d3d_RenderTarget, d3d_DepthBuffer);
}

