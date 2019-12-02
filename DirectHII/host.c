// host.c -- coordinates spawning and killing of local servers

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

void History_Shutdown (void);
void ED_ClearEdict (edict_t *e);

#define HOST_FRAME_DELTA	(1.0 / 72.0)

/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

void Host_WriteConfiguration (char *fname);

quakeparms_t host_parms;

qboolean	host_initialized;		// true if into command execution

double		realtime;				// without any filtering or bounding
double		oldrealtime;			// last frame run
int			host_framecount;

client_t	*host_client;			// current client

jmp_buf 	host_abortserver;

byte		*host_basepal;
byte		*host_colormap;


extern cvar_t	sys_quake2;

cvar_t	host_framerate = {"host_framerate", "0"};	// set for slow motion
cvar_t	host_timescale = {"host_timescale", "0"};	// set for slow motion
cvar_t	host_speeds = {"host_speeds", "0"};			// set for running times

cvar_t	sys_ticrate = {"sys_ticrate", "0.05"};
cvar_t	serverprofile = {"serverprofile", "0"};

cvar_t	fraglimit = {"fraglimit", "0", false, true};
cvar_t	timelimit = {"timelimit", "0", false, true};
cvar_t	teamplay = {"teamplay", "0", false, true};

cvar_t	samelevel = {"samelevel", "0"};
cvar_t	noexit = {"noexit", "0", false, true};

#ifdef QUAKE2
cvar_t	developer = {"developer", "1", true};	// should be 0 for release!
#else
cvar_t	developer = {"developer", "0", true};
#endif

cvar_t	skill = {"skill", "1"};						// 0 - 3
cvar_t	deathmatch = {"deathmatch", "0"};			// 0, 1, or 2
cvar_t	randomclass = {"randomclass", "0"};			// 0, 1, or 2
cvar_t	coop = {"coop", "0"};			// 0 or 1

cvar_t	pausable = {"pausable", "1"};
cvar_t	sys_adaptive = {"sys_adaptive", "1", true};

cvar_t	temp1 = {"temp1", "0"};


/*
================
Host_EndGame
================
*/
void Host_EndGame (char *message, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr, message);
	vsprintf (string, message, argptr);
	va_end (argptr);
	Con_Printf (PRINT_DEVELOPER, va ("Host_EndGame: %s\n", string));

	if (sv.active)
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_EndGame: %s\n", string);	// dedicated servers exit

	if (cls.demonum != -1)
		CL_NextDemo ();
	else
		CL_Disconnect ();

	longjmp (host_abortserver, 1);
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error (char *error, ...)
{
	va_list		argptr;
	char		string[1024];
	static	qboolean inerror = false;

	if (inerror)
		Sys_Error ("Host_Error: recursively entered");

	inerror = true;

	SCR_EndLoadingPlaque ();		// reenable screen updates

	va_start (argptr, error);
	vsprintf (string, error, argptr);
	va_end (argptr);
	Con_Printf (PRINT_NORMAL, va ("Host_Error: %s\n", string));

	if (sv.active)
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_Error: %s\n", string);	// dedicated servers exit

	CL_Disconnect ();
	cls.demonum = -1;

	inerror = false;

	longjmp (host_abortserver, 1);
}

/*
================
Host_FindMaxClients
================
*/
void	Host_FindMaxClients (void)
{
	int		i;

	svs.maxclients = 1;

	i = COM_CheckParm ("-dedicated");

	if (i)
	{
		cls.state = ca_dedicated;

		if (i != (com_argc - 1))
		{
			svs.maxclients = atoi (com_argv[i+1]);
		}
		else
			svs.maxclients = 8;
	}
	else
		cls.state = ca_disconnected;

	i = COM_CheckParm ("-listen");

	if (i)
	{
		if (cls.state == ca_dedicated)
			Sys_Error ("Only one of -dedicated or -listen can be specified");

		if (i != (com_argc - 1))
			svs.maxclients = atoi (com_argv[i+1]);
		else
			svs.maxclients = 8;
	}

	if (svs.maxclients < 1)
		svs.maxclients = 8;
	else if (svs.maxclients > MAX_SCOREBOARD)
		svs.maxclients = MAX_SCOREBOARD;

	svs.maxclientslimit = svs.maxclients;

	if (svs.maxclientslimit < 4)
		svs.maxclientslimit = 4;

	svs.clients = Hunk_Alloc (GAME_HUNK, svs.maxclientslimit * sizeof (client_t));

	if (svs.maxclients > 1)
		Cvar_SetValue ("deathmatch", 1.0);
	else
		Cvar_SetValue ("deathmatch", 0.0);
}

/*
===============
Host_SaveConfig_f
===============
*/
void Host_SaveConfig_f (void)
{

	if (cmd_source != src_command)
		return;

	if (Cmd_Argc () != 2)
	{
		Con_Printf (PRINT_NORMAL, "saveConfig <savename> : save a config file\n");
		return;
	}

	if (strstr (Cmd_Argv (1), ".."))
	{
		Con_Printf (PRINT_NORMAL, "Relative pathnames are not allowed.\n");
		return;
	}

	Host_WriteConfiguration (Cmd_Argv (1));
}


/*
=======================
Host_InitLocal
======================
*/
void Host_InitLocal (void)
{
	Cmd_AddCommand ("saveconfig", Host_SaveConfig_f);

	Host_InitCommands ();

	Cvar_RegisterVariable (&sys_quake2, NULL);

	Cvar_RegisterVariable (&host_timescale, NULL);
	Cvar_RegisterVariable (&host_framerate, NULL);
	Cvar_RegisterVariable (&host_speeds, NULL);

	Cvar_RegisterVariable (&sys_ticrate, NULL);
	Cvar_RegisterVariable (&serverprofile, NULL);

	Cvar_RegisterVariable (&fraglimit, NULL);
	Cvar_RegisterVariable (&timelimit, NULL);
	Cvar_RegisterVariable (&teamplay, NULL);
	Cvar_RegisterVariable (&samelevel, NULL);
	Cvar_RegisterVariable (&noexit, NULL);
	Cvar_RegisterVariable (&skill, NULL);
	Cvar_RegisterVariable (&developer, NULL);
	Cvar_RegisterVariable (&deathmatch, NULL);
	Cvar_RegisterVariable (&randomclass, NULL);
	Cvar_RegisterVariable (&coop, NULL);

	Cvar_RegisterVariable (&pausable, NULL);

	Cvar_RegisterVariable (&sys_adaptive, NULL);

	Cvar_RegisterVariable (&temp1, NULL);

	Host_FindMaxClients ();
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration (char *fname)
{
	// dedicated servers initialize the host but don't parse and set the
	// config.cfg cvars
	if (host_initialized && !isDedicated)
	{
		FILE *f = fopen (va ("%s/%s", com_gamedir, fname), "w");

		if (!f)
		{
			Con_Printf (PRINT_NORMAL, va ("Couldn't write %s.\n", fname));
			return;
		}

		Key_WriteBindings (f);
		Cvar_WriteVariables (f);

		fclose (f);
	}
}


/*
=================
SV_ClientPrintf

Sends text across to be displayed
FIXME: make this just a stuffed echo?
=================
*/
void SV_ClientPrintf (char *string)
{
	MSG_WriteByte (&host_client->message, svc_print);
	MSG_WriteString (&host_client->message, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf (char *string)
{
	int			i;

	for (i = 0; i < svs.maxclients; i++)
	{
		if (svs.clients[i].active && svs.clients[i].spawned)
		{
			MSG_WriteByte (&svs.clients[i].message, svc_print);
			MSG_WriteString (&svs.clients[i].message, string);
		}
	}
}


/*
=================
Host_ClientCommands

Send text over to the client to be executed
=================
*/
void Host_ClientCommands (char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr, fmt);
	vsprintf (string, fmt, argptr);
	va_end (argptr);

	MSG_WriteByte (&host_client->message, svc_stufftext);
	MSG_WriteString (&host_client->message, string);
}

/*
=====================
SV_DropClient

Called when the player is getting totally kicked off the host
if (crash = true), don't bother sending signofs
=====================
*/
void SV_DropClient (qboolean crash)
{
	int		saveSelf;
	int		i;
	client_t *client;

	if (!crash)
	{
		// send any final messages (don't check for errors)
		if (NET_CanSendMessage (host_client->netconnection))
		{
			MSG_WriteByte (&host_client->message, svc_disconnect);
			NET_SendMessage (host_client->netconnection, &host_client->message);
		}

		if (host_client->edict && host_client->spawned)
		{
			// call the prog function for removing a client
			// this will set the body to a dead frame, among other things
			saveSelf = pr_global_struct->self;
			pr_global_struct->self = EDICT_TO_PROG (host_client->edict);
			PR_ExecuteProgram (pr_global_struct->ClientDisconnect);
			pr_global_struct->self = saveSelf;
		}

		Sys_Printf ("Client %s removed\n", host_client->name);
	}

	// break the net connection
	NET_Close (host_client->netconnection);
	host_client->netconnection = NULL;

	// free the client (the body stays around)
	host_client->active = false;
	host_client->name[0] = 0;
	host_client->old_frags = -999999;
	memset (&host_client->old_v, 0, sizeof (host_client->old_v));
	ED_ClearEdict (host_client->edict);
	host_client->send_all_v = true;
	net_activeconnections--;

	// send notification to all clients
	for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		if (!client->active)
			continue;

		MSG_WriteByte (&client->message, svc_updatename);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteString (&client->message, "");
		MSG_WriteByte (&client->message, svc_updatefrags);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteShort (&client->message, 0);
		MSG_WriteByte (&client->message, svc_updatecolors);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteByte (&client->message, 0);
	}
}

/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void Host_ShutdownServer (qboolean crash)
{
	int		i;
	int		count;
	sizebuf_t	buf;
	byte		message[4];
	double	start;

	if (!sv.active)
		return;

	sv.active = false;

	// stop all client sounds immediately
	if (cls.state == ca_connected)
		CL_Disconnect ();

	// flush any pending messages - like the score!!!
	start = Sys_DoubleTime ();

	do
	{
		count = 0;

		for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		{
			if (host_client->active && host_client->message.cursize)
			{
				if (NET_CanSendMessage (host_client->netconnection))
				{
					NET_SendMessage (host_client->netconnection, &host_client->message);
					SZ_Clear (&host_client->message);
				}
				else
				{
					NET_GetMessage (host_client->netconnection);
					count++;
				}
			}
		}

		if ((Sys_DoubleTime () - start) > 3.0)
			break;
	}
	while (count);

	// make sure all the clients know we're disconnecting
	buf.data = message;
	buf.maxsize = 4;
	buf.cursize = 0;
	MSG_WriteByte (&buf, svc_disconnect);
	count = NET_SendToAll (&buf, 5);

	if (count)
		Con_Printf (PRINT_NORMAL, va ("Host_ShutdownServer: NET_SendToAll failed for %u clients\n", count));

	for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		if (host_client->active)
			SV_DropClient (crash);

	// clear structures
	memset (&sv, 0, sizeof (sv));
	memset (svs.clients, 0, svs.maxclientslimit * sizeof (client_t));
}


/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void Host_ClearMemory (void)
{
	Con_Printf (PRINT_DEVELOPER, "Clearing memory\n");
	Mod_ClearAll ();

	Hunk_FreeToLowMark (MAP_HUNK, 0);
	Hunk_FreeToLowMark (LOAD_HUNK, 0);
	Hunk_FreeToLowMark (FRAME_HUNK, 0);

	cls.signon = 0;
	memset (&sv, 0, sizeof (sv));
	memset (&cl, 0, sizeof (cl));
}


//============================================================================


/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void Host_GetConsoleCommands (void)
{
	char	*cmd;

	while (1)
	{
		cmd = Sys_ConsoleInput ();

		if (!cmd)
			break;

		Cbuf_AddText (cmd);
	}
}


void R_UpdateParticles (void);
void CL_UpdateEffects (void);

/*
==================
Host_ServerFrame

==================
*/
void Host_ServerFrame (double frametime)
{
	// run the world state
	pr_global_struct->frametime = frametime;

	// set the time and clear the general datagram
	SV_ClearDatagram ();

	// check for new clients
	SV_CheckForNewClients ();

	// read client messages
	SV_RunClients (frametime);

	// move things around and think
	// always pause in single player if in console or menus
	if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game))
		SV_Physics (frametime);

	// send all messages to the clients
	SV_SendClientMessages ();
}


/*
==================
Host_Frame

Runs all active servers
==================
*/
void _Host_Frame (float time)
{
	static double		time1 = 0;
	static double		time2 = 0;
	static double		time3 = 0;
	int			pass1, pass2, pass3;
	double		host_frametime;
	double	save_host_frametime, total_host_frametime;
	static double targettime = HOST_FRAME_DELTA;

	Hunk_FreeToLowMark (LOAD_HUNK, 0);
	Hunk_FreeToLowMark (FRAME_HUNK, 0);

	if (setjmp (host_abortserver))
		return;			// something bad happened, or the server disconnected

	// keep the random time dependent
	rand ();

	while (1)
	{
		// decide the simulation time
		realtime = Sys_DoubleTime ();

		// get new key events
		Sys_SendKeyEvents ();

		// bring the mouse state up to date
		IN_Frame ();

		if (!cls.timedemo && realtime - oldrealtime <= targettime)
		{
			// framerate is too high
			Sys_Sleep ();
			continue;
		}

		// hit target time now
		break;
	}

	// recalibrate realtime and eval the frame time
	realtime = Sys_DoubleTime ();
	host_frametime = realtime - oldrealtime;
	oldrealtime = realtime;

	// save out the unadjusted frametime for the screen-related stuff
	scr_frametime = host_frametime;

	// adjust target time for the next frame to rebalance fast/slow frames
	// (if in a timedemo it must be reset otherwise it will never hit at the end)
	if (cls.timedemo || cls.state != ca_connected || scr_disabled_for_loading || cls.signon != SIGNONS)
		targettime = HOST_FRAME_DELTA;
	else targettime -= host_frametime - HOST_FRAME_DELTA;

	// adjust time for slow motion
	if (host_framerate.value > 0) host_frametime = host_framerate.value;
	if (host_timescale.value > 0) host_frametime *= host_timescale.value;

	// process console commands
	Cbuf_Execute ();

	NET_Poll ();

	// if running the server locally, make intentions now
	if (sv.active)
		CL_SendCmd ();

	//-------------------
	// server operations
	//-------------------

	// check for commands typed to the host
	Host_GetConsoleCommands ();

	save_host_frametime = total_host_frametime = host_frametime;

	if (sys_adaptive.value)
	{
		if (host_frametime > HX_FRAME_TIME)
		{
			host_frametime = HX_FRAME_TIME;
		}
	}

	if (total_host_frametime > 1.0)
		total_host_frametime = HX_FRAME_TIME;

	do
	{
		if (sv.active)
			Host_ServerFrame (host_frametime);

		//-------------------
		// client operations
		//-------------------

		// if running the server remotely, send intentions now after
		// the incoming messages have been read
		if (!sv.active)
			CL_SendCmd ();

		// fetch results from server
		if (cls.state == ca_connected)
		{
			CL_ReadFromServer (host_frametime);
		}

		R_UpdateParticles ();
		CL_UpdateEffects ();

		if (!sys_adaptive.value) break;

		total_host_frametime -= HX_FRAME_TIME;

		if (total_host_frametime > 0 && total_host_frametime < HX_FRAME_TIME)
		{
			save_host_frametime -= total_host_frametime;
			oldrealtime -= total_host_frametime;
			break;
		}
	} while (total_host_frametime > 0);

	host_frametime = save_host_frametime;

	// update video
	if (host_speeds.value)
		time1 = Sys_DoubleTime ();

	SCR_UpdateScreen ();

	if (host_speeds.value)
		time2 = Sys_DoubleTime ();

	// update audio
	if (cls.signon == SIGNONS)
	{
		S_Update (r_origin, vpn, vright, vup);
		CL_DecayLights ();
	}
	else
		S_Update (vec3_origin, vec3_origin, vec3_origin, vec3_origin);

	CDAudio_Update ();

	if (host_speeds.value)
	{
		pass1 = (time1 - time3) * 1000;
		time3 = Sys_DoubleTime ();
		pass2 = (time2 - time1) * 1000;
		pass3 = (time3 - time2) * 1000;
		Con_Printf (PRINT_NORMAL, va ("%3i tot %3i server %3i gfx %3i snd\n", pass1 + pass2 + pass3, pass1, pass2, pass3));
	}

	host_framecount++;
}

void Host_Frame (float time)
{
	double	time1, time2;
	static double	timetotal;
	static int		timecount;
	int		i, c, m;

	if (!serverprofile.value)
	{
		_Host_Frame (time);
		return;
	}

	time1 = Sys_DoubleTime ();
	_Host_Frame (time);
	time2 = Sys_DoubleTime ();

	timetotal += time2 - time1;
	timecount++;

	if (timecount < 1000)
		return;

	m = timetotal * 1000 / timecount;
	timecount = 0;
	timetotal = 0;
	c = 0;

	for (i = 0; i < svs.maxclients; i++)
	{
		if (svs.clients[i].active)
			c++;
	}

	Con_Printf (PRINT_NORMAL, va ("serverprofile: %2i clients %2i msec\n",  c,  m));
}

//============================================================================


extern int vcrFile;
#define	VCR_SIGNATURE	0x56435231
// "VCR1"

void Host_InitVCR (quakeparms_t *parms)
{
	int		i, len, n;
	char	*p;

	if (COM_CheckParm ("-playback"))
	{
		if (com_argc != 2)
			Sys_Error ("No other parameters allowed with -playback\n");

		Sys_FileOpenRead ("quake.vcr", &vcrFile);

		if (vcrFile == -1)
			Sys_Error ("playback file not found\n");

		Sys_FileRead (vcrFile, &i, sizeof (int));

		if (i != VCR_SIGNATURE)
			Sys_Error ("Invalid signature in vcr file\n");

		Sys_FileRead (vcrFile, &com_argc, sizeof (int));
		com_argv = malloc (com_argc * sizeof (char *));
		com_argv[0] = parms->argv[0];

		for (i = 0; i < com_argc; i++)
		{
			Sys_FileRead (vcrFile, &len, sizeof (int));
			p = malloc (len);
			Sys_FileRead (vcrFile, p, len);
			com_argv[i+1] = p;
		}

		com_argc++; /* add one for arg[0] */
		parms->argc = com_argc;
		parms->argv = com_argv;
	}

	if ((n = COM_CheckParm ("-record")) != 0)
	{
		vcrFile = Sys_FileOpenWrite ("quake.vcr");

		i = VCR_SIGNATURE;
		Sys_FileWrite (vcrFile, &i, sizeof (int));
		i = com_argc - 1;
		Sys_FileWrite (vcrFile, &i, sizeof (int));

		for (i = 1; i < com_argc; i++)
		{
			if (i == n)
			{
				len = 10;
				Sys_FileWrite (vcrFile, &len, sizeof (int));
				Sys_FileWrite (vcrFile, "-playback", len);
				continue;
			}

			len = strlen (com_argv[i]) + 1;
			Sys_FileWrite (vcrFile, &len, sizeof (int));
			Sys_FileWrite (vcrFile, com_argv[i], len);
		}
	}

}

/*
====================
Host_Init
====================
*/
void Host_Init (quakeparms_t *parms)
{
	host_parms = *parms;

	com_argc = parms->argc;
	com_argv = parms->argv;

	Cbuf_Init ();
	Cmd_Init ();
	V_Init ();
	Host_InitVCR (parms);
	COM_Init (parms->basedir);
	Host_InitLocal ();
	W_LoadWadFile ("gfx.wad");
	Key_Init ();
	Con_Init ();
	M_Init ();
	PR_Init ();
	Mod_Init ();
	NET_Init ();
	SV_Init ();

	Con_Printf (PRINT_NORMAL, va ("Exe: "__TIME__" "__DATE__"\n"));

	if (cls.state != ca_dedicated)
	{
		host_basepal = (byte *) FS_LoadFile (GAME_HUNK, "gfx/palette.lmp");

		if (!host_basepal)
			Sys_Error ("Couldn't load gfx/palette.lmp");

		host_colormap = (byte *) FS_LoadFile (GAME_HUNK, "gfx/colormap.lmp");

		if (!host_colormap)
			Sys_Error ("Couldn't load gfx/colormap.lmp");

		VID_Init (host_basepal);

		Draw_Init ();
		SCR_Init ();
		R_Init ();

		// FIXME: doesn't use the new one-window approach yet
		S_Init ();

		CDAudio_Init ();
		MIDI_Init ();
		SB_Init ();
		CL_Init ();
		IN_Init ();
	}

	Cbuf_InsertText ("exec hexen.rc\n");

	host_initialized = true;

	Sys_Printf ("======== Hexen II Initialized =========\n");
}


/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown (void)
{
	static qboolean isdown = false;

	if (isdown)
	{
		printf ("recursive shutdown\n");
		return;
	}

	isdown = true;

	// keep Con_Printf from trying to update the screen
	scr_disabled_for_loading = true;

	Host_WriteConfiguration ("config.cfg");

	if (con_initialized) History_Shutdown ();

	CDAudio_Shutdown ();
	MIDI_Cleanup ();
	NET_Shutdown ();
	S_Shutdown ();
	IN_Shutdown ();

	if (cls.state != ca_dedicated)
	{
		VID_Shutdown ();
	}
}

