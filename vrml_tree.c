/* vrml_tree.c -- part of vulcan
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
#include <stdio.h>
#include <assert.h>

#include "list.h"
#include "vrml_tree.h"


struct vrml_vector *
vrml_vector_make(float x, float y, float z)
{
	struct vrml_vector *v;

	v = malloc(sizeof *v);

	v->x = x;
	v->y = y;
	v->z = z;

	return v;
}


void
vrml_vector_free(struct vrml_vector *v)
{
	free(v);
}


static void
ident(int n)
{
	while (n--)
		putchar('\t');
}


static void
vrml_node_common_free(union vrml_node *node)
{
	if (node->common.name)
		free(node->common.name);

	free(node);
}


static void
vrml_node_separator_dump(union vrml_node *node, int tabs)
{
	struct vrml_node_separator *vn;
	struct list_node *ln;

	assert(node->common.type == VRML_NODE_SEPARATOR);

	vn = (struct vrml_node_separator *)node;

	ident(tabs);
	if (vn->common.name)
		printf("DEF %s ", vn->common.name);
	printf("Separator {\n");

	for (ln = vn->children->first; ln; ln = ln->next) {
		union vrml_node *child = (union vrml_node *)ln->data;
		child->common.dump_fn(child, tabs + 1);
	}

	ident(tabs);

	printf("}\n");
}


static void
vrml_node_separator_free(union vrml_node *node)
{
	assert(node->common.type == VRML_NODE_SEPARATOR);

	list_free(node->separator.children, (void(*)(void *))vrml_node_free);

	vrml_node_common_free(node);
}


union vrml_node *
vrml_node_separator_make(struct list *children)
{
	struct vrml_node_separator *vn;

	vn = malloc(sizeof *vn);

	vn->common.type = VRML_NODE_SEPARATOR;
	vn->common.dump_fn = vrml_node_separator_dump;
	vn->common.free_fn = vrml_node_separator_free;
	vn->children = children;

	return (union vrml_node *)vn;
}


static void
vrml_node_coordinate3_free(union vrml_node *node)
{
	assert(node->common.type == VRML_NODE_COORDINATE3);

	list_free(node->coordinate3.points, (void(*)(void *))vrml_vector_free);

	vrml_node_common_free(node);
}


static void
vrml_node_coordinate3_dump(union vrml_node *node, int tabs)
{
	struct vrml_node_coordinate3 *vn;
	struct list_node *ln;

	assert(node->common.type == VRML_NODE_COORDINATE3);

	vn = (struct vrml_node_coordinate3 *)node;

	ident(tabs);
	if (vn->common.name)
		printf("DEF %s ", vn->common.name);
	printf("Coordinate3 {\n");

	ident(tabs + 1);
	printf("point [\n");

	for (ln = vn->points->first; ln; ln = ln->next) {
		struct vrml_vector *v = (struct vrml_vector *)ln->data;
		ident(tabs + 2);
		printf("%f %f %f,\n", v->x, v->y, v->z);
	}

	ident(tabs + 1);
	printf("]\n");

	ident(tabs);
	printf("}\n");
}


union vrml_node *
vrml_node_coordinate3_make(struct list *points)
{
	struct vrml_node_coordinate3 *vn;

	vn = malloc(sizeof *vn);

	vn->common.type = VRML_NODE_COORDINATE3;
	vn->common.dump_fn = vrml_node_coordinate3_dump;
	vn->common.free_fn = vrml_node_coordinate3_free;
	vn->points = points;

	return (union vrml_node *)vn;
}


static void
vrml_indexed_face_set_free(union vrml_node *node)
{
	assert(node->common.type == VRML_NODE_INDEXED_FACE_SET);

	list_free(node->indexed_face_set.indexes, NULL);

	vrml_node_common_free(node);
}


static void
vrml_indexed_face_set_dump(union vrml_node *node, int tabs)
{
	struct vrml_node_indexed_face_set *vn;
	struct list_node *ln;

	assert(node->common.type == VRML_NODE_INDEXED_FACE_SET);

	vn = (struct vrml_node_indexed_face_set *)node;

	ident(tabs);
	if (vn->common.name)
		printf("DEF %s ", vn->common.name);
	printf("IndexedFaceSet {\n");

	ident(tabs + 1);
	printf("coordIndex [\n");

	ident(tabs + 2);

	for (ln = vn->indexes->first; ln; ln = ln->next) {
		int idx = (int)ln->data;

		printf("%d,", idx);

		if (idx == -1) {
			putchar('\n');
			ident(tabs + 2);
		}
	}

	putchar('\n');

	ident(tabs + 1);
	printf("]\n");

	ident(tabs);
	printf("}\n");
}


union vrml_node *
vrml_node_indexed_face_set_make(struct list *indexes)
{
	struct vrml_node_indexed_face_set *vn;

	vn = malloc(sizeof *vn);

	vn->common.type = VRML_NODE_INDEXED_FACE_SET;
	vn->common.dump_fn = vrml_indexed_face_set_dump;
	vn->common.free_fn = vrml_indexed_face_set_free;
	vn->indexes = indexes;

	return (union vrml_node *)vn;
}


static void
vrml_node_info_free(union vrml_node *node)
{
	assert(node->common.type == VRML_NODE_INFO);

	free(node->info.info_str);

	vrml_node_common_free(node);
}


static void
vrml_node_info_dump(union vrml_node *node, int tabs)
{
	struct vrml_node_info *vn;

	assert(node->common.type == VRML_NODE_INFO);

	vn = (struct vrml_node_info *)node;

	ident(tabs);
	if (vn->common.name)
		printf("DEF %s ", vn->common.name);
	printf("Info {\n");

	ident(tabs + 1);
	printf("string \"%s\"\n", vn->info_str);

	ident(tabs);
	printf("}\n");
}


union vrml_node *
vrml_node_info_make(char *info_str)
{
	struct vrml_node_info *vn;

	vn = malloc(sizeof *vn);

	vn->common.type = VRML_NODE_INFO;
	vn->common.dump_fn = vrml_node_info_dump;
	vn->common.free_fn = vrml_node_info_free;
	vn->info_str = info_str;

	return (union vrml_node *)vn;
}


void
vrml_node_free(union vrml_node *node)
{
	node->common.free_fn(node);
}
