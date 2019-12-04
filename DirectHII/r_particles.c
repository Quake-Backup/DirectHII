// gl_main.c

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

#include "particles.h"
#include "d_particles.h"


/*
=======================================================================================================================

PARTICLES

=======================================================================================================================
*/

void R_DrawParticles (void)
{
	for (;;)
	{
		particle_t *kill = active_particles;

		if (kill && kill->die < cl.time)
		{
			active_particles = kill->next;
			kill->next = free_particles;
			free_particles = kill;
			continue;
		}

		break;
	}

	// nothing to draw
	if (!active_particles) return;

	D_BeginParticles ();

	for (particle_t *p = active_particles; p; p = p->next)
	{
		for (;;)
		{
			particle_t *kill = p->next;

			if (kill && kill->die < cl.time)
			{
				p->next = kill->next;
				kill->next = free_particles;
				free_particles = kill;
				continue;
			}

			break;
		}

		if (p->color > 255)
			D_AddParticle (p->org, d_8to24table_part[((int) p->color - 256) & 255], p->size);
		else D_AddParticle (p->org, d_8to24table_rgba[((int) p->color) & 255], p->size);
	}

	D_EndParticles ();
}



