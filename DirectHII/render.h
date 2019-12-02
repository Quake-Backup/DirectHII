
// refresh.h -- public interface to refresh functions

#define	MAXCLIPPLANES	11

#define	TOP_RANGE		16			// soldier uniform colors
#define	BOTTOM_RANGE	96

//=============================================================================

typedef struct efrag_s
{
	struct mleaf_s		*leaf;
	struct efrag_s		*leafnext;
	struct entity_s		*entity;
	struct efrag_s		*entnext;
} efrag_t;


#define LERP_MOVESTEP	(1 << 0) // this is a MOVETYPE_STEP entity, enable movement lerp
#define LERP_RESETANIM	(1 << 1) // disable anim lerping until next anim frame
#define LERP_RESETANIM2	(1 << 2) // set this and previous flag to disable anim lerping for two anim frames
#define LERP_RESETMOVE	(1 << 3) // disable movement lerping until next origin/angles change
#define LERP_FINISH		(1 << 4) // use lerpfinish time from server update instead of assuming interval of HX_FRAME_TIME

#define LERP_RESETALL	(LERP_RESETANIM | LERP_RESETANIM2 | LERP_RESETMOVE)

typedef struct entity_s
{
	qboolean				forcelink;		// model changed

	int						update_type;

	entity_state3_t			baseline;		// to fill in defaults in updates

	double					msgtime;		// time of last update
	vec3_t					msg_origins[2];	// last two updates (0 is newest)
	vec3_t					origin;
	vec3_t					oldtrailorigin;
	double					nexttrailtime;
	vec3_t					msg_angles[2];	// last two updates (0 is newest)
	vec3_t					angles;
	struct model_s			*model;			// NULL = no model
	struct efrag_s			*efrag;			// linked list of efrags
	int						frame;
	float					syncbase;		// for client-side animations
	byte					*colormap, *sourcecolormap;
	byte					colorshade;
	int						effects;		// light, particals, etc
	int						skinnum;		// for Alias models
	int						scale;			// for Alias models
	int						drawflags;		// for Alias models
	int						abslight;		// for Alias models
	int						visframe;		// last frame this entity was found in an active leaf

	// FIXME: could turn these into a union
	int						trivial_accept;
	struct mnode_s			*topnode;		// for bmodels, first world node
											//  that splits bmodel, or NULL if
											//  not split

	// lerping info
	byte					lerpflags;		// lerping
	double					lerpstart;		// animation lerping
	double					lerpfinish;		// lerping -- server sent us a more accurate interval, use it instead of 0.1
	short					prevpose;	// animation lerping
	short					currpose;	// animation lerping
	double					originlerpstart;	// transform lerping
	double					angleslerpstart;	// transform lerping
	vec3_t					prevorigin;	// transform lerping
	vec3_t					currorigin;	// transform lerping
	vec3_t					prevangles;	// transform lerping
	vec3_t					currangles;	// transform lerping
} entity_t;


// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct refdef_s
{
	vec3_t		vieworigin;
	vec3_t		viewangles;

	int			ambientlight;

	float		fov_x, fov_y;
} refdef_t;


//
// refresh
//
extern	refdef_t	r_refdef;
extern vec3_t	r_origin, vpn, vright, vup;


void R_Init (void);
void R_InitEfrags (void);
void R_RenderView (void);		// must set r_refdef first

void R_AddEfrags (entity_t *ent);
void R_NewMap (void);


void R_ParseParticleEffect (void);
void R_ParseParticleEffect2 (void);
void R_ParseParticleEffect3 (void);
void R_ParseParticleEffect4 (void);
void R_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count);
void R_RocketTrail (vec3_t start, vec3_t end, int type);
void R_SunStaffTrail (vec3_t source, vec3_t dest);

#ifdef QUAKE2
void R_DarkFieldParticles (entity_t *ent);
#endif
void R_RainEffect (vec3_t org, vec3_t e_size, int x_dir, int y_dir, int color, int count);
void R_SnowEffect (vec3_t org1, vec3_t org2, int flags, vec3_t alldir, int count);
void R_ColoredParticleExplosion (vec3_t org, int color, int radius, int counter);

void R_EntityParticles (entity_t *ent);
void R_BlobExplosion (vec3_t org);
void R_ParticleExplosion (vec3_t org);
void R_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength);
void R_LavaSplash (vec3_t org);
void R_TeleportSplash (vec3_t org);

