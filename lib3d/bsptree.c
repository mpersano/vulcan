/* bsptree.c -- part of vulcan
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
#include <stdio.h>
#include "mesh.h"
#include "vector.h"
#include "list.h"
#include "plane.h"
#include "bsptree.h"

struct vtx_set {
	struct vertex *vtx;
	int max_vtx;
	int nvtx;
};

void
vtx_set_init(struct vtx_set *vs)
{
	vs->nvtx = 0;
	vs->max_vtx = 10;
	vs->vtx = malloc(vs->max_vtx * sizeof *vs->vtx);
}

static void
vtx_set_init_from(struct vtx_set *vs, const struct vertex *vtx, int nvtx)
{
	vs->nvtx = nvtx;
	vs->max_vtx = vs->nvtx + 10;
	vs->vtx = malloc(vs->max_vtx * sizeof *vs->vtx);
	memcpy(vs->vtx, vtx, vs->nvtx * sizeof *vs->vtx);
}

static void
vtx_set_append(struct vtx_set *vs, const struct vertex *v)
{
	if (vs->nvtx == vs->max_vtx) {
		vs->max_vtx *= 2;
		vs->vtx = realloc(vs->vtx, vs->max_vtx * sizeof *vs->vtx);
	}

	memcpy(&vs->vtx[vs->nvtx++], v, sizeof *v);
}

static enum rpos
plane_classify_polygon(const struct plane *plane, const struct polygon *poly,
  const struct vertex *vtx)
{
	int i;
	enum rpos c, cv;
	
	c = RP_ON;

	for (i = 0; i < poly->nvtx; i++) {
		cv = plane_classify_point(plane,
		  &(vtx[poly->vtx_index[i]].pos));

		if (cv != RP_ON) {
			if (c == RP_ON) {
				c = cv;
			} else if (c != cv) {
				c = RP_SPANNING;
				break;
			}
		}
	}
	
	return c;
}

static void
plane_split_polygon(struct polygon *front_poly, struct polygon *back_poly,
  const struct plane *plane, const struct polygon *in_poly, struct vtx_set *vs)
{
	int i, j;
	enum rpos c1, c2;
	struct vector *a, *b;
	struct vertex nv;
	const int *in_vtx_index;
	int *front_vtx_index, *back_vtx_index;
	int in_nvtx, front_nvtx, back_nvtx;

	vec_copy(&front_poly->normal, &in_poly->normal);
	vec_copy(&back_poly->normal, &in_poly->normal);

	in_vtx_index = in_poly->vtx_index;
	front_vtx_index = front_poly->vtx_index;
	back_vtx_index = back_poly->vtx_index;

	in_nvtx = in_poly->nvtx;
	front_nvtx = back_nvtx = 0;

	for (i = 0; i < in_nvtx; i++) {
		j = i < in_nvtx - 1 ? i + 1 : 0;

		a = &(vs->vtx[in_vtx_index[i]].pos);
		b = &(vs->vtx[in_vtx_index[j]].pos);

		c1 = plane_classify_point(plane, a);
		c2 = plane_classify_point(plane, b);

		if (c1 == RP_FRONT || c1 == RP_ON)
			front_vtx_index[front_nvtx++] = in_vtx_index[i];

		if (c1 == RP_BACK || c1 == RP_ON)
			back_vtx_index[back_nvtx++] = in_vtx_index[i];

		if ((c1 == RP_FRONT && c2 == RP_BACK) ||
		  (c1 == RP_BACK && c2 == RP_FRONT)) {
			plane_intersect(&nv.pos, plane, a, b);
			vtx_set_append(vs, &nv);
			front_vtx_index[front_nvtx++] = vs->nvtx - 1;
			back_vtx_index[back_nvtx++] = vs->nvtx - 1;
		}
	}

	front_poly->nvtx = front_nvtx;
	back_poly->nvtx = back_nvtx;

	assert(front_poly->nvtx >= 3);
	assert(back_poly->nvtx >= 3);
}

static struct polygon *
poly_dup(const struct polygon *poly)
{
	struct polygon *p;

	p = malloc(sizeof *p);
	memcpy(p, poly, sizeof *p);

	return p;
}

static void
poly_free(struct polygon *poly)
{
	free(poly);
}

static void
pick_partition_plane(struct plane *out_plane, const struct list *poly_list,
  const struct vtx_set *vs)
{
	struct list_node *ln;
	struct polygon *poly;
	struct plane p;
	int i, split_count, min_splits;

	min_splits = 0;

	for (i = 0; i < 3; i++) {
		poly = (struct polygon *)list_element_at(poly_list,
		  rand() % poly_list->length);

		p.normal = poly->normal;
		p.pt = vs->vtx[poly->vtx_index[0]].pos;

		assert(plane_classify_polygon(&p, poly, vs->vtx) == RP_ON);

		split_count = 0;

		for (ln = poly_list->first; ln != NULL; ln = ln->next) {
			if (plane_classify_polygon(&p,
			  (struct polygon *)ln->data, vs->vtx) == RP_SPANNING)
				++split_count;
		}

		if (i == 0 || split_count < min_splits) {
			*out_plane = p;
			min_splits = split_count;
		}
	}
}

static struct bsp_tree *
bsp_tree_build(const struct plane *plane, const struct list *poly_list,
  struct vtx_set *vs)
{
	struct list *front_list, *back_list, *on_list;
	struct list_node *ln;
	struct polygon *poly;
	struct bsp_tree *node;

	/* XXX: since nodes will only have planes we don't need on_list */

	front_list = list_make();
	back_list = list_make();
	on_list = list_make();

	for (ln = poly_list->first; ln != NULL; ln = ln->next) {
		poly = (struct polygon *)ln->data;

		switch (plane_classify_polygon(plane, poly, vs->vtx)) {
			case RP_FRONT:
				list_append(front_list, poly_dup(poly));
				break;

			case RP_BACK:
				list_append(back_list, poly_dup(poly));
				break;

			case RP_ON:
				list_append(on_list, poly_dup(poly));
				break;

			case RP_SPANNING:
				{
					struct polygon front_poly, back_poly;

					plane_split_polygon(&front_poly,
					  &back_poly, plane, poly, vs);
					list_append(front_list,
					  poly_dup(&front_poly));
					list_append(back_list,
					  poly_dup(&back_poly));
				}
				break;
		}
	}

	assert(on_list->length > 0);

	node = malloc(sizeof *node);
	node->plane = *plane;
	node->num_children = 0;

	if (front_list->length == 0) {
		node->front = NULL;
	} else {
		struct plane p;

		pick_partition_plane(&p, front_list, vs);

		node->front = bsp_tree_build(&p, front_list, vs);

		node->num_children += node->front->num_children + 1;
	}

	if (back_list->length == 0) {
		node->back = NULL;
	} else {
		struct plane p;

		pick_partition_plane(&p, back_list, vs);

		node->back = bsp_tree_build(&p, back_list, vs);

		node->num_children += node->back->num_children + 1;
	}
	
	list_free(front_list, (void(*)(void *))poly_free);
	list_free(back_list, (void(*)(void *))poly_free);
	list_free(on_list, (void(*)(void *))poly_free); 

	return node;
}

struct bsp_tree *
bsp_tree_build_from_mesh(const struct mesh *mesh)
{
	struct vtx_set vs;
	struct bsp_tree *root;
	struct list *poly_list;
	struct plane plane;
	int i;

	vtx_set_init_from(&vs, mesh->vtx, mesh->nvtx);

	poly_list = list_make();

	for (i = 0; i < mesh->npoly; i++)
		list_append(poly_list, poly_dup(&mesh->poly[i]));

	pick_partition_plane(&plane, poly_list, &vs);

	root = bsp_tree_build(&plane, poly_list, &vs);

	list_free(poly_list, (void(*)(void *))poly_free); 
	free(vs.vtx);

	return root;
}

#ifdef TEST_BSPTREE

#include <stdio.h>
#include "3ds.h"

int
main_old(int argc, char *argv[])
{
	struct plane p = { { -1.f, 1.f, 0.f }, { 1.f, 1.f, 0.f } };
	struct vector a = { 1.f, 2.f }, b = { 2.f, 0.f }, c = { 0.f, 0.f }, r;

	vec_normalize(&p.normal);

	printf("%d\n", plane_classify_point(&p, &a));
	printf("%d\n", plane_classify_point(&p, &b));
	printf("%d\n", plane_classify_point(&p, &c));

	plane_intersect(&r, &p, &a, &b);

	vec_print(&r);

	return 0;
}

int
main(int argc, char *argv[])
{
	struct list *l;
	struct list_node *ln;
	struct mesh *m;

	if (argc < 2) {
		fprintf(stderr, "%s foo.3ds\n", argv[0]);
		return -1;
	}

	if ((l = import_3ds_meshes(argv[1])) != NULL) {
		for (ln = l->first; ln != NULL; ln = ln->next) {
			m = (struct mesh *)ln->data;
			mesh_init_poly_normals(m);
			printf("%s: %d verts, %d polys\n", m->name, m->nvtx,
			  m->npoly);
			printf("building tree...\n");
			bsp_tree_build_from_mesh(m);
			break;
		}
	}

	return 0;
}

int
main_blah(int argc, char *argv[])
{
	struct plane p = { { 1.f, 2.f, 0.f }, { 2.f, 1.f, 0.f } };
	struct vector a = { 1.f, 4.f, 0.f };

	vec_normalize(&p.normal);

	printf("%f\n", plane_point_distance(&p, &a));

	return 0;
}

#endif /* TEST_BSPTREE */
