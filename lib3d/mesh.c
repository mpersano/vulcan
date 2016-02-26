/* mesh.c -- part of vulcan
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
#include <string.h>
#include <assert.h>
#include "vector.h"
#include "matrix.h"
#include "mesh.h"

struct mesh *
mesh_make(void)
{
	struct mesh *m;

	m = malloc(sizeof *m);

	m->npoly = m->nvtx = 0;
	m->vtx = NULL;
	m->poly = NULL;

	return m;
}

void
mesh_free(struct mesh *m)
{
	if (m->vtx != NULL)
		free(m->vtx);

	if (m->poly != NULL)
		free(m->poly);

	free(m);
}

void
mesh_init_poly_normals(struct mesh *mesh)
{
	struct polygon *p, *end;
	struct vector u, v;
	int *vtx_index;
	struct vertex *vtx;

	vtx = mesh->vtx;

	end = &mesh->poly[mesh->npoly];

	for (p = mesh->poly; p != end; p++) {
		assert(p->nvtx >= 3);

		vtx_index = p->vtx_index;

		vec_sub(&u, &vtx[vtx_index[1]].pos, &vtx[vtx_index[0]].pos);
		vec_sub(&v, &vtx[vtx_index[2]].pos, &vtx[vtx_index[0]].pos);

		vec_cross_product(&p->normal, &u, &v);
		vec_normalize(&p->normal);
	}
}

void
mesh_init_vertex_normals(struct mesh *mesh)
{
	struct polygon *p, *poly_end;
	struct vertex *v, *vtx_end;
	struct vector *poly_normal;
	int *idx;
	int nvtx;

	vtx_end = &mesh->vtx[mesh->nvtx];

	for (v = mesh->vtx; v != vtx_end; v++)
		vec_zero(&v->normal);

	poly_end = &mesh->poly[mesh->npoly];

	for (p = mesh->poly; p != poly_end; p++) {
		poly_normal = &p->normal;
		idx = p->vtx_index;
		nvtx = p->nvtx;

		while (nvtx--)
			vec_add_to(&mesh->vtx[*idx++].normal, poly_normal);
	}

	for (v = mesh->vtx; v != vtx_end; v++)
		vec_normalize(&v->normal);
}


void
mesh_transform_all_vertices(struct mesh *mesh, const struct matrix *m)
{
	struct vertex *v, *end;

	end = &mesh->vtx[mesh->nvtx];

	for (v = mesh->vtx; v != end; v++)
		mat_transform(&v->pos, m);
}
