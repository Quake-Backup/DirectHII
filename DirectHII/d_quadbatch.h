
void D_BeginQuadBatch (void);
void D_CheckQuadBatch (void);
void D_QuadVertexPosition3fvColorTexCoord2f (float *xyz, DWORD color, float s, float t);
void D_QuadVertexPosition2fColorTexCoord2f (float x, float y, DWORD color, float s, float t);
void D_QuadVertexPosition2fColor (float x, float y, DWORD color);
void D_QuadVertexPosition2f (float x, float y);
void D_EndQuadBatch (void);
int D_CreateShaderBundleForQuadBatch (int resourceID, const char *vsentry, const char *psentry);

