// gl_mesh.c: triangle model functions

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "d_mesh.h"
#include "d_light.h"
#include "d_main.h"

/*
================
R_MakeAliasModelDisplayLists
================
*/
void R_MakeAliasPolyverts (model_t *m, aliashdr_t *hdr, trivertx_t *xyzverts[MAXALIASFRAMES], aliasmesh_t *mesh, int nummesh)
{
	int i, j;
	int mark = Hunk_LowMark (LOAD_HUNK);
	aliaspolyvert_t *polyverts = (aliaspolyvert_t *) Hunk_Alloc (LOAD_HUNK, sizeof (aliaspolyvert_t) * nummesh * hdr->numposes);

	// copy out polyverts
	for (i = 0; i < hdr->numposes; i++)
	{
		trivertx_t *src = xyzverts[i];
		aliaspolyvert_t *dst = polyverts + (i * nummesh);

		for (j = 0; j < nummesh; j++, dst++)
		{
			trivertx_t *tv = &src[mesh[j].vertindex];

			dst->position[0] = tv->v[0];
			dst->position[1] = tv->v[1];
			dst->position[2] = tv->v[2];
			dst->position[3] = tv->lightnormalindex;
		}
	}

	D_MakeAliasPolyverts (m->modelnum, hdr->numposes, nummesh, polyverts);
	Hunk_FreeToLowMark (LOAD_HUNK, mark);
}


void R_MakeAliasTexCoords (model_t *m, aliashdr_t *hdr, aliasmesh_t *mesh, int nummesh)
{
	int i;
	int mark = Hunk_LowMark (LOAD_HUNK);
	aliastexcoord_t *texcoords = (aliastexcoord_t *) Hunk_Alloc (LOAD_HUNK, sizeof (aliastexcoord_t) * nummesh);

	// copy out texcoords
	for (i = 0; i < nummesh; i++)
	{
		texcoords[i].texcoord[0] = ((float) mesh[i].st[0] + 0.5f) / hdr->skinwidth;
		texcoords[i].texcoord[1] = ((float) mesh[i].st[1] + 0.5f) / hdr->skinheight;
	}

	D_MakeAliasTexCoords (m->modelnum, nummesh, texcoords);
	Hunk_FreeToLowMark (LOAD_HUNK, mark);
}


void R_MakeAliasIndexes (model_t *m, unsigned short *indexes, int numindexes, int nummesh)
{
	int mark = Hunk_LowMark (LOAD_HUNK);
	unsigned short *optimized = (unsigned short *) Hunk_Alloc (LOAD_HUNK, sizeof (unsigned short) * numindexes);

	// optimize index order for vertex cache
	VCache_ReorderIndices (optimized, indexes, numindexes / 3, nummesh);
	D_MakeAliasIndexes (m->modelnum, numindexes, optimized);

	Hunk_FreeToLowMark (LOAD_HUNK, mark);
}


void R_MakeAliasModelDisplayListsRAPO (model_t *m, aliashdr_t *hdr, trivertx_t *xyzverts[MAXALIASFRAMES], stvert_t *stverts, dnewtriangle_t *triangles)
{
	// see has it already been allocated
	if ((m->modelnum = D_FindAliasBufferSet (m->name)) == -1)
	{
		int i, j;
		int mark = Hunk_LowMark (LOAD_HUNK);

		// there will always be this number of indexes
		unsigned short *indexes = (unsigned short *) Hunk_Alloc (LOAD_HUNK, sizeof (unsigned short) * hdr->numtris * 3);
		int numindexes = 0;

		// there can never be more than this number of verts
		// oh joy!  some h2 models actually expand to *more* verts when you run them through this, so this just assumes the worst case which is that every vert is unique
		aliasmesh_t *mesh = (aliasmesh_t *) Hunk_Alloc (LOAD_HUNK, sizeof (aliasmesh_t) * hdr->numtris * 3);
		int nummesh = 0;

		for (i = 0; i < hdr->numtris; i++)
		{
			for (j = 0; j < 3; j++)
			{
				int v;

				// index into vertexes
				unsigned short vertindex = triangles[i].vertindex[j];
				unsigned short stindex = triangles[i].stindex[j];

				// basic s/t coords
				int s = stverts[stindex].s;
				int t = stverts[stindex].t;

				// check for back side and adjust texcoord s
				if (!triangles[i].facesfront && stverts[stindex].onseam) s += hdr->skinwidth / 2;

				// see does this vert already exist
				for (v = 0; v < nummesh; v++)
				{
					// it could use the same xyz but have different s and t
					if (mesh[v].vertindex == vertindex && mesh[v].st[0] == s && mesh[v].st[1] == t)
					{
						// exists; emit an index for it
						indexes[numindexes++] = v;

						// no need to check any more
						break;
					}
				}

				if (v == nummesh)
				{
					// doesn't exist; emit a new vert and index
					indexes[numindexes++] = nummesh;

					mesh[nummesh].vertindex = vertindex;
					mesh[nummesh].st[0] = s;
					mesh[nummesh].st[1] = t;

					nummesh++;
				}
			}
		}

		// and convert it to vertex buffers
		m->modelnum = D_GetAliasBufferSet (m->name);

		R_MakeAliasPolyverts (m, hdr, xyzverts, mesh, nummesh);
		R_MakeAliasTexCoords (m, hdr, mesh, nummesh);
		R_MakeAliasIndexes (m, indexes, numindexes, nummesh);

		Hunk_FreeToLowMark (LOAD_HUNK, mark);
	}
}


void R_MakeAliasModelDisplayListsIDPO (model_t *m, aliashdr_t *hdr, trivertx_t *xyzverts[MAXALIASFRAMES], stvert_t *stverts, dtriangle_t *triangles)
{
	// see has it already been allocated
	if ((m->modelnum = D_FindAliasBufferSet (m->name)) == -1)
	{
		int i, j;
		int mark = Hunk_LowMark (LOAD_HUNK);

		// there will always be this number of indexes
		unsigned short *indexes = (unsigned short *) Hunk_Alloc (LOAD_HUNK, sizeof (unsigned short) * hdr->numtris * 3);
		int numindexes = 0;

		// there can never be more than this number of verts
		// oh joy!  some h2 models actually expand to *more* verts when you run them through this, so this just assumes the worst case which is that every vert is unique
		aliasmesh_t *mesh = (aliasmesh_t *) Hunk_Alloc (LOAD_HUNK, sizeof (aliasmesh_t) * hdr->numtris * 3);
		int nummesh = 0;

		for (i = 0; i < hdr->numtris; i++)
		{
			for (j = 0; j < 3; j++)
			{
				int v;

				// index into vertexes
				unsigned short vertindex = triangles[i].vertindex[j];
				unsigned short stindex = triangles[i].vertindex[j];

				// basic s/t coords
				int s = stverts[stindex].s;
				int t = stverts[stindex].t;

				// check for back side and adjust texcoord s
				if (!triangles[i].facesfront && stverts[stindex].onseam) s += hdr->skinwidth / 2;

				// see does this vert already exist
				for (v = 0; v < nummesh; v++)
				{
					// it could use the same xyz but have different s and t
					if (mesh[v].vertindex == vertindex && mesh[v].st[0] == s && mesh[v].st[1] == t)
					{
						// exists; emit an index for it
						indexes[numindexes++] = v;

						// no need to check any more
						break;
					}
				}

				if (v == nummesh)
				{
					// doesn't exist; emit a new vert and index
					indexes[numindexes++] = nummesh;

					mesh[nummesh].vertindex = vertindex;
					mesh[nummesh].st[0] = s;
					mesh[nummesh].st[1] = t;

					nummesh++;
				}
			}
		}

		// and convert it to vertex buffers
		m->modelnum = D_GetAliasBufferSet (m->name);

		R_MakeAliasPolyverts (m, hdr, xyzverts, mesh, nummesh);
		R_MakeAliasTexCoords (m, hdr, mesh, nummesh);
		R_MakeAliasIndexes (m, indexes, numindexes, nummesh);

		Hunk_FreeToLowMark (LOAD_HUNK, mark);
	}
}


float R_CalcLerpBlend (entity_t *e, double lerpstart, float lerp_interval)
{
	float blend = (cl.time - lerpstart) / lerp_interval;

	if (blend < 0.0f) return 0.0f;
	if (blend > 1.0f) return 1.0f;

	return blend;
}


void R_SetupAliasFrame (entity_t *e, aliashdr_t *hdr, meshconstants_t *consts)
{
	int		pose, numposes;
	int		frame;
	float	frame_interval;

	frame = e->frame;

	if ((frame >= hdr->numframes) || (frame < 0))
	{
		Con_Printf (PRINT_DEVELOPER, va ("R_AliasSetupFrame: no such frame %d\n", frame));
		frame = 0;
	}

	pose = hdr->frames[frame].firstpose;
	numposes = hdr->frames[frame].numposes;

	if (numposes > 1)
	{
		frame_interval = hdr->frames[frame].interval;
		pose += (int) (cl.time / frame_interval) % numposes;
	}
	else
	{
		if (e->lerpflags & LERP_FINISH)
			frame_interval = e->lerpfinish;
		else frame_interval = HX_FRAME_TIME;
	}

	if (e->lerpflags & LERP_RESETANIM)
	{
		// kill any lerp in progress
		e->lerpstart = 0;
		e->prevpose = pose;
		e->currpose = pose;
		e->lerpflags &= ~LERP_RESETANIM;
	}
	else if (e->currpose != pose)
	{
		// pose changed, start new lerp
		if (e->lerpflags & LERP_RESETANIM2)
		{
			// defer lerping one more time
			e->lerpstart = 0;
			e->prevpose = pose;
			e->currpose = pose;
			e->lerpflags &= ~LERP_RESETANIM2;
		}
		else
		{
			e->lerpstart = cl.time;
			e->prevpose = e->currpose;
			e->currpose = pose;
		}
	}

	D_SetAliasPoses (e->model->modelnum, e->prevpose, e->currpose);
	consts->lerpblend = R_CalcLerpBlend (e, e->lerpstart, frame_interval);
}


void R_TransformAliasModel (entity_t *e, aliashdr_t *hdr, QMATRIX *localMatrix, float *lerporigin, float *lerpangles, meshconstants_t *consts)
{
	float	forward;
	float	yaw, pitch;
	vec3_t	angles;

	R_MatrixIdentity (localMatrix);
	R_MatrixTranslate (localMatrix, lerporigin[0], lerporigin[1], lerporigin[2]);

	if (e->model->flags & EF_FACE_VIEW)
	{
		VectorSubtract (lerporigin, r_origin, angles);
		VectorSubtract (r_origin, lerporigin, angles);
		Vector3Normalize (angles);

		if (angles[1] == 0 && angles[0] == 0)
		{
			yaw = 0;

			if (angles[2] > 0)
				pitch = 90;
			else pitch = 270;
		}
		else
		{
			yaw = (int) (atan2 (angles[1], angles[0]) * 180 / M_PI);

			forward = sqrt (angles[0] * angles[0] + angles[1] * angles[1]);
			pitch = (int) (atan2 (angles[2], forward) * 180 / M_PI);

			if (pitch < 0) pitch += 360;
			if (yaw < 0) yaw += 360;
		}

		// fixme - should pitch and yaw be swapped???
		R_MatrixRotate (localMatrix, -pitch, yaw, -lerpangles[2]);
	}
	else
	{
		if (e->model->flags & EF_ROTATE)
			R_MatrixRotate (localMatrix, -lerpangles[0], anglemod ((lerporigin[0] + lerporigin[1]) * 0.8 + (108 * cl.time)), -lerpangles[2]);
		else R_MatrixRotate (localMatrix, -lerpangles[0], lerpangles[1], -lerpangles[2]);
	}

	if (e->scale != 0 && e->scale != 100)
	{
		float entScale = (float) e->scale / 100.0;
		float xyfact, zfact;

		switch (e->drawflags & SCALE_TYPE_MASKIN)
		{
		case SCALE_TYPE_UNIFORM:
			consts->scale[0] = hdr->scale[0] * entScale;
			consts->scale[1] = hdr->scale[1] * entScale;
			consts->scale[2] = hdr->scale[2] * entScale;
			xyfact = zfact = (entScale - 1.0) * 127.95;
			break;

		case SCALE_TYPE_XYONLY:
			consts->scale[0] = hdr->scale[0] * entScale;
			consts->scale[1] = hdr->scale[1] * entScale;
			consts->scale[2] = hdr->scale[2];
			xyfact = (entScale - 1.0) * 127.95;
			zfact = 1.0;
			break;

		case SCALE_TYPE_ZONLY:
			consts->scale[0] = hdr->scale[0];
			consts->scale[1] = hdr->scale[1];
			consts->scale[2] = hdr->scale[2] * entScale;
			xyfact = 1.0;
			zfact = (entScale - 1.0) * 127.95;
			break;

		default:
			consts->scale[0] = 1.0f;
			consts->scale[1] = 1.0f;
			consts->scale[2] = 1.0f;
			xyfact = 1.0f;
			zfact = 1.0f;
			break;
		}

		switch (e->drawflags & SCALE_ORIGIN_MASKIN)
		{
		case SCALE_ORIGIN_CENTER:
			consts->scale_origin[0] = hdr->scale_origin[0] - hdr->scale[0] * xyfact;
			consts->scale_origin[1] = hdr->scale_origin[1] - hdr->scale[1] * xyfact;
			consts->scale_origin[2] = hdr->scale_origin[2] - hdr->scale[2] * zfact;
			break;

		case SCALE_ORIGIN_BOTTOM:
			consts->scale_origin[0] = hdr->scale_origin[0] - hdr->scale[0] * xyfact;
			consts->scale_origin[1] = hdr->scale_origin[1] - hdr->scale[1] * xyfact;
			consts->scale_origin[2] = hdr->scale_origin[2];
			break;

		case SCALE_ORIGIN_TOP:
			consts->scale_origin[0] = hdr->scale_origin[0] - hdr->scale[0] * xyfact;
			consts->scale_origin[1] = hdr->scale_origin[1] - hdr->scale[1] * xyfact;
			consts->scale_origin[2] = hdr->scale_origin[2] - hdr->scale[2] * zfact * 2.0;
			break;

		default:
			consts->scale_origin[0] = 0.0f;
			consts->scale_origin[1] = 0.0f;
			consts->scale_origin[2] = 0.0f;
			break;
		}
	}
	else
	{
		Vector3Copy (consts->scale, hdr->scale);
		Vector3Copy (consts->scale_origin, hdr->scale_origin);
	}

	// Floating motion
	if (e->model->flags & EF_ROTATE)
		consts->scale_origin[2] += sin (lerporigin[0] + lerporigin[1] + (cl.time * 3)) * 5.5;
}


void R_SetupEntityTransform (entity_t *e, float *lerporigin, float *lerpangles)
{
	float blend;

	// if LERP_RESETMOVE, kill any lerps in progress
	if (e->lerpflags & LERP_RESETMOVE)
	{
		e->originlerpstart = e->angleslerpstart = 0;
		Vector3Copy (e->prevorigin, e->origin);
		Vector3Copy (e->currorigin, e->origin);
		Vector3Copy (e->prevangles, e->angles);
		Vector3Copy (e->currangles, e->angles);
		e->lerpflags &= ~LERP_RESETMOVE;
	}
	else
	{
		if (!Vector3Compare (e->origin, e->currorigin))
		{
			e->originlerpstart = cl.time;
			Vector3Copy (e->prevorigin, e->currorigin);
			Vector3Copy (e->currorigin, e->origin);
		}

		if (!Vector3Compare (e->angles, e->currangles))
		{
			e->angleslerpstart = cl.time;
			Vector3Copy (e->prevangles, e->currangles);
			Vector3Copy (e->currangles, e->angles);
		}
	}

	// set up values
	if (e != &cl.viewent && (e->lerpflags & LERP_MOVESTEP))
	{
		// translation
		blend = R_CalcLerpBlend (e, e->originlerpstart, (e->lerpflags & LERP_FINISH) ? e->lerpfinish : HX_FRAME_TIME);
		lerporigin[0] = e->prevorigin[0] + (e->currorigin[0] - e->prevorigin[0]) * blend;
		lerporigin[1] = e->prevorigin[1] + (e->currorigin[1] - e->prevorigin[1]) * blend;
		lerporigin[2] = e->prevorigin[2] + (e->currorigin[2] - e->prevorigin[2]) * blend;

		// rotation
		blend = R_CalcLerpBlend (e, e->angleslerpstart, (e->lerpflags & LERP_FINISH) ? e->lerpfinish : HX_FRAME_TIME);
		lerpangles[0] = e->prevangles[0] + RAD2DEG (asin (sin (DEG2RAD (e->currangles[0] - e->prevangles[0])) * blend));
		lerpangles[1] = e->prevangles[1] + RAD2DEG (asin (sin (DEG2RAD (e->currangles[1] - e->prevangles[1])) * blend));
		lerpangles[2] = e->prevangles[2] + RAD2DEG (asin (sin (DEG2RAD (e->currangles[2] - e->prevangles[2])) * blend));
	}
	else
	{
		// don't lerp
		Vector3Copy (lerporigin, e->origin);
		Vector3Copy (lerpangles, e->angles);
	}
}


void R_SetAliasSkin (entity_t *e, aliashdr_t *hdr)
{
	char temp[40];
	qpic_t *stonepic;
	glpic_t *gl;
	extern model_t *player_models[NUM_CLASSES];
	int texnum = 0;

	if (e->skinnum >= 100)
	{
		if (e->skinnum > 255)
		{
			Sys_Error ("skinnum > 255");
		}

		if (!gl_extra_textures[e->skinnum - 100])  // Need to load it in
		{
			sprintf (temp, "gfx/skin%d.lmp", e->skinnum);
			stonepic = Draw_CachePic (temp);
			gl = (glpic_t *) stonepic->data;
			gl_extra_textures[e->skinnum - 100] = gl->texnum;
		}

		texnum = gl_extra_textures[e->skinnum - 100];
	}
	else
	{
		texnum = hdr->texnum[e->skinnum];

		// we can't dynamically colormap textures, so they are cached
		// seperately for the players.  Heads are just uncolored.
		if (e->model == player_models[0] || e->model == player_models[1] || e->model == player_models[2] || e->model == player_models[3])
		{
			int playernum = (e - cl_entities) - 1;
			texnum = R_TranslatePlayerSkin (playernum);
		}
	}

	D_SetAliasSkin (texnum);
}


void R_MinimumLight (float *light, float minlight)
{
	light[0] = ((light[0] + minlight) * 256.0f) / (256.0f + minlight);
	light[1] = ((light[1] + minlight) * 256.0f) / (256.0f + minlight);
	light[2] = ((light[2] + minlight) * 256.0f) / (256.0f + minlight);
}


void R_LightPoint (float *p, float *shadelight);

void R_LightAliasModel (entity_t *e, aliashdr_t *hdr, float *lerporigin, float *lerpangles, meshconstants_t *consts)
{
	float an;
	extern float RTint[256], GTint[256], BTint[256];
	vec3_t adjust_origin;

	VectorCopy (lerporigin, adjust_origin);
	adjust_origin[2] += (e->model->mins[2] + e->model->maxs[2]) / 2;

	R_LightPoint (adjust_origin, consts->shadelight);

	if (e->colorshade)
	{
		consts->shadelight[0] *= RTint[e->colorshade];
		consts->shadelight[1] *= GTint[e->colorshade];
		consts->shadelight[2] *= BTint[e->colorshade];
	}

	if (e == &cl.viewent)
	{
		cl.light_level = consts->shadelight[0];
		R_MinimumLight (consts->shadelight, 24.0f);
	}

	if (e->model->flags & EF_ROTATE)
	{
		consts->shadelight[0] = (94 + sin (lerporigin[0] + lerporigin[1] + (cl.time * 3.8)) * 34) * 2.0f;
		consts->shadelight[1] = (94 + sin (lerporigin[0] + lerporigin[1] + (cl.time * 3.8)) * 34) * 2.0f;
		consts->shadelight[2] = (94 + sin (lerporigin[0] + lerporigin[1] + (cl.time * 3.8)) * 34) * 2.0f;
	}
	else if ((e->drawflags & MLS_MASKIN) == MLS_ABSLIGHT)
	{
		consts->shadelight[0] = e->abslight * 2;
		consts->shadelight[1] = e->abslight * 2;
		consts->shadelight[2] = e->abslight * 2;
	}
	else if ((e->drawflags & MLS_MASKIN) != MLS_NONE)
	{
		// Use a model light style (25-30)
		int styleval = d_lightstylevalue[24 + (e->drawflags & MLS_MASKIN)];

		consts->shadelight[0] = styleval;
		consts->shadelight[1] = styleval;
		consts->shadelight[2] = styleval;
	}

	Vector3Scalef (consts->shadelight, consts->shadelight, (1.0f / 200.0f));

	an = lerpangles[1] / 180 * M_PI;
	consts->shadevector[0] = cos (-an);
	consts->shadevector[1] = sin (-an);
	consts->shadevector[2] = 1;
	Vector3Normalize (consts->shadevector);
}


qboolean R_CullAliasModel (float *mins, float *maxs, float *lerporigin, float *lerpangles)
{
	vec3_t vectors[3];
	int i, f, aggregatemask = ~0;

	// rotate the bounding box
	lerpangles[1] = -lerpangles[1];
	AngleVectors (lerpangles, vectors[0], vectors[1], vectors[2]);
	lerpangles[1] = -lerpangles[1];

	for (i = 0; i < 8; i++)
	{
		vec3_t tmp, bbvec;
		int mask = 0;

		if (i & 1) tmp[0] = mins[0]; else tmp[0] = maxs[0];
		if (i & 2) tmp[1] = mins[1]; else tmp[1] = maxs[1];
		if (i & 4) tmp[2] = mins[2]; else tmp[2] = maxs[2];

		bbvec[0] = Vector3Dot (vectors[0], tmp);
		bbvec[1] = -Vector3Dot (vectors[1], tmp);
		bbvec[2] = Vector3Dot (vectors[2], tmp);

		VectorAdd (lerporigin, bbvec, bbvec);

		for (f = 0; f < 4; f++)
			if (Vector3Dot (frustum[f].normal, bbvec) - frustum[f].dist < 0)
				mask |= (1 << f);

		aggregatemask &= mask;
	}

	if (aggregatemask)
		return true;
	else return false;
}


void R_DrawAliasModel (entity_t *e)
{
	QMATRIX localMatrix;
	QMATRIX objMatrix;

	aliashdr_t *hdr = e->model->aliashdr;
	float lerporigin[3], lerpangles[3];

	meshconstants_t consts;

	// lerp the entity before culling/lightpoint/etc
	R_SetupEntityTransform (e, lerporigin, lerpangles);

	if (e != &cl.viewent)
	{
		if (R_CullAliasModel (e->model->mins, e->model->maxs, lerporigin, lerpangles))
		{
			e->lerpflags |= LERP_RESETALL;
			return;
		}
	}

	Vector3Subtract (modelorg, r_refdef.vieworigin, lerporigin);
	R_TransformAliasModel (e, hdr, &localMatrix, lerporigin, lerpangles, &consts);

	// we need to retain the local matrix for lighting so transform it into objmatrix
	if (e == &cl.viewent)
	{
		// hack the depth range to prevent view model from poking into walls
		R_MatrixLoadf (&objMatrix, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0.3f, 0, 0, 0, 0, 1);

		// if fov is > 90 construct a new MVP as though it were 90
		R_MatrixFrustum (&objMatrix, r_viewmodel_refdef.fov_x, r_viewmodel_refdef.fov_y, 4.0f, R_GetFarClip ());

		// derive a new MVP; we needn't bother deriving a frustum/etc from it because the view model isn't frustum-culled
		R_MatrixMultiply (&objMatrix, &r_view_matrix, &objMatrix);

		// and now apply it all
		R_MatrixMultiply (&objMatrix, &localMatrix, &objMatrix);
	}
	else R_MatrixMultiply (&objMatrix, &localMatrix, &r_mvp_matrix);

	if (e->model->flags & EF_SPECIAL_TRANS)
		D_SetupAliasModel (&objMatrix, 1.0f, lerporigin, true, true);
	else if (e->drawflags & DRF_TRANSLUCENT)
		D_SetupAliasModel (&objMatrix, 0.4f, lerporigin, true, false);
	else if (e->model->flags & EF_TRANSPARENT)
		D_SetupAliasModel (&objMatrix, 1.0f, lerporigin, true, false);
	else if (e->model->flags & EF_HOLEY)
		D_SetupAliasModel (&objMatrix, 1.0f, lerporigin, true, false);
	else D_SetupAliasModel (&objMatrix, 1.0f, lerporigin, false, false);

	R_LightAliasModel (e, hdr, lerporigin, lerpangles, &consts);
	R_SetAliasSkin (e, hdr);
	R_SetupAliasFrame (e, hdr, &consts);

	// now it's OK to set up the consts
	D_SetAliasCBuffer (&consts);

	D_DrawAliasModel (e->model->modelnum);

	// perform dynamic lighting
	R_AliasDlights (e, hdr, &localMatrix);
}

