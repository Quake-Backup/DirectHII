
#include "precompiled.h"

#include "d_local.h"
#include "d_texture.h"
#include "d_draw.h"
#include "d_image.h"


// gamma-correct to 16-bit precision, average, then mix back down to 8-bit precision so that we don't lose ultra-darks in the correction process
unsigned short image_mipgammatable[256];
byte image_mipinversegamma[65536];


byte Image_GammaVal8to8 (byte val, float gamma)
{
	float f = powf ((val + 1) / 256.0, gamma);
	float inf = f * 255 + 0.5;

	if (inf < 0) inf = 0;
	if (inf > 255) inf = 255;

	return inf;
}


unsigned short Image_GammaVal8to16 (byte val, float gamma)
{
	float f = powf ((val + 1) / 256.0, gamma);
	float inf = f * 65535 + 0.5;

	if (inf < 0) inf = 0;
	if (inf > 65535) inf = 65535;

	return inf;
}


byte Image_GammaVal16to8 (unsigned short val, float gamma)
{
	float f = powf ((val + 1) / 65536.0, gamma);
	float inf = (f * 255) + 0.5;

	if (inf < 0) inf = 0;
	if (inf > 255) inf = 255;

	return inf;
}


unsigned short Image_GammaVal16to16 (unsigned short val, float gamma)
{
	float f = powf ((val + 1) / 65536.0, gamma);
	float inf = (f * 65535) + 0.5;

	if (inf < 0) inf = 0;
	if (inf > 65535) inf = 65535;

	return inf;
}


void Image_Initialize (void)
{
	int i;

	// gamma-correct to 16-bit precision, average, then mix back down to 8-bit precision so that we don't lose ultra-darks in the correction process
	for (i = 0; i < 256; i++) image_mipgammatable[i] = Image_GammaVal8to16 (i, 2.2f);
	for (i = 0; i < 65536; i++) image_mipinversegamma[i] = Image_GammaVal16to8 (i, 1.0f / 2.2f);
}


int AverageMip (int _1, int _2, int _3, int _4)
{
	return (_1 + _2 + _3 + _4) >> 2;
}


int AverageMipGC (int _1, int _2, int _3, int _4)
{
	// http://filmicgames.com/archives/327
	// gamma-correct to 16-bit precision, average, then mix back down to 8-bit precision so that we don't lose ultra-darks in the correction process
	return image_mipinversegamma[(image_mipgammatable[_1] + image_mipgammatable[_2] + image_mipgammatable[_3] + image_mipgammatable[_4]) >> 2];
}


unsigned *Image_Resample (unsigned *in, int inwidth, int inheight)
{
	// round down to meet np2 specification
	int outwidth = (inwidth > 1) ? (inwidth >> 1) : 1;
	int outheight = (inheight > 1) ? (inheight >> 1) : 1;

	int mark, i, j;
	unsigned *out, *p1, *p2, fracstep, frac;

	// can this ever happen???
	if (outwidth == inwidth && outheight == inheight) return in;

	// allocating the out buffer before the hunk mark so that we can return it
	out = (unsigned *) Hunk_Alloc (LOAD_HUNK, outwidth * outheight * 4);

	// and now get the mark because all allocations after this will be released
	mark = Hunk_LowMark (LOAD_HUNK);

	p1 = (unsigned *) Hunk_Alloc (LOAD_HUNK, outwidth * 4);
	p2 = (unsigned *) Hunk_Alloc (LOAD_HUNK, outwidth * 4);

	fracstep = inwidth * 0x10000 / outwidth;
	frac = fracstep >> 2;

	for (i = 0; i < outwidth; i++)
	{
		p1[i] = 4 * (frac >> 16);
		frac += fracstep;
	}

	frac = 3 * (fracstep >> 2);

	for (i = 0; i < outwidth; i++)
	{
		p2[i] = 4 * (frac >> 16);
		frac += fracstep;
	}

	for (i = 0; i < outheight; i++)
	{
		unsigned *outrow = out + (i * outwidth);
		unsigned *inrow0 = in + inwidth * (int) (((i + 0.25f) * inheight) / outheight);
		unsigned *inrow1 = in + inwidth * (int) (((i + 0.75f) * inheight) / outheight);

		for (j = 0; j < outwidth; j++)
		{
			byte *pix1 = (byte *) inrow0 + p1[j];
			byte *pix2 = (byte *) inrow0 + p2[j];
			byte *pix3 = (byte *) inrow1 + p1[j];
			byte *pix4 = (byte *) inrow1 + p2[j];

			// don't gamma correct the alpha channel
			((byte *) &outrow[j])[0] = AverageMipGC (pix1[0], pix2[0], pix3[0], pix4[0]);
			((byte *) &outrow[j])[1] = AverageMipGC (pix1[1], pix2[1], pix3[1], pix4[1]);
			((byte *) &outrow[j])[2] = AverageMipGC (pix1[2], pix2[2], pix3[2], pix4[2]);
			((byte *) &outrow[j])[3] = AverageMip (pix1[3], pix2[3], pix3[3], pix4[3]);
		}
	}

	Hunk_FreeToLowMark (LOAD_HUNK, mark);

	return out;
}


unsigned *Image_Mipmap (unsigned *data, int width, int height)
{
	// because each SRD must have it's own data we can't mipmap in-place otherwise we'll corrupt the previous miplevel
	unsigned *trans = (unsigned *) Hunk_Alloc (LOAD_HUNK, (width >> 1) * (height >> 1) * 4);
	byte *in = (byte *) data;
	byte *out = (byte *) trans;
	int i, j;

	// do this after otherwise it will interfere with the allocation size above
	width <<= 2;
	height >>= 1;

	for (i = 0; i < height; i++, in += width)
	{
		for (j = 0; j < width; j += 8, out += 4, in += 8)
		{
			// don't gamma correct the alpha channel
			out[0] = AverageMipGC (in[0], in[4], in[width + 0], in[width + 4]);
			out[1] = AverageMipGC (in[1], in[5], in[width + 1], in[width + 5]);
			out[2] = AverageMipGC (in[2], in[6], in[width + 2], in[width + 6]);
			out[3] = AverageMip (in[3], in[7], in[width + 3], in[width + 7]);
		}
	}

	return trans;
}


void Image_AlphaEdgeFix (unsigned *data, int width, int height)
{
	int i, neighbour_u, neighbour_v;
	int s = width * height;
	byte *rgba = (byte *) data;

	for (i = 0; i < s; i++)
	{
		if (rgba[(i << 2) + 3] > 0)
			continue;
		else
		{
			int num_neighbours_valid = 0;
			int r = 0, g = 0, b = 0;

			int u = i % width;	// bug - was s % width
			int v = i / width;	// bug - was s / width

			for (neighbour_u = u - 1; neighbour_u <= u + 1; neighbour_u++)
			{
				for (neighbour_v = v - 1; neighbour_v <= v + 1; neighbour_v++)
				{
					if (neighbour_u == neighbour_v) continue;
					if (neighbour_u < 0 || neighbour_u >= width || neighbour_v < 0 || neighbour_v >= height) continue;	// bug - was >
					if (rgba[((neighbour_u + neighbour_v * width) << 2) + 3] == 0) continue;

					r += data[neighbour_u + neighbour_v * width] & 0xff;
					g += (data[neighbour_u + neighbour_v * width] & 0xff00) >> 8;
					b += (data[neighbour_u + neighbour_v * width] & 0xff0000) >> 16;

					num_neighbours_valid++;
				}
			}

			if (num_neighbours_valid == 0)
				continue;

			if ((r = r / num_neighbours_valid) > 255) r = 255;
			if ((g = g / num_neighbours_valid) > 255) g = 255;
			if ((b = b / num_neighbours_valid) > 255) b = 255;

			data[i] = (b << 16) | (g << 8) | r;
		}
	}
}


unsigned *Image_8to32 (byte *data, int width, int height, int stride, unsigned *palette, int flags)
{
	int i, j;
	unsigned *out = (unsigned *) Hunk_Alloc (LOAD_HUNK, width * height * 4);
	unsigned *trans = out;

	// TEX_HOLEY, etc are handled through palettes rather than branching
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
			trans[j] = palette[data[j]];

		trans += width;
		data += stride;
	}

	// and now fix alpha edges
	if (flags & TEX_ANYALPHA)
		Image_AlphaEdgeFix (out, width, height);

	return out;
}


void Image_CollapseRowPitch (unsigned *data, int width, int height, int pitch)
{
	if (width != pitch)
	{
		int h, w;
		unsigned *out = data;

		// as a minor optimization we can skip the first row
		// since out and data point to the same this is OK
		out += width;
		data += pitch;

		for (h = 1; h < height; h++)
		{
			for (w = 0; w < width; w++)
				out[w] = data[w];

			out += width;
			data += pitch;
		}
	}
}


void Image_Compress32To24 (byte *data, int width, int height)
{
	int h, w;
	byte *out = data;

	for (h = 0; h < height; h++)
	{
		for (w = 0; w < width; w++, data += 4, out += 3)
		{
			out[0] = data[0];
			out[1] = data[1];
			out[2] = data[2];
		}
	}
}


void Image_Compress32To24RGBtoBGR (byte *data, int width, int height)
{
	int h, w;
	byte *out = data;

	for (h = 0; h < height; h++)
	{
		for (w = 0; w < width; w++, data += 4, out += 3)
		{
			out[0] = data[2];
			out[1] = data[1];
			out[2] = data[0];
		}
	}
}


void Image_WriteDataToTGA (char *name, void *data, int width, int height, int bpp)
{
	if ((bpp == 24 || bpp == 8) && name && data && width > 0 && height > 0)
	{
		FILE *f = fopen (name, "wb");

		if (f)
		{
			byte header[18];

			memset (header, 0, 18);

			header[2] = 2;
			header[12] = width & 255;
			header[13] = width >> 8;
			header[14] = height & 255;
			header[15] = height >> 8;
			header[16] = bpp;
			header[17] = 0x20;

			fwrite (header, 18, 1, f);
			fwrite (data, (width * height * bpp) >> 3, 1, f);

			fclose (f);
		}
	}
}


void Image_ApplyTranslationRGB (byte *rgb, int size, byte *rtable, byte *gtable, byte *btable)
{
	int i;

	for (i = 0; i < size; i++, rgb += 3)
	{
		rgb[0] = rtable[rgb[0]];
		rgb[1] = rtable[rgb[1]];
		rgb[2] = rtable[rgb[2]];
	}
}

