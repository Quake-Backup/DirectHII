// cl_main.c  -- client main loop

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"
#include "particles.h"

void CL_InitEffects (void);
void CL_ClearEffects (void);

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

// these two are not intended to be set directly
cvar_t	cl_name = {"_cl_name", "player", true};
cvar_t	cl_color = {"_cl_color", "0", true};
cvar_t	cl_playerclass = {"_cl_playerclass", "5", true};

cvar_t	cl_shownet = {"cl_shownet", "0"};	// can be 0, 1, or 2
cvar_t	cl_nolerp = {"cl_nolerp", "0"};

cvar_t	lookspring = {"lookspring", "0", true};
cvar_t	lookstrafe = {"lookstrafe", "0", true};
cvar_t	sensitivity = {"sensitivity", "3", true};
static float save_sensitivity;

cvar_t	m_pitch = {"m_pitch", "0.022", true};
cvar_t	m_yaw = {"m_yaw", "0.022", true};
cvar_t	m_forward = {"m_forward", "1", true};
cvar_t	m_side = {"m_side", "0.8", true};


client_static_t	cls;
client_state_t	cl;

// FIXME: put these on hunk?
entity_t		cl_entities[MAX_EDICTS];
lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
dlight_t		cl_dlights[MAX_DLIGHTS];

int				cl_numvisedicts;
entity_t		*cl_visedicts[MAX_VISEDICTS];
/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState (void)
{
	if (!sv.active)
		Host_ClearMemory ();

	// wipe the entire cl structure
	memset (&cl, 0, sizeof (cl));

	SZ_Clear (&cls.message);

	// clear other arrays
	memset (cl_entities, 0, sizeof (cl_entities));
	memset (cl_dlights, 0, sizeof (cl_dlights));
	memset (cl_lightstyle, 0, sizeof (cl_lightstyle));

	CL_ClearTEnts ();
	CL_ClearEffects ();

	cl.current_frame = cl.current_sequence = 99;
	cl.reference_frame = cl.last_frame = cl.last_sequence = 199;
	cl.need_build = 2;

	plaquemessage = "";

	SB_InvReset ();
}

void CL_RemoveGIPFiles (char *path)
{
	char	name[MAX_OSPATH], tempdir[MAX_OSPATH];
	int i;
	HANDLE handle;
	WIN32_FIND_DATA filedata;
	BOOL retval;

	if (path)
	{
		sprintf (tempdir, "%s\\", path);
	}
	else
	{
		i = GetTempPath (sizeof (tempdir), tempdir);

		if (!i)
		{
			sprintf (tempdir, "%s\\", com_gamedir);
		}
	}

	sprintf (name, "%s*.gip", tempdir);

	handle = FindFirstFile (name, &filedata);
	retval = TRUE;

	while (handle != INVALID_HANDLE_VALUE && retval)
	{
		sprintf (name, "%s%s", tempdir, filedata.cFileName);
		DeleteFile (name);

		retval = FindNextFile (handle, &filedata);
	}

	if (handle != INVALID_HANDLE_VALUE)
		FindClose (handle);
}

qboolean CL_CopyFiles (char *source, char *pat, char *dest)
{
	char	name[MAX_OSPATH], tempdir[MAX_OSPATH];
	HANDLE handle;
	WIN32_FIND_DATA filedata;
	BOOL retval, error;

	handle = FindFirstFile (pat, &filedata);
	retval = TRUE;
	error = false;

	while (handle != INVALID_HANDLE_VALUE && retval)
	{
		sprintf (name, "%s%s", source, filedata.cFileName);
		sprintf (tempdir, "%s%s", dest, filedata.cFileName);

		if (!CopyFile (name, tempdir, FALSE))
			error = true;

		retval = FindNextFile (handle, &filedata);
	}

	if (handle != INVALID_HANDLE_VALUE)
		FindClose (handle);

	return error;
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void)
{
	R_ClearParticles ();	//jfm: need to clear parts because some now check world
	S_StopAllSounds (true);// stop sounds (especially looping!)
	loading_stage = 0;

	// should also stop music
	MIDI_Stop ();
	CDAudio_Stop ();

	// if running a local server, shut it down
	if (cls.demoplayback)
		CL_StopPlayback ();
	else if (cls.state == ca_connected)
	{
		if (cls.demorecording)
			CL_Stop_f ();

		Con_Printf (PRINT_DEVELOPER, "Sending clc_disconnect\n");
		SZ_Clear (&cls.message);
		MSG_WriteByte (&cls.message, clc_disconnect);
		NET_SendUnreliableMessage (cls.netcon, &cls.message);
		SZ_Clear (&cls.message);
		NET_Close (cls.netcon);

		cls.state = ca_disconnected;

		if (sv.active)
			Host_ShutdownServer (false);

		CL_RemoveGIPFiles (NULL);
	}

	cls.demoplayback = cls.timedemo = false;
	cls.signon = 0;
	cl.intermission = 0;
}

void CL_Disconnect_f (void)
{
	CL_Disconnect ();

	if (sv.active)
		Host_ShutdownServer (false);
}




/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address to be passed on
=====================
*/
void CL_EstablishConnection (char *host)
{
	if (cls.state == ca_dedicated)
		return;

	if (cls.demoplayback)
		return;

	CL_Disconnect ();

	cls.netcon = NET_Connect (host);

	if (!cls.netcon)
		Host_Error ("CL_Connect: connect failed\n");

	Con_Printf (PRINT_DEVELOPER, va ("CL_EstablishConnection: connected to %s\n", host));

	cls.demonum = -1;			// not in the demo loop now
	cls.state = ca_connected;
	cls.signon = 0;				// need all the signon messages before playing
}

/*
=====================
CL_SignonReply

An svc_signonnum has been received, perform a client side setup
=====================
*/
void CL_SignonReply (void)
{
	char 	str[8192];

	Con_Printf (PRINT_DEVELOPER, va ("CL_SignonReply: %i\n", cls.signon));

	switch (cls.signon)
	{
	case 1:
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, "prespawn");
		break;

	case 2:
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, va ("name \"%s\"\n", cl_name.string));

		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, va ("playerclass %i\n", (int) cl_playerclass.value));

		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, va ("color %i %i\n", ((int) cl_color.value) >> 4, ((int) cl_color.value) & 15));

		MSG_WriteByte (&cls.message, clc_stringcmd);
		sprintf (str, "spawn %s", cls.spawnparms);
		MSG_WriteString (&cls.message, str);
		break;

	case 3:
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, "begin");
		break;

	case 4:
		SCR_EndLoadingPlaque ();		// allow normal screen updates
		break;
	}
}

/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo (void)
{
	char	str[1024];

	if (cls.demonum == -1)
		return;		// don't play demos

	SCR_BeginLoadingPlaque ();

	if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS)
	{
		cls.demonum = 0;

		if (!cls.demos[cls.demonum][0])
		{
			Con_Printf (PRINT_NORMAL, "No demos listed with startdemos\n");
			cls.demonum = -1;
			return;
		}
	}

	sprintf (str, "playdemo %s\n", cls.demos[cls.demonum]);
	Cbuf_InsertText (str);
	cls.demonum++;
}

/*
==============
CL_PrintEntities_f
==============
*/
void CL_PrintEntities_f (void)
{
	entity_t	*ent;
	int			i;

	for (i = 0, ent = cl_entities; i < cl.num_entities; i++, ent++)
	{
		Con_Printf (PRINT_NORMAL, va ("%3i:", i));

		if (!ent->model)
		{
			Con_Printf (PRINT_NORMAL, "EMPTY\n");
			continue;
		}

		Con_Printf (PRINT_NORMAL, va ("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n",
			ent->model->name, ent->frame, ent->origin[0], ent->origin[1], ent->origin[2], ent->angles[0], ent->angles[1], ent->angles[2]));
	}
}


/*
===============
SetPal

Debugging tool, just flashes the screen
===============
*/
void SetPal (int i)
{
#if 0
	static int old;
	byte	pal[768];
	int		c;

	if (i == old)
		return;

	old = i;

	if (i == 0)
		VID_SetPalette (host_basepal);
	else if (i == 1)
	{
		for (c = 0; c < 768; c += 3)
		{
			pal[c] = 0;
			pal[c+1] = 255;
			pal[c+2] = 0;
		}

		VID_SetPalette (pal);
	}
	else
	{
		for (c = 0; c < 768; c += 3)
		{
			pal[c] = 0;
			pal[c+1] = 0;
			pal[c+2] = 255;
		}

		VID_SetPalette (pal);
	}

#endif
}

/*
===============
CL_AllocDlight

===============
*/
void CL_InitDlight (dlight_t *dl, int key)
{
	memset (dl, 0, sizeof (dlight_t));

	dl->key = key;
	dl->basetime = cl.time;
	dl->minlight = 0.1f;
}


dlight_t *CL_AllocDlight (int key)
{
	int		i;
	dlight_t	*dl;

	// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;

		for (i = 0; i < MAX_DLIGHTS; i++, dl++)
		{
			if (dl->key == key)
			{
				CL_InitDlight (dl, key);
				return dl;
			}
		}
	}

	// then look for a dead light
	dl = cl_dlights;

	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (dl->die < cl.time || dl->radius < dl->minlight)
		{
			CL_InitDlight (dl, key);
			return dl;
		}
	}

	// just take the first one
	// (fixme - take the oldest light)
	dl = &cl_dlights[0];
	CL_InitDlight (dl, key);
	return dl;
}


/*
===============
CL_DecayLights

===============
*/
void CL_DecayLights (void)
{
	int			i;
	dlight_t	*dl;
	float		time;

	time = cl.time - cl.oldtime;

	dl = cl_dlights;

	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (dl->die < cl.time || dl->radius < dl->minlight)
			continue;

		if (dl->baseradius > 0)
		{
			dl->radius = dl->baseradius - ((cl.time - dl->basetime) * dl->decay);
			if (dl->radius < 0) dl->radius = dl->baseradius = 0;
		}
		else
		{
			dl->radius = dl->baseradius + ((cl.time - dl->basetime) * dl->decay);
			if (dl->radius > 0) dl->radius = dl->baseradius = 0;
		}
	}
}


/*
===============
CL_LerpPoint

Determines the fraction between the last two messages that the objects
should be put at.
===============
*/
float	CL_LerpPoint (void)
{
	float	f, frac;

	f = cl.mtime[0] - cl.mtime[1];

	if (!f || cl_nolerp.value || cls.timedemo || sv.active)
	{
		cl.time = cl.mtime[0];
		return 1;
	}

	if (f > HX_FRAME_TIME)
	{
		// dropped packet, or start of demo
		cl.mtime[1] = cl.mtime[0] - HX_FRAME_TIME;
		f = HX_FRAME_TIME;
	}

	frac = (cl.time - cl.mtime[1]) / f;

	if (frac < 0)
	{
		if (frac < -0.01)
		{
			SetPal (1);
			cl.time = cl.mtime[1];
		}

		frac = 0;
	}
	else if (frac > 1)
	{
		if (frac > 1.01)
		{
			SetPal (2);
			cl.time = cl.mtime[0];
		}

		frac = 1;
	}
	else
		SetPal (0);

	return frac;
}


static const double cl_traildelta = (1.0 / 36.0);

void CL_RocketTrail (entity_t *ent, int type)
{
	if (cl.time > ent->nexttrailtime)
	{
		R_RocketTrail (ent->oldtrailorigin, ent->origin, type);
		Vector3Copy (ent->oldtrailorigin, ent->origin);
		ent->nexttrailtime += cl_traildelta;
	}
}


void CL_ClearRocketTrail (entity_t *ent)
{
	Vector3Copy (ent->oldtrailorigin, ent->origin);
	ent->nexttrailtime = cl.time + cl_traildelta;
}


/*
===============
CL_RelinkEntities
===============
*/
void CL_RelinkEntities (void)
{
	entity_t	*ent;
	int			i, j;
	float		frac, f, d;
	vec3_t		delta;
	vec3_t		oldorg;
	dlight_t	*dl;
	int c;
	static int lastc = 0;

	c = 0;

	// determine partial update time
	frac = CL_LerpPoint ();

	cl_numvisedicts = 0;

	// interpolate player info
	for (i = 0; i < 3; i++)
		cl.velocity[i] = cl.mvelocity[1][i] + frac * (cl.mvelocity[0][i] - cl.mvelocity[1][i]);

	if (cls.demoplayback && !intro_playing)
	{
		// interpolate the angles
		for (j = 0; j < 3; j++)
		{
			d = cl.mviewangles[0][j] - cl.mviewangles[1][j];

			if (d > 180)
				d -= 360;
			else if (d < -180)
				d += 360;

			cl.viewangles[j] = cl.mviewangles[1][j] + frac * d;
		}
	}

	// start on the entity after the world
	for (i = 1, ent = cl_entities + 1; i < cl.num_entities; i++, ent++)
	{
		// if the object wasn't included in the last packet, remove it
		if (ent->msgtime != cl.mtime[0] && !(ent->baseline.flags & BE_ON))
		{
			// falls through
			ent->model = NULL;
			ent->lerpflags |= LERP_RESETALL;
		}

		if (!ent->model)
		{
			CL_ClearRocketTrail (ent);
			continue;
		}

		VectorCopy (ent->origin, oldorg);

		if (ent->forcelink || ent->msgtime != cl.mtime[0])
		{
			// the entity was not updated in the last message so move to the final spot
			VectorCopy (ent->msg_origins[0], ent->origin);
			VectorCopy (ent->msg_angles[0], ent->angles);
			CL_ClearRocketTrail (ent);
		}
		else if (ent->lerpflags & LERP_MOVESTEP)
		{
			// will be step-lerped so just move to the final spot
			VectorCopy (ent->msg_origins[0], ent->origin);
			VectorCopy (ent->msg_angles[0], ent->angles);
		}
		else
		{
			// if the delta is large, assume a teleport and don't lerp
			f = frac;

			for (j = 0; j < 3; j++)
			{
				delta[j] = ent->msg_origins[0][j] - ent->msg_origins[1][j];

				if (delta[j] > 100 || delta[j] < -100)
				{
					f = 1;		// assume a teleportation, not a motion
					ent->lerpflags |= LERP_RESETMOVE;
				}
			}

			// interpolate the origin and angles
			for (j = 0; j < 3; j++)
			{
				ent->origin[j] = ent->msg_origins[1][j] + f * delta[j];

				d = ent->msg_angles[0][j] - ent->msg_angles[1][j];

				if (d > 180)
					d -= 360;
				else if (d < -180)
					d += 360;

				ent->angles[j] = ent->msg_angles[1][j] + f * d;
			}
		}

		c++;

		if (ent->effects & EF_DARKFIELD)
			R_DarkFieldParticles (ent);

		if (ent->effects & EF_MUZZLEFLASH)
		{
			vec3_t		fv, rv, uv;

			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin,  dl->origin);
				dl->origin[2] += 16;
				AngleVectors (ent->angles, fv, rv, uv);

				Vector3Madf (dl->origin, fv, 18, dl->origin);
				dl->radius = dl->baseradius = 200 + (rand () & 31);
				dl->minlight = 32;
				dl->die = cl.time + 0.1;
			}

			if (ent == &cl_entities[cl.viewentity])
				cl.viewent.lerpflags |= LERP_RESETANIM | LERP_RESETANIM2;
			else ent->lerpflags |= LERP_RESETANIM;
		}

		if (ent->effects & EF_BRIGHTLIGHT)
		{
			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin,  dl->origin);
				dl->origin[2] += 16;
				dl->radius = dl->baseradius = 400 + (rand () & 31);
				dl->die = cl.time + 0.001;
			}
		}

		if (ent->effects & EF_DIMLIGHT)
		{
			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin,  dl->origin);
				dl->radius = dl->baseradius = 200 + (rand () & 31);
				dl->die = cl.time + 0.001;
			}
		}

		if (ent->effects & EF_DARKLIGHT)
		{
			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin,  dl->origin);
				dl->radius = dl->baseradius = 200.0 + (rand () & 31);
				dl->die = cl.time + 0.001;
				dl->dark = true;
			}
		}

		if (ent->effects & EF_LIGHT)
		{
			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin,  dl->origin);
				dl->radius = dl->baseradius = 200;
				dl->die = cl.time + 0.001;
			}
		}

		if (ent->model->flags & EF_GIB)
			CL_RocketTrail (ent, 2);
		else if (ent->model->flags & EF_ZOMGIB)
			CL_RocketTrail (ent, 4);
		else if (ent->model->flags & EF_BLOODSHOT)
			CL_RocketTrail (ent, 17);
		else if (ent->model->flags & EF_TRACER)
			CL_RocketTrail (ent, 3);
		else if (ent->model->flags & EF_TRACER2)
			CL_RocketTrail (ent, 5);
		else if (ent->model->flags & EF_ROCKET)
		{
			CL_RocketTrail (ent, 0);
			// this is used for gibbing spiders!!!
			/*
			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin, dl->origin);
			dl->radius = dl->baseradius = 200;
			dl->die = cl.time + 0.01;
			*/
		}
		else if (ent->model->flags & EF_FIREBALL)
		{
			CL_RocketTrail (ent, rt_fireball);

			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = dl->baseradius = 120 - (rand () % 20);
				dl->die = cl.time + 0.01;
			}
		}
		else if (ent->model->flags & EF_ACIDBALL)
		{
			CL_RocketTrail (ent, rt_acidball);

			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = dl->baseradius = 120 - (rand () % 20);
				dl->die = cl.time + 0.01;
			}
		}
		else if (ent->model->flags & EF_ICE)
		{
			CL_RocketTrail (ent, rt_ice);
		}
		else if (ent->model->flags & EF_SPIT)
		{
			CL_RocketTrail (ent, rt_spit);

			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = dl->baseradius = -120 - (rand () % 20);
				dl->die = cl.time + HX_FRAME_TIME;
			}
		}
		else if (ent->model->flags & EF_SPELL)
		{
			CL_RocketTrail (ent, rt_spell);
		}
		else if (ent->model->flags & EF_GRENADE)
			CL_RocketTrail (ent, 1);
		else if (ent->model->flags & EF_TRACER3)
			CL_RocketTrail (ent, 6);
		else if (ent->model->flags & EF_VORP_MISSILE)
		{
			CL_RocketTrail (ent, rt_vorpal);
		}
		else if (ent->model->flags & EF_SET_STAFF)
		{
			CL_RocketTrail (ent, rt_setstaff);
		}
		else if (ent->model->flags & EF_MAGICMISSILE)
		{
			if ((rand () & 3) < 1)
				CL_RocketTrail (ent, rt_magicmissile);
		}
		else if (ent->model->flags & EF_BONESHARD)
			CL_RocketTrail (ent, rt_boneshard);
		else if (ent->model->flags & EF_SCARAB)
			CL_RocketTrail (ent, rt_scarab);

		ent->forcelink = false;

		if (i == cl.viewentity)
			continue;

		if (ent->effects & EF_NODRAW)
			continue;

		if (cl_numvisedicts < MAX_VISEDICTS)
		{
			cl_visedicts[cl_numvisedicts] = ent;
			cl_numvisedicts++;
		}
	}
}


/*
===============
CL_ReadFromServer

Read all incoming data from the server
===============
*/
int CL_ReadFromServer (double frametime)
{
	int		ret;

	cl.oldtime = cl.time;
	cl.time += frametime;

	// keep the random time-dependent
	srand ((unsigned int) (cl.time * 36.0));

	do
	{
		ret = CL_GetMessage ();

		if (ret == -1)
			Host_Error ("CL_ReadFromServer: lost server connection");

		if (!ret)
			break;

		cl.last_received_message = realtime;
		CL_ParseServerMessage ();
	} while (ret && cls.state == ca_connected);

	if (cl_shownet.value)
		Con_Printf (PRINT_NORMAL, "\n");

	CL_RelinkEntities ();
	CL_UpdateTEnts ();

	// bring the links up to date
	return 0;
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	usercmd_t		cmd;

	if (cls.state != ca_connected)
		return;

	if (cls.signon == SIGNONS)
	{
		// get basic movement from keyboard
		CL_BaseMove (&cmd);

		// send the unreliable message
		CL_SendMove (&cmd);

	}

	if (cls.demoplayback)
	{
		SZ_Clear (&cls.message);
		return;
	}

	// send the reliable message
	if (!cls.message.cursize)
		return;		// no message at all

	if (!NET_CanSendMessage (cls.netcon))
	{
		Con_Printf (PRINT_DEVELOPER, "CL_WriteToServer: can't send\n");
		return;
	}

	if (NET_SendMessage (cls.netcon, &cls.message) == -1)
		Host_Error ("CL_WriteToServer: lost server connection");

	SZ_Clear (&cls.message);
}

void CL_Sensitivity_save_f (void)
{
	if (Cmd_Argc () != 2)
	{
		Con_Printf (PRINT_NORMAL, "sensitivity_save <save/restore>\n");
		return;
	}

	if (strcmpi (Cmd_Argv (1), "save") == 0)
		save_sensitivity = sensitivity.value;
	else if (strcmpi (Cmd_Argv (1), "restore") == 0)
		Cvar_SetValue ("sensitivity", save_sensitivity);
}
/*
=================
CL_Init
=================
*/
void CL_Init (void)
{
	SZ_Alloc (&cls.message, 1024);

	CL_InitInput ();
	CL_InitTEnts ();
	CL_InitEffects ();

	// register our commands
	Cvar_RegisterVariable (&cl_name, NULL);
	Cvar_RegisterVariable (&cl_color, NULL);
	Cvar_RegisterVariable (&cl_playerclass, NULL);
	Cvar_RegisterVariable (&cl_upspeed, NULL);
	Cvar_RegisterVariable (&cl_forwardspeed, NULL);
	Cvar_RegisterVariable (&cl_backspeed, NULL);
	Cvar_RegisterVariable (&cl_sidespeed, NULL);
	Cvar_RegisterVariable (&cl_movespeedkey, NULL);
	Cvar_RegisterVariable (&cl_yawspeed, NULL);
	Cvar_RegisterVariable (&cl_pitchspeed, NULL);
	Cvar_RegisterVariable (&cl_anglespeedkey, NULL);
	Cvar_RegisterVariable (&cl_shownet, NULL);
	Cvar_RegisterVariable (&cl_nolerp, NULL);
	Cvar_RegisterVariable (&lookspring, NULL);
	Cvar_RegisterVariable (&lookstrafe, NULL);
	Cvar_RegisterVariable (&sensitivity, NULL);

	Cvar_RegisterVariable (&m_pitch, NULL);
	Cvar_RegisterVariable (&m_yaw, NULL);
	Cvar_RegisterVariable (&m_forward, NULL);
	Cvar_RegisterVariable (&m_side, NULL);
	Cvar_RegisterVariable (&cl_prettylights, NULL);

	//	Cvar_RegisterVariable (&cl_autofire, NULL);

	Cmd_AddCommand ("entities", CL_PrintEntities_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("stop", CL_Stop_f);
	Cmd_AddCommand ("playdemo", CL_PlayDemo_f);
	Cmd_AddCommand ("timedemo", CL_TimeDemo_f);
	Cmd_AddCommand ("sensitivity_save", CL_Sensitivity_save_f);
}

