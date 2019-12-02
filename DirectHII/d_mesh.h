
// alias models

// this needs to be available to both R_ and D_ code
typedef struct meshconstants_s {
	float scale[4];			// padded for cbuffer alignment
	float scale_origin[3];
	float lerpblend;
	float shadelight[4];	// padded for cbuffer alignment
	float shadevector[4];	// padded for cbuffer alignment
} meshconstants_t;

typedef struct aliaspolyvert_s {
	// xyz is position
	// w is lightnormalindex
	byte position[4];
} aliaspolyvert_t;

typedef struct aliastexcoord_s {
	float texcoord[2];
} aliastexcoord_t;

typedef struct aliasmesh_s {
	int st[2];
	unsigned short vertindex;
} aliasmesh_t;


int D_FindAliasBufferSet (char *name);
int D_GetAliasBufferSet (char *name);
void D_MakeAliasPolyverts (int modelnum, int numposes, int nummesh, aliaspolyvert_t *polyverts);
void D_MakeAliasTexCoords (int modelnum, int numtexcoords, aliastexcoord_t *texcoords);
void D_MakeAliasIndexes (int modelnum, int numindexes, unsigned short *indexes);
void D_SetAliasSkin (int texnum);
void D_SetAliasPoses (int modelnum, int posenum0, int posenum1);
void D_SetupAliasModel (QMATRIX *localMatrix, float alphaVal, float *origin, qboolean alphaEnable, qboolean alphaReverse);
void D_DrawAliasModel (int modelnum);
void D_DrawAliasPolyset (int modelnum);
void D_SetAliasCBuffer (meshconstants_t *consts);
void D_SetAliasDynamicState (void);

void D_FreeUnusedAliasBuffers (void);


