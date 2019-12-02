
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include <dsound.h>
#pragma comment (lib, "dsound.lib")

// 64K is > 1 second at 16-bit, 22050 Hz
#define	WAV_BUFFERS				128
#define	WAV_MASK				0x7F
#define	WAV_BUFFER_SIZE			0x0400
#define SECONDARY_BUFFER_SIZE	0x10000

typedef enum {SIS_SUCCESS, SIS_FAILURE, SIS_NOTAVAIL} sndinitstat;

static qboolean	wavonly;
static qboolean	dsound_init;
static qboolean	wav_init;
static qboolean	snd_firsttime = true, snd_isdirect, snd_iswave;

// starts at 0 for disabled
static int	sample16;
static int	snd_sent, snd_completed;


/*
 * Global variables. Must be visible to window-procedure function
 *  so it can unlock and free the data block after it has been played.
 */

HANDLE		hData;
HPSTR		lpData, lpData2;

HGLOBAL		hWaveHdr;
LPWAVEHDR	lpWaveHdr;

HWAVEOUT    hWaveOut;

WAVEOUTCAPS	wavecaps;

DWORD	ds_SndBufSize;

MMTIME		mmstarttime;

IDirectSound8 *ds_Object;
IDirectSoundBuffer *ds_Buffer;

qboolean SNDDMA_InitDirect (void);
qboolean SNDDMA_InitWav (void);


void FreeSound (void);


static const char *DSoundError (int error)
{
	switch (error)
	{
	case DSERR_BUFFERLOST:
		return "DSERR_BUFFERLOST";
	case DSERR_INVALIDCALL:
		return "DSERR_INVALIDCALLS";
	case DSERR_INVALIDPARAM:
		return "DSERR_INVALIDPARAM";
	case DSERR_PRIOLEVELNEEDED:
		return "DSERR_PRIOLEVELNEEDED";
	}

	return "unknown";
}


/*
===================
DS_CreateBuffers
===================
*/
static qboolean DS_CreateBuffers (void)
{
	DSBUFFERDESC	dsbuf;
	DSBCAPS			dsbcaps;
	WAVEFORMATEX	format;
	DWORD			dwWrite;

	memset (&format, 0, sizeof (format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = dma.channels;
	format.wBitsPerSample = dma.samplebits;
	format.nSamplesPerSec = dma.speed;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	Con_Printf (PRINT_NORMAL, "Creating DS buffers\n");
	Con_Printf (PRINT_DEVELOPER, "...setting EXCLUSIVE coop level: ");

	if (DS_OK != ds_Object->lpVtbl->SetCooperativeLevel (ds_Object, vid.hWnd, DSSCL_EXCLUSIVE))
	{
		Con_Printf (PRINT_NORMAL, "failed\n");
		FreeSound ();
		return false;
	}

	Con_Printf (PRINT_DEVELOPER, "ok\n");

	// create the secondary buffer we'll actually work with
	memset (&dsbuf, 0, sizeof (dsbuf));
	dsbuf.dwSize = sizeof (DSBUFFERDESC);
	dsbuf.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_LOCSOFTWARE;
	dsbuf.dwBufferBytes = SECONDARY_BUFFER_SIZE;
	dsbuf.lpwfxFormat = &format;

	memset (&dsbcaps, 0, sizeof (dsbcaps));
	dsbcaps.dwSize = sizeof (dsbcaps);

	Con_Printf (PRINT_DEVELOPER, "...creating secondary buffer: ");

	if (DS_OK != ds_Object->lpVtbl->CreateSoundBuffer (ds_Object, &dsbuf, &ds_Buffer, NULL))
	{
		Con_Printf (PRINT_NORMAL, "failed\n");
		FreeSound ();
		return false;
	}

	Con_Printf (PRINT_DEVELOPER, "ok\n");

	dma.channels = format.nChannels;
	dma.samplebits = format.wBitsPerSample;
	dma.speed = format.nSamplesPerSec;

	if (DS_OK != ds_Buffer->lpVtbl->GetCaps (ds_Buffer, &dsbcaps))
	{
		Con_Printf (PRINT_NORMAL, "*** GetCaps failed ***\n");
		FreeSound ();
		return false;
	}

	Con_Printf (PRINT_NORMAL, "...using secondary sound buffer\n");

	// Make sure mixer is active
	ds_Buffer->lpVtbl->Play (ds_Buffer, 0, 0, DSBPLAY_LOOPING);

	if (snd_firsttime)
		Con_Printf (PRINT_NORMAL, va ("   %d channel(s)\n"
		"   %d bits/sample\n"
		"   %d bytes/sec\n",
		dma.channels, dma.samplebits, dma.speed));

	ds_SndBufSize = dsbcaps.dwBufferBytes;

	/* we don't want anyone to access the buffer directly w/o locking it first. */
	lpData = NULL;

	ds_Buffer->lpVtbl->Stop (ds_Buffer);
	ds_Buffer->lpVtbl->GetCurrentPosition (ds_Buffer, &mmstarttime.u.sample, &dwWrite);
	ds_Buffer->lpVtbl->Play (ds_Buffer, 0, 0, DSBPLAY_LOOPING);

	dma.samples = ds_SndBufSize / (dma.samplebits / 8);
	dma.samplepos = 0;
	dma.submission_chunk = 1;
	dma.buffer = (unsigned char *) lpData;
	sample16 = (dma.samplebits / 8) - 1;

	return true;
}


/*
===================
DS_DestroyBuffers
===================
*/
static void DS_DestroyBuffers (void)
{
	Con_Printf (PRINT_DEVELOPER, "Destroying DS buffers\n");

	if (ds_Object)
	{
		Con_Printf (PRINT_DEVELOPER, "...setting NORMAL coop level\n");
		ds_Object->lpVtbl->SetCooperativeLevel (ds_Object, vid.hWnd, DSSCL_NORMAL);
	}

	if (ds_Buffer)
	{
		Con_Printf (PRINT_DEVELOPER, "...stopping and releasing sound buffer\n");
		ds_Buffer->lpVtbl->Stop (ds_Buffer);
		ds_Buffer->lpVtbl->Release (ds_Buffer);
	}

	ds_Buffer = NULL;
	dma.buffer = NULL;
}


/*
==================
S_BlockSound
==================
*/
void S_BlockSound (void)
{
	CDAudio_Pause ();
	MIDI_Pause ();

	// DirectSound takes care of blocking itself
	if (snd_iswave)
	{
		snd_blocked++;

		if (snd_blocked == 1)
			waveOutReset (hWaveOut);
	}
}


/*
==================
S_UnblockSound
==================
*/
void S_UnblockSound (void)
{
	CDAudio_Resume ();
	MIDI_Resume ();

	// DirectSound takes care of blocking itself
	if (snd_iswave)
	{
		snd_blocked--;
	}
}


/*
==================
FreeSound
==================
*/
void FreeSound (void)
{
	int		i;

	if (ds_Object)
		DS_DestroyBuffers ();

	if (hWaveOut)
	{
		waveOutReset (hWaveOut);

		if (lpWaveHdr)
		{
			for (i = 0; i < WAV_BUFFERS; i++)
				waveOutUnprepareHeader (hWaveOut, lpWaveHdr + i, sizeof (WAVEHDR));
		}

		waveOutClose (hWaveOut);

		if (hWaveHdr)
		{
			GlobalUnlock (hWaveHdr);
			GlobalFree (hWaveHdr);
		}

		if (hData)
		{
			GlobalUnlock (hData);
			GlobalFree (hData);
		}

	}

	if (ds_Object)
	{
		Con_Printf (PRINT_DEVELOPER, "...releasing DS object\n");
		ds_Object->lpVtbl->Release (ds_Object);
	}

	ds_Object = NULL;
	ds_Buffer = NULL;
	hWaveOut = 0;
	hData = 0;
	hWaveHdr = 0;
	lpData = NULL;
	lpWaveHdr = NULL;
	dsound_init = false;
	wav_init = false;
}


/*
==================
SNDDMA_InitDirect

Direct-Sound support
==================
*/
sndinitstat SNDDMA_InitDirect (void)
{
	DSCAPS			dscaps;
	HRESULT			hresult;

	memset ((void *) &dma, 0, sizeof (dma));

	dma.channels = 2;
	dma.samplebits = 16;
	dma.speed = 11025;

	Con_Printf (PRINT_SAFE, "Initializing DirectSound\n");

	Con_Printf (PRINT_DEVELOPER, "...creating DS object: ");

	while ((hresult = DirectSoundCreate8 (NULL, &ds_Object, NULL)) != DS_OK)
	{
		if (hresult != DSERR_ALLOCATED)
		{
			Con_Printf (PRINT_SAFE, "failed\n");
			return SIS_FAILURE;
		}

		if (MessageBox (NULL,
			"The sound hardware is in use by another app.\n\n"
			"Select Retry to try to start sound again or Cancel to run Quake with no sound.",
			"Sound not available",
			MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY)
		{
			Con_Printf (PRINT_SAFE, "failed, hardware already in use\n");
			return SIS_NOTAVAIL;
		}
	}

	Con_Printf (PRINT_DEVELOPER, "ok\n");

	dscaps.dwSize = sizeof (dscaps);

	if (DS_OK != ds_Object->lpVtbl->GetCaps (ds_Object, &dscaps))
	{
		Con_Printf (PRINT_SAFE, "*** couldn't get DS caps ***\n");
	}

	if (dscaps.dwFlags & DSCAPS_EMULDRIVER)
	{
		Con_Printf (PRINT_DEVELOPER, "...no DSound driver found\n");
		FreeSound ();
		return SIS_FAILURE;
	}

	if (!DS_CreateBuffers ())
		return SIS_FAILURE;

	dsound_init = true;

	Con_Printf (PRINT_DEVELOPER, "...completed successfully\n");

	return SIS_SUCCESS;
}


/*
==================
SNDDM_InitWav

Crappy windows multimedia base
==================
*/
qboolean SNDDMA_InitWav (void)
{
	WAVEFORMATEX  format;
	int				i;
	HRESULT			hr;

	snd_sent = 0;
	snd_completed = 0;

	dma.channels = 2;
	dma.samplebits = 16;
	dma.speed = 11025;

	memset (&format, 0, sizeof (format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = dma.channels;
	format.wBitsPerSample = dma.samplebits;
	format.nSamplesPerSec = dma.speed;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	/* Open a waveform device for output using window callback. */
	while ((hr = waveOutOpen ((LPHWAVEOUT) &hWaveOut, WAVE_MAPPER, &format, 0, 0L, CALLBACK_NULL)) != MMSYSERR_NOERROR)
	{
		if (hr != MMSYSERR_ALLOCATED)
		{
			Con_Printf (PRINT_SAFE, "waveOutOpen failed\n");
			return false;
		}

		if (MessageBox (NULL,
						"The sound hardware is in use by another app.\n\n"
						"Select Retry to try to start sound again or Cancel to run Hexen II with no sound.",
						"Sound not available",
						MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY)
		{
			Con_Printf (PRINT_SAFE, "waveOutOpen failure;\n"
							"  hardware already in use\n");
			return false;
		}
	}

	/*
	 * Allocate and lock memory for the waveform data. The memory
	 * for waveform data must be globally allocated with
	 * GMEM_MOVEABLE and GMEM_SHARE flags.

	*/
	ds_SndBufSize = WAV_BUFFERS * WAV_BUFFER_SIZE;
	hData = GlobalAlloc (GMEM_MOVEABLE | GMEM_SHARE, ds_SndBufSize);

	if (!hData)
	{
		Con_Printf (PRINT_SAFE, "Sound: Out of memory.\n");
		FreeSound ();
		return false;
	}

	lpData = GlobalLock (hData);

	if (!lpData)
	{
		Con_Printf (PRINT_SAFE, "Sound: Failed to lock.\n");
		FreeSound ();
		return false;
	}

	memset (lpData, 0, ds_SndBufSize);

	/*
	 * Allocate and lock memory for the header. This memory must
	 * also be globally allocated with GMEM_MOVEABLE and
	 * GMEM_SHARE flags.
	 */
	hWaveHdr = GlobalAlloc (GMEM_MOVEABLE | GMEM_SHARE,
							(DWORD) sizeof (WAVEHDR) * WAV_BUFFERS);

	if (hWaveHdr == NULL)
	{
		Con_Printf (PRINT_SAFE, "Sound: Failed to Alloc header.\n");
		FreeSound ();
		return false;
	}

	lpWaveHdr = (LPWAVEHDR) GlobalLock (hWaveHdr);

	if (lpWaveHdr == NULL)
	{
		Con_Printf (PRINT_SAFE, "Sound: Failed to lock header.\n");
		FreeSound ();
		return false;
	}

	memset (lpWaveHdr, 0, sizeof (WAVEHDR) * WAV_BUFFERS);

	/* After allocation, set up and prepare headers. */
	for (i = 0; i < WAV_BUFFERS; i++)
	{
		lpWaveHdr[i].dwBufferLength = WAV_BUFFER_SIZE;
		lpWaveHdr[i].lpData = lpData + i * WAV_BUFFER_SIZE;

		if (waveOutPrepareHeader (hWaveOut, lpWaveHdr + i, sizeof (WAVEHDR)) !=
				MMSYSERR_NOERROR)
		{
			Con_Printf (PRINT_SAFE, "Sound: failed to prepare wave headers\n");
			FreeSound ();
			return false;
		}
	}

	dma.soundalive = true;
	dma.splitbuffer = false;
	dma.samples = ds_SndBufSize / (dma.samplebits / 8);
	dma.samplepos = 0;
	dma.submission_chunk = 1;
	dma.buffer = (unsigned char *) lpData;
	sample16 = (dma.samplebits / 8) - 1;

	wav_init = true;

	return true;
}

/*
==================
SNDDMA_Init

Try to find a sound device to mix for.
Returns false if nothing is found.
==================
*/

int SNDDMA_Init (void)
{
	sndinitstat	stat;

	memset ((void *) &dma, 0, sizeof (dma));

	if (COM_CheckParm ("-wavonly"))
		wavonly = true;

	dsound_init = wav_init = 0;

	stat = SIS_FAILURE;	// assume DirectSound won't initialize

	/* Init DirectSound */
	if (!wavonly)
	{
		if (snd_firsttime || snd_isdirect)
		{
			stat = SNDDMA_InitDirect ();;

			if (stat == SIS_SUCCESS)
			{
				snd_isdirect = true;

				if (snd_firsttime)
					Con_Printf (PRINT_SAFE, "DirectSound initialized\n");
			}
			else
			{
				snd_isdirect = false;
				Con_Printf (PRINT_SAFE, "DirectSound failed to init\n");
			}
		}
	}

	// if DirectSound didn't succeed in initializing, try to initialize
	// waveOut sound, unless DirectSound failed because the hardware is
	// already allocated (in which case the user has already chosen not
	// to have sound)
	if (!dsound_init && (stat != SIS_NOTAVAIL))
	{
		if (snd_firsttime || snd_iswave)
		{
			snd_iswave = SNDDMA_InitWav ();

			if (snd_iswave)
			{
				if (snd_firsttime)
					Con_Printf (PRINT_SAFE, "Wave sound initialized\n");
			}
			else
			{
				Con_Printf (PRINT_SAFE, "Wave sound failed to init\n");
			}
		}
	}

	snd_firsttime = false;

	if (!dsound_init && !wav_init)
	{
		if (snd_firsttime)
			Con_Printf (PRINT_SAFE, "No sound device initialized\n");

		return 0;
	}

	return 1;
}

/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int SNDDMA_GetDMAPos (void)
{
	MMTIME	mmtime;
	int		s;
	DWORD	dwWrite;

	if (dsound_init)
	{
		mmtime.wType = TIME_SAMPLES;
		ds_Buffer->lpVtbl->GetCurrentPosition (ds_Buffer, &mmtime.u.sample, &dwWrite);
		s = mmtime.u.sample - mmstarttime.u.sample;
	}
	else if (wav_init)
	{
		s = snd_sent * WAV_BUFFER_SIZE;
	}
	else return 0;

	s >>= sample16;
	s &= (dma.samples - 1);

	return s;
}


/*
==============
SNDDMA_BeginPainting

Makes sure dma.buffer is valid
===============
*/
DWORD	locksize;

void SNDDMA_BeginPainting (void)
{
	int		reps;
	DWORD	dwSize2;
	DWORD	*pbuf, *pbuf2;
	HRESULT	hresult;
	DWORD	dwStatus;

	if (!ds_Buffer)
		return;

	// if the buffer was lost or stopped, restore it and/or restart it
	if (ds_Buffer->lpVtbl->GetStatus (ds_Buffer, &dwStatus) != DS_OK)
		Con_Printf (PRINT_NORMAL, "Couldn't get sound buffer status\n");

	if (dwStatus & DSBSTATUS_BUFFERLOST)
		ds_Buffer->lpVtbl->Restore (ds_Buffer);

	if (!(dwStatus & DSBSTATUS_PLAYING))
		ds_Buffer->lpVtbl->Play (ds_Buffer, 0, 0, DSBPLAY_LOOPING);

	// lock the dsound buffer
	reps = 0;
	dma.buffer = NULL;

	while ((hresult = ds_Buffer->lpVtbl->Lock (ds_Buffer, 0, ds_SndBufSize, &pbuf, &locksize, &pbuf2, &dwSize2, 0)) != DS_OK)
	{
		if (hresult != DSERR_BUFFERLOST)
		{
			Con_Printf (PRINT_NORMAL, va ("S_TransferStereo16: Lock failed with error '%s'\n", DSoundError (hresult)));
			S_Shutdown ();
			return;
		}
		else
		{
			ds_Buffer->lpVtbl->Restore (ds_Buffer);
		}

		if (++reps > 2)
			return;
	}

	dma.buffer = (unsigned char *) pbuf;
}


/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit (void)
{
	LPWAVEHDR	h;
	int			wResult;

	if (!dma.buffer)
		return;

	// unlock the dsound buffer
	if (ds_Buffer)
	{
		ds_Buffer->lpVtbl->Unlock (ds_Buffer, dma.buffer, locksize, NULL, 0);
		dma.buffer = NULL;
	}

	if (!wav_init)
		return;

	// find which sound blocks have completed
	while (1)
	{
		if (snd_completed == snd_sent)
		{
			Con_Printf (PRINT_DEVELOPER, "Sound overrun\n");
			break;
		}

		if (!(lpWaveHdr[snd_completed & WAV_MASK].dwFlags & WHDR_DONE))
		{
			break;
		}

		snd_completed++;	// this buffer has been played
	}

	// submit a few new sound blocks
	while (((snd_sent - snd_completed) >> sample16) < 8)
	{
		h = lpWaveHdr + (snd_sent & WAV_MASK);

		if (paintedtime / 256 <= snd_sent)
			break;	//	Con_Printf (PRINT_NORMAL, "submit overrun\n");

		//Con_Printf (va ("send %i\n", snd_sent));
		snd_sent++;
		/*
		 * Now the data block can be sent to the output device. The
		 * waveOutWrite function returns immediately and waveform
		 * data is sent to the output device in the background.
		 */
		wResult = waveOutWrite (hWaveOut, h, sizeof (WAVEHDR));

		if (wResult != MMSYSERR_NOERROR)
		{
			Con_Printf (PRINT_NORMAL, "Failed to write block to device\n");
			FreeSound ();
			return;
		}
	}
}


/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void SNDDMA_Shutdown (void)
{
	FreeSound ();
}


/*
===========
S_Activate

Called when the main window gains or loses focus.
The window have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void S_Activate (qboolean active)
{
	if (active)
	{
		if (ds_Object && vid.hWnd && snd_isdirect)
		{
			DS_CreateBuffers ();
		}
	}
	else
	{
		if (ds_Object && vid.hWnd && snd_isdirect)
		{
			DS_DestroyBuffers ();
		}
	}
}



