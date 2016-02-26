/* body.c -- part of vulcan
 *
 * This program is copyright (C) 2006 Mauro Persano, and is free
 * software which is freely distributable under the terms of the
 * GNU public license, included as the file COPYING in this
 * distribution.  It is NOT public domain software, and any
 * redistribution not permitted by the GNU General Public License is
 * expressly forbidden without prior written permission from
 * the author.
 *
 */

#include <stdlib.h>
#include "vector.h"
#include "mesh.h"
#include "body.h"
#include "bsptree.h"

void
body_find_bounding_sphere(struct body *body)
{
	struct vector max, min;
	struct vector *p, d;
	float radius;
	int i;
	const struct mesh *m;

	m = body->mesh;

	p = &(m->vtx[0].pos);

	min.x = max.x = p->x;
	min.y = max.y = p->y;
	min.z = max.z = p->z;

	for (i = 1; i < body->mesh->nvtx; i++) {
		p = &(m->vtx[i].pos);

		if (p->x < min.x) min.x = p->x;
		if (p->x > max.x) max.x = p->x;
		if (p->y < min.y) min.y = p->y;
		if (p->y > max.y) max.y = p->y;
		if (p->z < min.z) min.z = p->z;
		if (p->z > max.z) max.z = p->z;
	}

	vec_set(&body->bounding_sphere.center, 
	  .5f*(min.x + max.x), .5f*(min.y + max.y), .5f*(min.z + max.z));

	body->bounding_sphere.radius = 0.f;

	for (i = 0; i < m->nvtx; i++) {
		p = &(m->vtx[i].pos);
		vec_sub(&d, p, &body->bounding_sphere.center);
		radius = vec_length(&d);

		if (radius > body->bounding_sphere.radius)
			body->bounding_sphere.radius = radius;
	}
}
