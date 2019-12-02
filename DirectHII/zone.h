

#ifndef ZONE_H_INCLUDED
#define ZONE_H_INCLUDED

// ============================================================================
// Zone Memory

void Zone_Free (void *ptr);
void *Zone_Alloc (int size);


// ============================================================================
// Hunk Memory

typedef enum _hunktype {
	GAME_HUNK,
	MAP_HUNK,
	LOAD_HUNK,
	FRAME_HUNK,
	MAX_HUNKS
} hunktype_t;

// moved from common.h because it needs to know what a hunktype_t is
byte *FS_LoadFile (hunktype_t hunk, char *path);

void *Hunk_Alloc (hunktype_t hunk, int size);		// returns 0 filled memory
int	Hunk_LowMark (hunktype_t hunk);
void Hunk_FreeToLowMark (hunktype_t hunk, int mark);
void Hunk_Advance (hunktype_t hunk, int size);


// ============================================================================
// Temp Memory

void *Temp_Alloc (int size);

#endif
