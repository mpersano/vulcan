/* mesh.h -- part of vulcan
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

#ifndef MESH_H_
#define MESH_H_

#include "vector.h"

#define MAX_POLY_VTX 20

struct vertex {
	struct vector pos;
	struct vector normal;
};

struct polygon {
	int nvtx;
	int vtx_index[MAX_POLY_VTX];
	struct vector normal;
};

struct mesh {
	int npoly, nvtx;
	struct vertex *vtx;
	struct polygon *poly;
};

struct mesh *
mesh_make(void);

void
mesh_free(struct mesh *mesh);

void
mesh_init_poly_normals(struct mesh *mesh);

void
mesh_init_vertex_normals(struct mesh *mesh);

struct matrix;

void
mesh_transform_all_vertices(struct mesh *mesh, const struct matrix *m);

#endif /* MESH_H_ */
