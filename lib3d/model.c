/* model.c -- part of vulcan
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
#include "model.h"
#include "bsptree.h"

void
model_find_bounding_sphere(struct model *model)
{
	struct vector max, min;
	struct vector *p, d;
	float radius;
	int i;

	p = &(model->mesh->vtx[0].pos);

	min.x = max.x = p->x;
	min.y = max.y = p->y;
	min.z = max.z = p->z;

	for (i = 1; i < model->mesh->nvtx; i++) {
		p = &(model->mesh->vtx[i].pos);

		if (p->x < min.x) min.x = p->x;
		if (p->x > max.x) max.x = p->x;
		if (p->y < min.y) min.y = p->y;
		if (p->y > max.y) max.y = p->y;
		if (p->z < min.z) min.z = p->z;
		if (p->z > max.z) max.z = p->z;
	}

	vec_set(&model->bounding_sphere.center, 
	  .5f*(min.x + max.x), .5f*(min.y + max.y), .5f*(min.z + max.z));

	model->bounding_sphere.radius = 0.f;

	for (i = 0; i < model->mesh->nvtx; i++) {
		p = &(model->mesh->vtx[i].pos);
		vec_sub(&d, p, &model->bounding_sphere.center);
		radius = vec_length(&d);

		if (radius > model->bounding_sphere.radius)
			model->bounding_sphere.radius = radius;
	}
}
