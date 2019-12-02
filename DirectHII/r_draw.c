
// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "d_draw.h"
#include "d_texture.h"

cvar_t gl_texturemode = {"gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR", true};
cvar_t gl_texture_anisotropy = {"gl_texture_anisotropy", "1", true};

extern int ColorIndex[16];
extern unsigned ColorPercent[16];
extern qboolean	vid_initialized;

#define MAX_DISC 18

qpic_t *draw_disc[MAX_DISC] = {
	NULL  // make the first one null for sure
};

qpic_t		*draw_backtile;

int char_texture;
int char_smalltexture;
int char_menufonttexture;
int crosshair_texture;

qpic_t	*conback;


//=============================================================================
/* Support Routines */

typedef struct cachepic_s {
	char		name[MAX_QPATH];
	qpic_t		*pic;
} cachepic_t;

#define MAX_CACHED_PICS 	256

cachepic_t	menu_cachepics[MAX_CACHED_PICS];
int			menu_numcachepics;


byte *menuplyr_pixels[NUM_CLASSES];
int	translate_texture[NUM_CLASSES];

void Draw_SetupTranslateTexture (qpic_t *pic, int classnum)
{
	// create a texture for it
	translate_texture[classnum] = D_GetTexture (va ("translate_%i", classnum), pic->width, pic->height, 1, 0, TEX_RGBA8);

	// and save off the texels for translation
	menuplyr_pixels[classnum] = (byte *) Hunk_Alloc (GAME_HUNK, pic->width * pic->height);
	memcpy (menuplyr_pixels[classnum], pic->data, pic->width * pic->height);
}


/*
================
Draw_LoadPicTexture
================
*/
void Draw_LoadPicTexture (qpic_t *pic, glpic_t *gl)
{
	D_DrawFlush ();

	// load it first otherwise setting the glpic_t params will trash the data
	gl->texnum = D_LoadTexture ("", pic->width, pic->height, pic->width, pic->data, d_8to24table_rgba, TEX_ALPHA);

	// and now it's safe to set these
	gl->sl = 0;
	gl->sh = 1;
	gl->tl = 0;
	gl->th = 1;
}


qboolean Draw_PicFromScrap (glpic_t *gl, int width, int height, byte *data)
{
	if (width < SCRAP_SIZE && height < SCRAP_SIZE && width * height < 4096)
	{
		int scrap_x, scrap_y;

		// gl->texture cannot be assigned until the load completes owing to qpic_t/glpic_t overlap, so assign it to temporary first
		int texnum;

		if ((texnum = D_LoadScrapTexture (width, height, data, d_8to24table_rgba, &scrap_x, &scrap_y)) != 0)
		{
			D_DrawFlush ();

			gl->texnum = texnum;

			gl->sl = (float) scrap_x / (float) SCRAP_SIZE;
			gl->sh = (float) (scrap_x + width) / (float) SCRAP_SIZE;
			gl->tl = (float) scrap_y / (float) SCRAP_SIZE;
			gl->th = (float) (scrap_y + height) / (float) SCRAP_SIZE;

			// this texture is now in the scrap
			return true;
		}
	}

	// didn't load or not a scrap pic
	return false;
}


qpic_t *Draw_PicFromFile (char *name)
{
	qpic_t	*p;
	glpic_t *gl;
	qboolean scrapable = false;

	if (!(p = (qpic_t *) FS_LoadFile (GAME_HUNK, name)))
		return NULL;

	if (!strcmp (name, "gfx/menu/netp1.lmp"))
		Draw_SetupTranslateTexture (p, 0);
	else if (!strcmp (name, "gfx/menu/netp2.lmp"))
		Draw_SetupTranslateTexture (p, 1);
	else if (!strcmp (name, "gfx/menu/netp3.lmp"))
		Draw_SetupTranslateTexture (p, 2);
	else if (!strcmp (name, "gfx/menu/netp4.lmp"))
		Draw_SetupTranslateTexture (p, 3);
	else scrapable = true;

	gl = (glpic_t *) p->data;

	// load little ones into the scrap
	if (scrapable && Draw_PicFromScrap (gl, p->width, p->height, p->data))
		return p;

	// not a scrap texture or failed to load into the scrap
	Draw_LoadPicTexture (p, gl);

	return p;
}


qpic_t *Draw_PicFromWad (char *name)
{
	qpic_t	*p;
	glpic_t *gl;

	p = W_GetLumpName (name);
	gl = (glpic_t *) p->data;

	// load little ones into the scrap
	if (Draw_PicFromScrap (gl, p->width, p->height, p->data))
		return p;

	// not a scrap texture or failed to load into the scrap
	Draw_LoadPicTexture (p, gl);

	return p;
}


/*
================
Draw_CachePic
================
*/
qpic_t *Draw_CachePic (char *path)
{
	cachepic_t	*pic;
	int			i;

	for (pic = menu_cachepics, i = 0; i < menu_numcachepics; pic++, i++)
		if (!strcmp (path, pic->name))
			return pic->pic;

	if (menu_numcachepics == MAX_CACHED_PICS)
		Sys_Error ("menu_numcachepics == MAX_CACHED_PICS");

	menu_numcachepics++;
	strcpy (pic->name, path);

	// flush anything pending because we're going to update a texture here
	D_DrawFlush ();

	return (pic->pic = Draw_PicFromFile (path));
}


/*
===============
R_SetTextureMode
===============
*/
void R_SetTextureMode (void)
{
	int anisotropy = 1;

	for (anisotropy = 1; anisotropy < gl_texture_anisotropy.value; anisotropy <<= 1);
	if (anisotropy > 16) anisotropy = 16;

	D_SetTextureMode (gl_texturemode.string, anisotropy);
}


void Draw_LoadCrosshair (void)
{
	byte chbase[] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0x1f, 0x1f, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x1f, 0x1f, 0xff, 0xff,
		0xff, 0xff, 0x1f, 0x1f, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x1f, 0x1f, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	};

	crosshair_texture = D_LoadTexture ("crosshair", 16, 16, 16, chbase, d_8to24table_rgba, TEX_ALPHA);
}


/*
===============
Draw_Init
===============
*/
void Draw_Init (void)
{
	int	i;

	Cvar_RegisterVariable (&gl_texturemode, R_SetTextureMode);
	Cvar_RegisterVariable (&gl_texture_anisotropy, R_SetTextureMode);

	char_texture = D_LoadTexture ("charset", 256, 128, 256, FS_LoadFile (LOAD_HUNK, "gfx/menu/conchars.lmp"), d_8to24table_char, TEX_ALPHA);
	char_smalltexture = D_LoadTexture ("smallcharset", 128, 32, 128, W_GetLumpName ("tinyfont"), d_8to24table_char, TEX_ALPHA);
	char_menufonttexture = D_LoadTexture ("menufont", 160, 80, 160, ((qpic_t *) FS_LoadFile (LOAD_HUNK, "gfx/menu/bigfont2.lmp"))->data, d_8to24table_char, TEX_ALPHA);

	conback = Draw_PicFromFile ("gfx/menu/conback.lmp");

	// load the other textures we need
	Draw_LoadCrosshair ();

	// get the other pics we need
	for (i = MAX_DISC - 1; i >= 0; i--)
		draw_disc[i] = Draw_PicFromFile (va ("gfx/menu/skull%d.lmp", i));

	draw_backtile = Draw_PicFromFile ("gfx/menu/backtile.lmp");
}


void Draw_CrossHair (void)
{
	extern cvar_t crosshair;
	extern cvar_t crosshaircolor;
	extern cvar_t cl_crossx;
	extern cvar_t cl_crossy;

	if (crosshair.value)
	{
		D_DrawTexturedQuad (
			((vid.width2d >> 1) + cl_crossx.value) - 8,
			(((vid.height2d - sb_lines2d) >> 1) + cl_crossy.value) - 8,
			16,
			16,
			d_8to24table_rgba[(int) crosshaircolor.value & 255],
			0,
			1,
			0,
			1,
			crosshair_texture
			);
	}
}


/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character (int x, int y, unsigned int num)
{
	int row, col;
	float frow, fcol, xsize, ysize;

	num &= 511;

	if (num == 32) return; 	// space
	if (y <= -8) return; 	// totally off screen

	row = num >> 5;
	col = num & 31;

	xsize = 0.03125;
	ysize = 0.0625;
	fcol = col * xsize;
	frow = row * ysize;

	D_DrawTexturedQuad (
		x,
		y,
		8,
		8,
		0xffffffff,
		fcol,
		fcol + xsize,
		frow,
		frow + ysize,
		char_texture
		);
}

/*
================
Draw_String
================
*/
void Draw_String (int x, int y, char *str)
{
	while (*str)
	{
		Draw_Character (x, y, *str);
		str++;
		x += 8;
	}
}


//==========================================================================
//
// Draw_SmallCharacter
//
// Draws a small character that is clipped at the bottom edge of the
// screen.
//
//==========================================================================
void Draw_SmallCharacter (int x, int y, int num)
{
	int				row, col;
	float			frow, fcol, xsize, ysize;

	if (num < 32)
		num = 0;
	else if (num >= 'a' && num <= 'z')
		num -= 64;
	else if (num > '_')
		num = 0;
	else num -= 32;

	if (num == 0) return;
	if (y <= -8) return; 		// totally off screen
	if (y >= vid.height2d) return;	// Totally off screen

	row = num >> 4;
	col = num & 15;

	xsize = 0.0625;
	ysize = 0.25;
	fcol = col * xsize;
	frow = row * ysize;

	D_DrawTexturedQuad (
		x,
		y,
		8,
		8,
		0xffffffff,
		fcol,
		fcol + xsize,
		frow,
		frow + ysize,
		char_smalltexture
		);
}


//==========================================================================
//
// Draw_SmallString
//
//==========================================================================
void Draw_SmallString (int x, int y, char *str)
{
	while (*str)
	{
		Draw_SmallCharacter (x, y, *str);
		str++;
		x += 6;
	}
}

/*
================
Draw_DebugChar

Draws a single character directly to the upper right corner of the screen.
This is for debugging lockups by drawing different chars in different parts
of the code.
================
*/
void Draw_DebugChar (char num)
{
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, qpic_t *pic)
{
	glpic_t *gl = (glpic_t *) pic->data;
	D_DrawTexturedQuad (x, y, pic->width, pic->height, 0xffffffff, gl->sl, gl->sh, gl->tl, gl->th, gl->texnum);
}


void Draw_PicCropped (int x, int y, qpic_t *pic)
{
	int height;
	glpic_t 		*gl;
	float th, tl;

	if ((x < 0) || (x + pic->width > vid.width2d)) return;
	if (y >= (int) vid.height2d || y + pic->height < 0) return;

	gl = (glpic_t *) pic->data;

	// rjr tl/th need to be computed based upon pic->tl and pic->th
	//     cuz the piece may come from the scrap
	if (y + pic->height > vid.height2d)
	{
		height = vid.height2d - y;
		tl = 0;
		th = (height - 0.01) / pic->height;
	}
	else if (y < 0)
	{
		height = pic->height + y;
		y = -y;
		tl = (y - 0.01) / pic->height;
		th = (pic->height - 0.01) / pic->height;
		y = 0;
	}
	else
	{
		height = pic->height;
		tl = gl->tl;
		th = gl->th;//(height-0.01)/pic->height;
	}

	D_DrawTexturedQuad (x, y, pic->width, height, 0xffffffff, gl->sl, gl->sh, tl, th, gl->texnum);
}

/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x, y, pic);
}


void Draw_TransPicCropped (int x, int y, qpic_t *pic)
{
	Draw_PicCropped (x, y, pic);
}


/*
=============
Draw_TransPicTranslate

Only used for the player color selection menu
=============
*/
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, int classnum, int color)
{
	static int cached_colors[NUM_CLASSES] = {-1, -1, -1, -1};

	if (color != cached_colors[classnum])
	{
		unsigned transtable[256];

		// flush anything pending because we're going to update a texture here
		D_DrawFlush ();

		R_BuildTranslationTable (classnum, color, d_8to24table_rgba, transtable);
		D_LoadTextureData8 (translate_texture[classnum], 0, 0, 0, pic->width, pic->height, menuplyr_pixels[classnum], transtable);

		cached_colors[classnum] = color;
	}

	D_DrawTexturedQuad (
		x,
		y,
		pic->width,
		pic->height,
		0xffffffff,
		0,
		1,
		0,
		1,
		translate_texture[classnum]
		);
}


int M_DrawBigCharacter (int x, int y, int num, int numNext)
{
	int				row, col;
	float			frow, fcol, xsize, ysize;
	int				add;

	if (num == ' ') return 32;

	if (num == '/') num = 26;
	else num -= 65;

	if (num < 0 || num >= 27)  // only a-z and /
		return 0;

	if (numNext == '/') numNext = 26;
	else numNext -= 65;

	row = num / 8;
	col = num % 8;

	xsize = 0.125;
	ysize = 0.25;
	fcol = col * xsize;
	frow = row * ysize;

	D_DrawTexturedQuad (
		x,
		y,
		20,
		20,
		0xffffffff,
		fcol,
		fcol + xsize,
		frow,
		frow + ysize,
		char_menufonttexture
		);

	if (numNext < 0 || numNext >= 27) return 0;

	add = 0;

	if (num == (int) 'C' - 65 && numNext == (int) 'P' - 65)
		add = 3;

	return BigCharWidth[num][numNext] + add;
}


/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground (int lines)
{
	glpic_t *gl = (glpic_t *) conback->data;
	int alpha = (int) (((float) lines / (float) vid.height2d) * 384.0f);

	if (alpha > 255)
		D_DrawTexturedQuad (0, (lines - vid.height2d), vid.width2d, vid.height2d, 0xffffffff, 0, 1, 0, 1, gl->texnum);
	else if (alpha > 0)
		D_DrawTexturedQuad (0, (lines - vid.height2d), vid.width2d, vid.height2d, COLOR_FROM_RGBA (255, 255, 255, alpha), 0, 1, 0, 1, gl->texnum);
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h)
{
	glpic_t *gl = (glpic_t *) draw_backtile->data;

	float sl = x / 64.0f;
	float tl = y / 64.0f;
	float sh = (x + w) / 64.0f;
	float th = (y + h) / 64.0f;

	D_DrawTexturedQuad (x, y, w, h, 0xffffffff, sl, sh, tl, th, gl->texnum);
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	D_DrawColouredQuad (x, y, w, h, d_8to24table_rgba[c & 255]);
}


/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	int c;

	// size the quads for different resolutions
	int ofs = (20 * vid.height2d) / 480;

	// number of quads is dependent on screen size
	int coverage = (40 * vid.width2d * vid.height2d) / (640 * 480);

	// first pass darkens the screen
	D_DrawColouredQuad (0, 0, vid.width2d, vid.height2d, COLOR_FROM_RGBA (0, 0, 0, 64));

	// second pass lays the baseline
	D_DrawColouredQuad (0, 0, vid.width2d, vid.height2d, COLOR_FROM_RGBA (248, 220, 120, 50));

	// keep the random framerate-independent
	srand ((unsigned int) (realtime * 36.0));

	// subsequent passes blend smaller quads over
	for (c = 0; c < coverage; c++)
	{
		int bx = rand () % vid.width2d - ofs;
		int by = rand () % vid.height2d - ofs;
		int ex = bx + (rand () % (ofs * 2)) + ofs;
		int ey = by + (rand () % (ofs * 2)) + ofs;

		D_DrawColouredQuad (bx, by, (ex - bx), (ey - by), COLOR_FROM_RGBA (208, 180, 80, 10));
	}

	SB_Changed ();
}


//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void SCR_DrawDisc (qpic_t *disc);

void Draw_BeginDisc (void)
{
	int index = (int) (Sys_DoubleTime () * 10) % MAX_DISC;
	if (!draw_disc[index]) return;
	SCR_DrawDisc (draw_disc[index]);
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc (void)
{
}


