// client.h

typedef struct usercmd_s {
	vec3_t	viewangles;

	// intended velocities
	float	forwardmove;
	float	sidemove;
	float	upmove;
#ifdef QUAKE2RJ
	byte	lightlevel;
#endif
} usercmd_t;

typedef struct lightstyle_s {
	int		length;
	char	map[MAX_STYLESTRING];
} lightstyle_t;

typedef struct scoreboard_s {
	char	name[MAX_SCOREBOARDNAME];
	float	entertime;
	int		frags;
	int		colors;			// two 4 bit fields
	byte	translations[VID_GRADES * 256];
	float	playerclass;
} scoreboard_t;

typedef struct cshift_s {
	float	destcolor[3];
	float	percent;		// 0-256
	float	initialpercent;
	double	inittime;
} cshift_t;


#define	CSHIFT_CONTENTS		0
#define	CSHIFT_DAMAGE		1
#define	CSHIFT_BONUS		2
#define	CSHIFT_INTERVENTION 3
#define CSHIFT_FROZEN		4
#define CSHIFT_STONED		5
#define CSHIFT_INVISIBLE	6
#define CSHIFT_INVINCIBLE	7
#define CSHIFT_HASTE		8
#define CSHIFT_TOMEOFPOWER	9
#define CSHIFT_HEALTH		10
#define	CSHIFT_EXTRA		11
#define	NUM_CSHIFTS			12

#define	NAME_LENGTH	64


//
// client_state_t should hold all pieces of the client state
//

#define	SIGNONS		4			// signon messages to receive before connected

#define	MAX_DLIGHTS		32

typedef struct dlight_s {
	vec3_t	origin;
	vec3_t	transformed;

	// for FPS-independent decay
	float	baseradius;
	float	basetime;

	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
	int		key;

	int		numsurfaces;

	qboolean dark;

	//float	rgb[3];
	int		flags;
} dlight_t;


#define	MAX_MAPSTRING	2048
#define	MAX_DEMOS		8
#define	MAX_DEMONAME	16

typedef enum {
	ca_dedicated, 		// a dedicated server with no ability to start a client
	ca_disconnected, 	// full screen console with no connection
	ca_connected		// valid netcon, talking to a server
} cactive_t;

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
typedef struct client_static_s {
	cactive_t	state;

	// personalization data sent to server
	char		mapstring[MAX_QPATH];
	char		spawnparms[MAX_MAPSTRING];	// to restart a level

	// demo loop control
	int			demonum;		// -1 = don't play demos
	char		demos[MAX_DEMOS][MAX_DEMONAME];		// when not playing

	// demo recording info must be here, because record is started before
	// entering a map (and clearing client_state_t)
	qboolean	demorecording;
	qboolean	demoplayback;
	qboolean	timedemo;
	int			forcetrack;			// -1 = use normal cd track
	FILE		*demofile;
	FILE		*introdemofile;
	int			td_lastframe;		// to meter out one message a frame
	int			td_startframe;		// host_framecount at start
	float		td_starttime;		// realtime at second frame of timedemo


	// connection information
	int			signon;			// 0 to SIGNONS
	struct qsocket_s	*netcon;
	sizebuf_t	message;		// writing buffer to send to server

} client_static_t;

extern client_static_t	cls;

//
// the client_state_t structure is wiped completely at every
// server signon
//
typedef struct client_state_s {
	int			movemessages;	// since connecting to this server
	// throw out the first couple, so the player
	// doesn't accidentally do something the
	// first frame
	usercmd_t	cmd;			// last command sent to the server

	int			Protocol;

	// information for local display
	int			stats[MAX_CL_STATS];	// health, etc
	int			inv_order[MAX_INVENTORY];
	int			inv_count, inv_startpos, inv_selected;
	int			items;			// inventory bit flags
	float		item_gettime[32];	// cl.time of aquiring item, for blinking
	float		faceanimtime;	// use anim frame if cl.time < this

	entvars_t		v; // NOTE: not every field will be update - you must specifically add
	// them in functions SV_WriteClientdatatToMessag () and CL_ParseClientdat ()

	cshift_t	cshifts[NUM_CSHIFTS];	// color shifts for damage, powerups
	cshift_t	prev_cshifts[NUM_CSHIFTS];	// and content types

	char puzzle_pieces[8][10]; // puzzle piece names

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  The server sets punchangle when
	// the view is temporarliy offset, and an angle reset commands at the start
	// of each level and after teleporting.
	vec3_t		mviewangles[2];	// during demo playback viewangles is lerped
	// between these
	vec3_t		viewangles;

	vec3_t		mvelocity[2];	// update by server, used for lean+bob
	// (0 is newest)
	vec3_t		velocity;		// lerped between mvelocity[0] and [1]

	vec3_t		punchangle;		// temporary offset

	float		viewheight;
	float		crouch;			// local amount for smoothing stepups

	qboolean	paused;			// send over by server
	qboolean	onground;
	qboolean	inwater;

	int			intermission;	// don't change view angle, full screen, etc
	int			completed_time;	// latched at intermission start

	double		mtime[2];		// the timestamp of last two messages
	double		time;			// clients view of time, should be between
	// servertime and oldservertime to generate
	// a lerp point for other data
	double		oldtime;		// previous cl.time, time-oldtime is used
	// to decay light values and smooth step ups

	double		lastparticletime;

	float		last_received_message;	// (realtime) for net trouble icon

	// information that is static for the entire time connected to a server
	struct model_s		*model_precache[MAX_MODELS];
	struct sfx_s		*sound_precache[MAX_SOUNDS];

	char		levelname[40];	// for display on solo scoreboard
	int			viewentity;		// cl_entitites[cl.viewentity] = player
	int			maxclients;
	int			gametype;

	// refresh related state
	struct model_s	*worldmodel;	// cl_entitites[0].model
	int			num_entities;	// held in cl_entities array
	entity_t	viewent;			// the gun model
	struct EffectT Effects[MAX_EFFECTS];

	int			cdtrack, looptrack;	// cd audio
	char		midi_name[128];     // midi file name
	byte		current_frame, last_frame, reference_frame;
	byte		current_sequence, last_sequence;
	byte		need_build;

	// frag scoreboard
	scoreboard_t	*scores;		// [cl.maxclients]

#ifdef QUAKE2RJ
	// light level at player's position including dlights
	// this is sent back to the server each frame
	// architectually ugly but it works
	int			light_level;
#endif

	client_frames2_t frames[3]; // 0 = base, 1 = building, 2 = 0 & 1 merged
	short RemoveList[MAX_CLIENT_STATES], NumToRemove;

	long	info_mask, info_mask2;
} client_state_t;


//
// cvars
//
extern	cvar_t	cl_name;
extern	cvar_t	cl_color;
extern	cvar_t	cl_playerclass;

extern	cvar_t	cl_upspeed;
extern	cvar_t	cl_forwardspeed;
extern	cvar_t	cl_backspeed;
extern  cvar_t	cl_prettylights;
extern	cvar_t	cl_sidespeed;

extern	cvar_t	cl_movespeedkey;

extern	cvar_t	cl_yawspeed;
extern	cvar_t	cl_pitchspeed;

extern	cvar_t	cl_anglespeedkey;

extern	cvar_t	cl_autofire;

extern	cvar_t	cl_shownet;
extern	cvar_t	cl_nolerp;

extern	cvar_t	lookspring;
extern	cvar_t	lookstrafe;
extern	cvar_t	sensitivity;

extern	cvar_t	m_pitch;
extern	cvar_t	m_yaw;
extern	cvar_t	m_forward;
extern	cvar_t	m_side;


extern	client_state_t	cl;

// FIXME, allocate dynamically
extern	entity_t		cl_entities[MAX_EDICTS];
extern	lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
extern	dlight_t		cl_dlights[MAX_DLIGHTS];

//=============================================================================

//
// cl_main
//
dlight_t *CL_AllocDlight (int key);
void	CL_DecayLights (void);

void CL_Init (void);

void CL_EstablishConnection (char *host);
void CL_Signon1 (void);
void CL_Signon2 (void);
void CL_Signon3 (void);
void CL_Signon4 (void);
void CL_SignonReply (void);

void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_NextDemo (void);

#define			MAX_VISEDICTS	2048

extern	int				cl_numvisedicts;
extern	entity_t		*cl_visedicts[MAX_VISEDICTS];

//
// cl_input
//
typedef struct kbutton_s {
	int		down[2];		// key nums holding it down
	int		state;			// low bit is down state
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_SendMove (usercmd_t *cmd);

void CL_ClearState (void);

int  CL_ReadFromServer (double frametime);
void CL_WriteToServer (usercmd_t *cmd);
void CL_BaseMove (usercmd_t *cmd);


float CL_KeyState (kbutton_t *key);
char *Key_KeynumToString (int keynum);

//
// cl_demo.c
//
void CL_StopPlayback (void);
int CL_GetMessage (void);

void CL_Stop_f (void);
void CL_Record_f (void);
void CL_PlayDemo_f (void);
void CL_TimeDemo_f (void);

//
// cl_parse.c
//
void CL_ParseServerMessage (void);

//
// view
//
void V_RenderView (void);
void V_UpdatePalette (void);
void V_Register (void);
void V_ParseDamage (void);
void V_SetContentsColor (int contents);

//
// cl_tent
//
void CL_InitTEnts (void);
void CL_ClearTEnts (void);
void CL_ParseTEnt (void);
void CL_UpdateTEnts (void);

void CL_RocketTrail (entity_t *ent, int type);
void CL_ClearRocketTrail (entity_t *ent);

