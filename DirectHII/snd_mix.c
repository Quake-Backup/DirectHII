// snd_mix.c -- portable code to mix sounds for snd_dma.c

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#define	PAINTBUFFER_SIZE	2048
portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];
int		snd_scaletable[32][256];
int 	*snd_p, snd_linear_count, snd_vol;
short	*snd_out;


__declspec (naked) void Snd_WriteLinearBlastStereo16 (void)
{
	__asm
	{
		push edi
			push ebx
			mov ecx, ds:dword ptr[snd_linear_count]
			mov ebx, ds : dword ptr[snd_p]
			mov edi, ds : dword ptr[snd_out]
LWLBLoopTop :
			mov eax, ds : dword ptr[-8 + ebx + ecx * 4]
			sar eax, 8
			cmp eax, 07FFFh
			jg LClampHigh
			cmp eax, 0FFFF8000h
			jnl LClampDone
			mov eax, 0FFFF8000h
			jmp LClampDone
LClampHigh :
		mov eax, 07FFFh
LClampDone :
		   mov edx, ds : dword ptr[-4 + ebx + ecx * 4]
		   sar edx, 8
		   cmp edx, 07FFFh
		   jg LClampHigh2
		   cmp edx, 0FFFF8000h
		   jnl LClampDone2
		   mov edx, 0FFFF8000h
		   jmp LClampDone2
LClampHigh2 :
		mov edx, 07FFFh
LClampDone2 :
			shl edx, 16
			and eax, 0FFFFh
			or edx, eax
			mov ds : dword ptr[-4 + edi + ecx * 2], edx
			sub ecx, 2
			jnz LWLBLoopTop
			pop ebx
			pop edi
			ret
	}
}


void S_TransferStereo16 (unsigned long *pbuf, int endtime)
{
	int		lpos;
	int		lpaintedtime;

	snd_p = (int *) paintbuffer;
	lpaintedtime = paintedtime;

	while (lpaintedtime < endtime)
	{
		// handle recirculating buffer issues
		lpos = lpaintedtime & ((dma.samples >> 1) - 1);

		snd_out = (short *) pbuf + (lpos << 1);

		snd_linear_count = (dma.samples >> 1) - lpos;

		if (lpaintedtime + snd_linear_count > endtime)
			snd_linear_count = endtime - lpaintedtime;

		snd_linear_count <<= 1;

		// write a linear blast of samples
		Snd_WriteLinearBlastStereo16 ();

		snd_p += snd_linear_count;
		lpaintedtime += (snd_linear_count >> 1);
	}
}


void S_TransferPaintBuffer (int endtime)
{
	int 	out_idx;
	int 	count;
	int 	out_mask;
	int 	*p;
	int 	step;
	int		val;
	unsigned long *pbuf;

	pbuf = (unsigned long *) dma.buffer;

	if (dma.samplebits == 16 && dma.channels == 2)
	{
		// optimized case
		S_TransferStereo16 (pbuf, endtime);
	}
	else
	{
		// general case
		p = (int *) paintbuffer;
		count = (endtime - paintedtime) * dma.channels;
		out_mask = dma.samples - 1;
		out_idx = paintedtime * dma.channels & out_mask;
		step = 3 - dma.channels;

		if (dma.samplebits == 16)
		{
			short *out = (short *) pbuf;

			while (count--)
			{
				val = *p >> 8;
				p += step;

#pragma warning(disable : 4310) // cast truncates constant value
				if (val > 0x7fff)
					val = 0x7fff;
				else if (val < (short) 0x8000)
					val = (short) 0x8000;
#pragma warning(default : 4310)

				out[out_idx] = val;
				out_idx = (out_idx + 1) & out_mask;
			}
		}
		else if (dma.samplebits == 8)
		{
			unsigned char *out = (unsigned char *) pbuf;

			while (count--)
			{
				val = *p >> 8;
				p += step;

#pragma warning(disable : 4310) // cast truncates constant value
				if (val > 0x7fff)
					val = 0x7fff;
				else if (val < (short) 0x8000)
					val = (short) 0x8000;
#pragma warning(default : 4310)

				out[out_idx] = (val >> 8) + 128;
				out_idx = (out_idx + 1) & out_mask;
			}
		}
	}
}


/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/
__declspec (naked) void SND_PaintChannelFrom8 (channel_t *ch, sfxcache_t *sc, int count, int offset)
{
	// this is done as asm so that it (1) avoids the 8-bit mixing bug in the C code, and (2) gets the perf back
	__asm
	{
		push esi
			push edi
			push ebx
			push ebp
			mov ebx, ds:dword ptr[4 + 16 + esp]
			mov esi, ds : dword ptr[8 + 16 + esp]
			mov eax, ds : dword ptr[4 + ebx]
			mov edx, ds : dword ptr[8 + ebx]
			cmp eax, 255
			jna LLeftSet
			mov eax, 255
LLeftSet :
		 cmp edx, 255
		 jna LRightSet
		 mov edx, 255
LRightSet :
		  and eax, 0F8h
		  add esi, 20
		  and edx, 0F8h
		  mov edi, ds : dword ptr[16 + ebx]
		  mov ecx, ds : dword ptr[12 + 16 + esp]
		  add esi, edi
		  shl eax, 7
		  add edi, ecx
		  shl edx, 7
		  mov ds : dword ptr[16 + ebx], edi
		  add eax, offset snd_scaletable
		  add edx, offset snd_scaletable
		  sub ebx, ebx
		  mov bl, ds : byte ptr[-1 + esi + ecx * 1]
		  test ecx, 1
		  jz LMix8Loop
		  mov edi, ds : dword ptr[eax + ebx * 4]
		  mov ebp, ds : dword ptr[edx + ebx * 4]
		  add edi, ds : dword ptr[paintbuffer + 0 - 8 + ecx * 8]
		  add ebp, ds : dword ptr[paintbuffer + 4 - 8 + ecx * 8]
		  mov ds : dword ptr[paintbuffer + 0 - 8 + ecx * 8], edi
		  mov ds : dword ptr[paintbuffer + 4 - 8 + ecx * 8], ebp
		  mov bl, ds : byte ptr[-2 + esi + ecx * 1]
		  dec ecx
		  jz LDone
LMix8Loop :
		mov edi, ds : dword ptr[eax + ebx * 4]
			mov ebp, ds : dword ptr[edx + ebx * 4]
			add edi, ds : dword ptr[paintbuffer + 0 - 8 + ecx * 8]
			add ebp, ds : dword ptr[paintbuffer + 4 - 8 + ecx * 8]
			mov bl, ds : byte ptr[-2 + esi + ecx * 1]
			mov ds : dword ptr[paintbuffer + 0 - 8 + ecx * 8], edi
			mov ds : dword ptr[paintbuffer + 4 - 8 + ecx * 8], ebp
			mov edi, ds : dword ptr[eax + ebx * 4]
			mov ebp, ds : dword ptr[edx + ebx * 4]
			mov bl, ds : byte ptr[-3 + esi + ecx * 1]
			add edi, ds : dword ptr[paintbuffer + 0 - 8 * 2 + ecx * 8]
			add ebp, ds : dword ptr[paintbuffer + 4 - 8 * 2 + ecx * 8]
			mov ds : dword ptr[paintbuffer + 0 - 8 * 2 + ecx * 8], edi
			mov ds : dword ptr[paintbuffer + 4 - 8 * 2 + ecx * 8], ebp
			sub ecx, 2
			jnz LMix8Loop
LDone :
		pop ebp
			pop ebx
			pop edi
			pop esi
			ret
	}
}


void SND_PaintChannelFrom16 (channel_t *ch, sfxcache_t *sc, int count, int offset)
{
	int data;
	int left, right;
	int leftvol, rightvol;
	signed short *sfx;
	int	i;
	portable_samplepair_t	*samp;

	leftvol = ch->leftvol * snd_vol;
	rightvol = ch->rightvol * snd_vol;
	sfx = (signed short *) sc->data + ch->pos;

	samp = &paintbuffer[offset];

	for (i = 0; i < count; i++, samp++)
	{
		data = sfx[i];
		left = (data * leftvol) >> 8;
		right = (data * rightvol) >> 8;
		samp->left += left;
		samp->right += right;
	}

	ch->pos += count;
}


void S_PaintChannels (int endtime)
{
	int 	i;
	int 	end;
	channel_t *ch;
	sfxcache_t	*sc;
	int		ltime, count;

	snd_vol = volume.value * 256;

	while (paintedtime < endtime)
	{
		// if paintbuffer is smaller than DMA buffer
		end = endtime;

		if (endtime - paintedtime > PAINTBUFFER_SIZE)
			end = paintedtime + PAINTBUFFER_SIZE;

		// clear the paint buffer
		memset (paintbuffer, 0, (end - paintedtime) * sizeof (portable_samplepair_t));

		// paint in the channels.
		ch = channels;

		for (i = 0; i < total_channels; i++, ch++)
		{
			ltime = paintedtime;

			while (ltime < end)
			{
				if (!ch->sfx || (!ch->leftvol && !ch->rightvol))
					break;

				// max painting is to the end of the buffer
				count = end - ltime;

				// might be stopped by running out of data
				if (ch->end - ltime < count)
					count = ch->end - ltime;

				sc = S_LoadSound (ch->sfx);

				if (!sc)
					break;

				if (count > 0 && ch->sfx)
				{
					if (sc->width == 1) // FIXME; 8 bit asm is wrong now
						SND_PaintChannelFrom8 (ch, sc, count, ltime - paintedtime);
					else SND_PaintChannelFrom16 (ch, sc, count, ltime - paintedtime);

					ltime += count;
				}

				// if at end of loop, restart
				if (ltime >= ch->end)
				{
					if (sc->loopstart >= 0)
					{
						ch->pos = sc->loopstart;
						ch->end = ltime + sc->length - ch->pos;
					}
					else
					{
						// channel just stopped
						ch->sfx = NULL;
					}
				}
			}
		}

		// transfer out according to DMA format
		S_TransferPaintBuffer (end);
		paintedtime = end;
	}
}


void SND_InitScaletable (void)
{
	int		i, j;
	int		scale;

	for (i = 0; i < 32; i++)
	{
		scale = i * 8 * 256 * volume.value;

		for (j = 0; j < 256; j++)
			snd_scaletable[i][j] = ((signed char) j) * scale;
	}
}


