

#define	SFL_FLUFFY			1	// All largish flakes
#define	SFL_MIXED			2	// Mixed flakes
#define	SFL_HALF_BRIGHT		4	// All flakes start darker
#define	SFL_NO_MELT			8	// Flakes don't melt when his surface, just go away
#define	SFL_IN_BOUNDS		16	// Flakes cannot leave the bounds of their box
#define	SFL_NO_TRANS		32	// All flakes start non-translucent
#define SFL_64	64
#define SFL_128	128


// Changes to ptype_t must also be made in d_iface.h
typedef enum _ptype_t {
	pt_static,
	pt_grav,
	pt_fastgrav,
	pt_slowgrav,
	pt_fire,
	pt_explode,
	pt_explode2,
	pt_blob,
	pt_blob2,
	pt_rain,
	pt_c_explode,
	pt_c_explode2,
	pt_spit,
	pt_fireball,
	pt_ice,
	pt_spell,
	pt_test,
	pt_quake,
	pt_rd,			// rider's death
	pt_vorpal,
	pt_setstaff,
	pt_magicmissile,
	pt_boneshard,
	pt_scarab,
	pt_acidball,
	pt_darken,
	pt_snow,
	pt_gravwell,
	pt_redfire
} ptype_t;

// Changes to rtype_t must also be made in glquake.h
typedef enum _rt_type_t {
	rt_rocket_trail = 0,
	rt_smoke,
	rt_blood,
	rt_tracer,
	rt_slight_blood,
	rt_tracer2,
	rt_voor_trail,
	rt_fireball,
	rt_ice,
	rt_spit,
	rt_spell,
	rt_vorpal,
	rt_setstaff,
	rt_magicmissile,
	rt_boneshard,
	rt_scarab,
	rt_acidball,
	rt_bloodshot,
} rt_type_t;

// !!! if this is changed, it must be changed in d_iface.h too !!!
typedef struct particle_s {
	// driver-usable fields
	vec3_t		org;
	float		color;
	int			size;

	// drivers never touch the following fields
	struct particle_s	*next;
	vec3_t		vel;
	vec3_t		min_org;
	vec3_t		max_org;
	float		ramp;
	float		die;
	byte		type;
	byte		flags;
	byte		count;
} particle_t;


void R_RunParticleEffect2 (vec3_t org, vec3_t dmin, vec3_t dmax, int color, int effect, int count);
void R_DarkFieldParticles (entity_t *ent);
void R_ClearParticles (void);


extern particle_t	*active_particles;
extern particle_t	*free_particles;

