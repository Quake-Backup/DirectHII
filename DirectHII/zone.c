// Z_zone.c

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"


/*
==============================================================================

ZONE MEMORY ALLOCATION

This is just wrapped HeapAlloc/HeapFree these days; Zone_Free also calls
HeapCompact to help with possible address space fragmentation.

==============================================================================
*/

void Zone_Free (void *ptr)
{
	HeapFree (GetProcessHeap (), 0, ptr);
	HeapCompact (GetProcessHeap (), 0);
}


void *Zone_Alloc (int size)
{
	return HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, size);
}


/*
==============================================================================

TEMP MEMORY ALLOCATION

This just cycles a temp pointer allocated on the zone

==============================================================================
*/

void *Temp_Alloc (int size)
{
	static void *z_tempptr = NULL;

	if (z_tempptr)
	{
		Zone_Free (z_tempptr);
		z_tempptr = NULL;
	}

	return (z_tempptr = Zone_Alloc (size));
}


/*
==============================================================================

HUNK MEMORY ALLOCATION

The hunk is a collection of big static buffers categorized by usage:
permanent, this game, this map, loading or per-frame.

==============================================================================
*/

typedef struct hunk_s {
	byte *buffer;
	int maxsize;
	int lowmark;
	int peaksize;
} hunk_t;

#define GAMEHUNK_SIZE (8 * 1024 * 1024)	// 8mb
#define MAPHUNK_SIZE (256 * 1024 * 1024)	// 256mb // telefragged.bsp peaks at 89mb
#define LOADHUNK_SIZE (128 * 1024 * 1024)	// 128mb // telefragged.bsp in rrp overflows 64mb
#define FRAMEHUNK_SIZE (32 * 1024 * 1024)	// 32mb

hunk_t hunks[MAX_HUNKS] = {
	{NULL, GAMEHUNK_SIZE, 0, 0},	// game
	{NULL, MAPHUNK_SIZE, 0, 0},		// map
	{NULL, LOADHUNK_SIZE, 0, 0},	// load
	{NULL, FRAMEHUNK_SIZE, 0, 0}	// frame
};


void *Hunk_Alloc (hunktype_t hunk, int size)
{
	// create the hunk first time it's seen
	if (!hunks[hunk].buffer)
	{
		HANDLE hHeap = HeapCreate (0, 0, 0);
		hunks[hunk].buffer = (byte *) HeapAlloc (hHeap, HEAP_ZERO_MEMORY, hunks[hunk].maxsize);
	}

	// special-case: we sometimes allocate a 0-sized buffer when we need temp storage, we don't know how
	// big it's going to be (yet), we're going to copy it to permanent storage when done (or we're going
	// to throw it away immediately after doing something with it) and we'll have no further allocations
	// from this hunk while the temp storage is being used.
	if (!size) return &hunks[hunk].buffer[hunks[hunk].lowmark];

	// 16-align the allocation
	if (size > 0) size = (size + 15) & ~15;

	if (hunk == MAP_HUNK && !host_initialized)
	{
		// because the MAP_HUNK is reset to 0 each memory clear it's illegal to use it before the host is initialized
		Sys_Error ("Hunk_Alloc : hunk == MAP_HUNK && !host_initialized");
		return NULL;
	}
	else if (size < 0)
	{
		Sys_Error ("Hunk_Alloc : negative size");
		return NULL;
	}
	else if (hunks[hunk].lowmark + size >= hunks[hunk].maxsize)
	{
		Sys_Error ("Hunk_Alloc : overflow");
		return NULL;
	}
	else
	{
		byte *data = &hunks[hunk].buffer[hunks[hunk].lowmark];

		// allocations with size-0 are sometimes done where we don't know how much memory we'll need but we do know
		// that we won't be making future allocations until we're finished with the memory
		if (size > 0)
		{
			hunks[hunk].lowmark += size;
			memset (data, 0, size);
		}

		if (hunks[hunk].lowmark > hunks[hunk].peaksize)
			hunks[hunk].peaksize = hunks[hunk].lowmark;

		return data;
	}
}


int Hunk_LowMark (hunktype_t hunk)
{
	return hunks[hunk].lowmark;
}


void Hunk_FreeToLowMark (hunktype_t hunk, int mark)
{
	if (mark < hunks[hunk].lowmark)
	{
		memset (&hunks[hunk].buffer[mark], 0, hunks[hunk].lowmark - mark);
		hunks[hunk].lowmark = mark;
	}
}


void Hunk_Advance (hunktype_t hunk, int size)
{
	// 16-align the allocation
	if (size > 0)
	{
		size = (size + 15) & ~15;

		if (hunks[hunk].lowmark + size >= hunks[hunk].maxsize)
			Sys_Error ("Hunk_Advance : overflow");

		hunks[hunk].lowmark += size;

		if (hunks[hunk].lowmark > hunks[hunk].peaksize)
			hunks[hunk].peaksize = hunks[hunk].lowmark;
	}
}


