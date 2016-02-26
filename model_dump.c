/* model_dump.c -- part of vulcan
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "hash_table.h"
#include "vector.h"
#include "mesh.h"
#include "model.h"
#include "bsptree.h"
#include "model_dump.h"

static unsigned
ptr_hash(const void *p)
{
	return (unsigned)p; /* tee-hee. */
}

static int
ptr_equals(const void *p, const void *q)
{
	return p == q;
}

typedef void(*bsp_tree_mapper)(const struct bsp_tree *, void *);

/* we'll need to perform a tree traversal more than a couple of times */
static void
bsp_tree_map(const struct bsp_tree *root, bsp_tree_mapper mapper, void *extra)
{
	mapper(root, extra);

	if (root->front)
		bsp_tree_map(root->front, mapper, extra);

	if (root->back)
		bsp_tree_map(root->back, mapper, extra);
}

struct node_map_context {
	FILE *out;
	int last_idx;
	struct hash_table *node_map;	/* maps node pointers to indices */
};

static void
bsp_tree_build_node_map(const struct bsp_tree *root,
  struct node_map_context *ctx)
{
	hash_table_put(ctx->node_map, (void *)root, (void *)ctx->last_idx++);
}

static void
bsp_tree_dump_plane(const struct bsp_tree *root, FILE *out)
{
	const struct vector *normal = &root->plane.normal;
	const struct vector *pt = &root->plane.pt;
	float d;

	d = -vec_dot_product(normal, pt);

#if 0
	{
	struct vector r;
	float k = -d/(normal->x*normal->x + normal->y*normal->y + normal->z*normal->z);
	float e;
	r.x = k*normal->x;
	r.y = k*normal->y;
	r.z = k*normal->z;
	e = -vec_dot_product(normal, &r);
printf("-> %f/%f %f (%f %f %f)\n", d, e, vec_dot_product(normal, &r), r.x, r.y, r.z);
	assert(fabs(vec_dot_product(normal, &r) + d) < 1e-4);
	}
#endif

	fprintf(out, "\t\t\t\t%.5f %.5f %.5f %.5f,\n",
	  normal->x, normal->y, normal->z, d);
}

static void
bsp_tree_dump_node(const struct bsp_tree *root, struct node_map_context *ctx)
{
	int this_idx, front_idx, back_idx;

	this_idx = (int)hash_table_get(ctx->node_map, root);
	front_idx = root->front ? (int)hash_table_get(ctx->node_map, root->front) : -1;
	back_idx = root->back ? (int)hash_table_get(ctx->node_map, root->back) : -1;

	assert(this_idx && front_idx && back_idx);

	fprintf(ctx->out, "\t\t\t\t%d %d %d,\n", this_idx, front_idx, back_idx);
}

void
bsp_tree_dump(FILE *out, const struct bsp_tree *bsp_tree)
{
	struct node_map_context node_map_context;

	fprintf(out, "\t\tbsp-tree {\n");

	/* dump node planes */
	fprintf(out, "\t\t\tplanes {\n");
	bsp_tree_map(bsp_tree, (bsp_tree_mapper)bsp_tree_dump_plane, out);
	fprintf(out, "\t\t\t}\n");

	/* build a map of node pointers to node indices */
	node_map_context.last_idx = 1;
	node_map_context.node_map = hash_table_make(3331, ptr_hash, ptr_equals);

	/* initialize node_map */
	bsp_tree_map(bsp_tree, (bsp_tree_mapper)bsp_tree_build_node_map,
	  &node_map_context);

	/* dump node indices */
	node_map_context.out = out;

	fprintf(out, "\t\t\tnodes {\n");
	bsp_tree_map(bsp_tree, (bsp_tree_mapper)bsp_tree_dump_node,
	  &node_map_context);
	fprintf(out, "\t\t\t}\n");

	fprintf(out, "\t\t}\n");
}

void
bounding_sphere_dump(FILE *out, const struct sphere *sphere)
{
	fprintf(out, "\t\tbounding-sphere {\n");
	fprintf(out, "\t\t\tcenter %f %f %f\n",
	  sphere->center.x, sphere->center.y, sphere->center.z);
	fprintf(out, "\t\t\tradius %f\n", sphere->radius);
	fprintf(out, "\t\t}\n");
}

static void
mesh_dump(FILE *out, const struct mesh *mesh)
{
	int i, j;

	fprintf(out, "\tmesh {\n");
	
	fprintf(out, "\t\tcoordinates {\n");
	for (i = 0; i < mesh->nvtx; i++)
		fprintf(out, "\t\t\t%.5f %.5f %.5f,\n",
		  mesh->vtx[i].pos.x, mesh->vtx[i].pos.y, mesh->vtx[i].pos.z);
	fprintf(out, "\t\t}\n");

	fprintf(out, "\t\tnormals {\n");
	for (i = 0; i < mesh->nvtx; i++)
		fprintf(out, "\t\t\t%.5f %.5f %.5f,\n",
		  mesh->vtx[i].normal.x, mesh->vtx[i].normal.y,
		  mesh->vtx[i].normal.z);
	fprintf(out, "\t\t}\n");

	fprintf(out, "\t\ttriangles {\n");
	for (i = 0; i < mesh->npoly; i++) {
		const struct polygon *p = &mesh->poly[i];

		for (j = 0; j < p->nvtx - 2; j++)
			fprintf(out, "\t\t\t%d %d %d,\n", p->vtx_index[0],
			  p->vtx_index[j + 1], p->vtx_index[j + 2]);
	}
	fprintf(out, "\t\t}\n");

	fprintf(out, "\t}\n");
}

void
model_dump(FILE *out, const char *model_name, const struct model *model)
{
	fprintf(out, "model \"%s\" {\n", model_name);

	mesh_dump(out, model->mesh);

	fprintf(out, "\tvolume {\n");
	bsp_tree_dump(out, model->bsp_tree);
	bounding_sphere_dump(out, &model->bounding_sphere);
	fprintf(out, "\t}\n");

	fprintf(out, "}\n");
}
