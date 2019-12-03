
extern int ColorIndex[16];
extern unsigned ColorPercent[16];

// helpers/etc
void Image_CollapseRowPitch (unsigned *data, int width, int height, int pitch);
void Image_Compress32To24 (byte *data, int width, int height);
void Image_Compress32To24RGBtoBGR (byte *data, int width, int height);
void Image_WriteDataToTGA (char *name, void *data, int width, int height, int bpp);

void Image_Initialize (void);
unsigned *Image_Resample (unsigned *in, int inwidth, int inheight);
unsigned *Image_Mipmap (unsigned *data, int width, int height);
void Image_AlphaEdgeFix (unsigned *data, int width, int height);
unsigned *Image_8to32 (byte *data, int width, int height, int stride, unsigned *palette, int flags);

// gamma
byte Image_GammaVal8to8 (byte val, float gamma);
unsigned short Image_GammaVal8to16 (byte val, float gamma);
byte Image_GammaVal16to8 (unsigned short val, float gamma);
unsigned short Image_GammaVal16to16 (unsigned short val, float gamma);
void Image_ApplyTranslationRGB (byte *rgb, int size, byte *rtable, byte *gtable, byte *btable);

