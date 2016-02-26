/* cylinder.c -- part of vulcan
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

#include <math.h>
#include <stdlib.h>
#include "vector.h"
#include "mesh.h"
#include "cylinder.h"

static void
init_quad(struct polygon *poly, int a, int b, int c, int d)
{
	poly->nvtx = 4;
	poly->vtx_index[0] = a;
	poly->vtx_index[1] = b;
	poly->vtx_index[2] = c;
	poly->vtx_index[3] = d;
}

static void
init_coord(struct vertex *vtx, float r, float a, float z)
{
	float s = sin(a), c = cos(a);

	vtx->pos.x = r*c;
	vtx->pos.y = r*s;
	vtx->pos.z = z;

	vtx->normal.x = c;
	vtx->normal.y = s;
	vtx->normal.z = 0.f;
}

struct mesh *
make_cylinder(int density, float radius, float height)
{
	struct mesh *mesh;
	struct vertex *vtx;
	struct polygon *poly;
	int i, j;
	float a, da;

	mesh = mesh_make();

	mesh->nvtx = 2*density;
	mesh->npoly = density;

	vtx = mesh->vtx = malloc(mesh->nvtx * sizeof *mesh->vtx);
	poly = mesh->poly = malloc(mesh->npoly * sizeof *mesh->poly);

	da = 2.*M_PI/density;

	for (a = 0., i = 0; i < density; i++, a += da) {
		init_coord(&vtx[i], radius, a, 0.f);
		init_coord(&vtx[i + density], radius, a, height);
	}

	for (i = 0; i < density; i++) {
		j = (i + 1)%density;
		init_quad(poly++, i, j, j + density, i + density);
	}

	return mesh;
}
