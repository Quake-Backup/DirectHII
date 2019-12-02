// snd_mem.c: sound caching

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

extern int s_registration_sequence;

/*
================
ResampleSfx
================
*/
void ResampleSfx (sfx_t *sfx, int inrate, int inwidth, byte *data)
{
	int		outcount;
	int		srcsample;
	float	stepscale;
	int		i;
	int		sample, samplefrac, fracstep;
	sfxcache_t	*sc;

	if ((sc = sfx->sndcache) == NULL)
		return;

	stepscale = (float) inrate / dma.speed;	// this is usually 0.5, 1, or 2

	outcount = sc->length / stepscale;
	sc->length = outcount;

	if (sc->loopstart != -1)
		sc->loopstart = sc->loopstart / stepscale;

	sc->speed = dma.speed;
	sc->width = inwidth;

	sc->stereo = 0;

	// resample / decimate to the current source rate

	if (stepscale == 1 && inwidth == 1 && sc->width == 1)
	{
		// fast special case
		for (i = 0; i < outcount; i++)
			((signed char *) sc->data)[i] = (int) ((unsigned char) (data[i]) - 128);
	}
	else
	{
		// general case
		samplefrac = 0;
		fracstep = stepscale * 256;

		for (i = 0; i < outcount; i++)
		{
			srcsample = samplefrac >> 8;
			samplefrac += fracstep;

			if (inwidth == 2)
				sample = LittleShort (((short *) data)[srcsample]);
			else sample = (int) ((unsigned char) (data[srcsample]) - 128) << 8;

			if (sc->width == 2)
				((short *) sc->data)[i] = sample;
			else ((signed char *) sc->data)[i] = sample >> 8;
		}
	}
}

//=============================================================================

/*
==============
S_LoadSound
==============
*/
sfxcache_t *S_ActuallyLoadSound (sfx_t *s)
{
	byte	*data;
	wavinfo_t	info;
	int		len;
	float	stepscale;
	sfxcache_t	*sc;

	// see if still in memory
	if ((sc = s->sndcache) != NULL)
		return sc;

	// load it in
	if ((data = FS_LoadFile (LOAD_HUNK, va ("sound/%s", s->name))) == NULL)
	{
		// ne_tower gets this
		if ((data = FS_LoadFile (LOAD_HUNK, s->name)) == NULL)
		{
			Con_Printf (PRINT_NORMAL, va ("Couldn't load %s\n", s->name));
			return NULL;
		}
	}

	// hey! you can do this!
	if ((info = GetWavinfo (s->name, data, com_filesize)).channels != 1)
	{
		Con_Printf (PRINT_NORMAL, va ("%s is not a mono sample\n", s->name));
		return NULL;
	}

	Con_Printf (PRINT_DEVELOPER, va ("S_LoadSound : %s\n", s->name));

	stepscale = (float) info.rate / dma.speed;

	len = info.samples / stepscale;
	len = len * info.width * info.channels;

	// can this actually ever fail?
	if ((sc = s->sndcache = Zone_Alloc (len + sizeof (sfxcache_t))) == NULL)
		return NULL;

	sc->length = info.samples;
	sc->loopstart = info.loopstart;
	sc->speed = info.rate;
	sc->width = info.width;
	sc->stereo = info.channels;

	ResampleSfx (s, sc->speed, sc->width, data + info.dataofs);

	return sc;
}


sfxcache_t *S_LoadSound (sfx_t *s)
{
	// done this way so we can return NULL from S_ActuallyLoadSound but still handle s->registration_sequence more cleanly
	int mark = Hunk_LowMark (LOAD_HUNK);
	sfxcache_t *sc = S_ActuallyLoadSound (s);

	s->registration_sequence = s_registration_sequence;
	Hunk_FreeToLowMark (LOAD_HUNK, mark);

	return sc;
}


/*
===============================================================================

WAV loading

===============================================================================
*/


byte	*data_p;
byte 	*iff_end;
byte 	*last_chunk;
byte 	*iff_data;
int 	iff_chunk_len;


short GetLittleShort (void)
{
	short val = 0;
	val = *data_p;
	val = val + (* (data_p + 1) << 8);
	data_p += 2;
	return val;
}

int GetLittleLong (void)
{
	int val = 0;
	val = *data_p;
	val = val + (* (data_p + 1) << 8);
	val = val + (* (data_p + 2) << 16);
	val = val + (* (data_p + 3) << 24);
	data_p += 4;
	return val;
}

void FindNextChunk (char *name)
{
	while (1)
	{
		data_p = last_chunk;

		if (data_p >= iff_end)
		{
			// didn't find the chunk
			data_p = NULL;
			return;
		}

		data_p += 4;
		iff_chunk_len = GetLittleLong ();

		if (iff_chunk_len < 0)
		{
			data_p = NULL;
			return;
		}

		//		if (iff_chunk_len > 1024*1024)
		//			Sys_Error ("FindNextChunk: %i length is past the 1 meg sanity limit", iff_chunk_len);
		data_p -= 8;
		last_chunk = data_p + 8 + ((iff_chunk_len + 1) & ~1);

		if (!memcmp (data_p, name, 4))
			return;
	}
}

void FindChunk (char *name)
{
	last_chunk = iff_data;
	FindNextChunk (name);
}


void DumpChunks (void)
{
	char	str[5];

	str[4] = 0;
	data_p = iff_data;

	do
	{
		memcpy (str, data_p, 4);
		data_p += 4;
		iff_chunk_len = GetLittleLong ();
		Con_Printf (PRINT_NORMAL, va ("0x%x : %s (%d)\n", (int) (data_p - 4), str, iff_chunk_len));
		data_p += (iff_chunk_len + 1) & ~1;
	} while (data_p < iff_end);
}

/*
============
GetWavinfo
============
*/
wavinfo_t GetWavinfo (char *name, byte *wav, int wavlength)
{
	wavinfo_t	info;
	int     i;
	int     format;
	int		samples;

	memset (&info, 0, sizeof (info));

	if (!wav)
		return info;

	iff_data = wav;
	iff_end = wav + wavlength;

	// find "RIFF" chunk
	FindChunk ("RIFF");

	if (!(data_p && !memcmp (data_p + 8, "WAVE", 4)))
	{
		Con_Printf (PRINT_NORMAL, "Missing RIFF/WAVE chunks\n");
		return info;
	}

	// get "fmt " chunk
	iff_data = data_p + 12;
	// DumpChunks ();

	FindChunk ("fmt ");

	if (!data_p)
	{
		Con_Printf (PRINT_NORMAL, "Missing fmt chunk\n");
		return info;
	}

	data_p += 8;
	format = GetLittleShort ();

	if (format != 1)
	{
		Con_Printf (PRINT_NORMAL, "Microsoft PCM format only\n");
		return info;
	}

	info.channels = GetLittleShort ();
	info.rate = GetLittleLong ();
	data_p += 4 + 2;
	info.width = GetLittleShort () / 8;

	// get cue chunk
	FindChunk ("cue ");

	if (data_p)
	{
		data_p += 32;
		info.loopstart = GetLittleLong ();
		//		Con_Printf("loopstart=%d\n", sfx->loopstart);

		// if the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk ("LIST");

		if (data_p)
		{
			if (!memcmp (data_p + 28, "mark", 4))
			{
				// this is not a proper parse, but it works with cooledit...
				data_p += 24;
				i = GetLittleLong ();	// samples in loop
				info.samples = info.loopstart + i;
				//				Con_Printf("looped length: %i\n", i);
			}
		}
	}
	else
		info.loopstart = -1;

	// find data chunk
	FindChunk ("data");

	if (!data_p)
	{
		Con_Printf (PRINT_NORMAL, "Missing data chunk\n");
		return info;
	}

	data_p += 4;
	samples = GetLittleLong () / info.width;

	if (info.samples)
	{
		if (samples < info.samples)
			Sys_Error (va ("Sound %s has a bad loop length", name));
	}
	else
		info.samples = samples;

	info.dataofs = data_p - wav;

	return info;
}

