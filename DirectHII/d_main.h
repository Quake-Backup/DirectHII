
void D_UpdateMainConstants (QMATRIX *mvpMatrix, float *vieworigin, float *viewforward, float *viewright, float *viewup, float *polyblend, float time);
void D_UpdateEntityConstants (QMATRIX *localMatrix, float alphaVal, float absLight, float *origin);
void D_ClearRenderTargets (unsigned color, BOOL clearcolor, BOOL cleardepth, BOOL clearstencil);
void D_SyncPipeline (void);
void D_DrawPolyblend (void);

