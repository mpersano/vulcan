%{
/* vrml_gram.y -- part of vulcan
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

#include "yy_to_modeldef.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "list.h"
#include "dict.h"
#include "vector.h"
#include "mesh.h"
#include "bsptree.h"
#include "mesh.h"
#include "model.h"
#include "model_dump.h"
#include "yyerror.h"
#include "panic.h"

struct tuple3i {
	int a, b, c;
};

struct tuple3f {
	float x, y, z;
};

struct tuple4f {
	float x, y, z, w;
};

struct volume {
	struct bsp_tree *bsp_tree;
	struct sphere *bounding_sphere;
};

struct tuple3i *
tuple3i_make(int a, int b, int c)
{
	struct tuple3i *p = malloc(sizeof *p);

	p->a = a;
	p->b = b;
	p->c = c;

	return p;
}

struct tuple3f *
tuple3f_make(float x, float y, float z)
{
	struct tuple3f *p = malloc(sizeof *p);

	p->x = x;
	p->y = y;
	p->z = z;

	return p;
}

struct tuple4f *
tuple4f_make(float x, float y, float z, float w)
{
	struct tuple4f *p = malloc(sizeof *p);

	p->x = x;
	p->y = y;
	p->z = z;
	p->w = w;

	return p;
}

enum attrib_type {
	/* body */
	ATTR_VOLUME,
	ATTR_MESH,

	/* mesh */
	ATTR_COORDINATES,
	ATTR_TRIANGLES,
	ATTR_NORMALS,

	/* volume */
	ATTR_BSP_TREE,
	ATTR_BOUNDING_SPHERE,

	/* sphere */
	ATTR_CENTER,
	ATTR_RADIUS,

	/* bsp tree */
	ATTR_NODES,
	ATTR_PLANES
};

struct attrib {
	enum attrib_type type;
	union {
		void *ptrval;
		float floatval;
	};
};

struct attrib *
attrib_ptrval_make(enum attrib_type type, void *ptrval)
{
	struct attrib *p = malloc(sizeof *p);

	p->type = type;
	p->ptrval = ptrval;

	return p;
}

struct attrib *
attrib_floatval_make(enum attrib_type type, float floatval)
{
	struct attrib *p = malloc(sizeof *p);

	p->type = type;
	p->floatval = floatval;

	return p;
}

void
attrib_free(struct attrib *attrib)
{
	switch (attrib->type) {
		case ATTR_CENTER:
			free(attrib->ptrval);
			break;

		case ATTR_NORMALS:
		case ATTR_COORDINATES:
		case ATTR_TRIANGLES:
		case ATTR_PLANES:
		case ATTR_NODES:
			list_free((struct list *)attrib->ptrval, free);
			break;

		default:
			break;
	}
}

static int
bsp_tree_count_children(struct bsp_tree *root)
{
	int num_children = 0;

	if (root->front != NULL)
		num_children += bsp_tree_count_children(root->front);

	if (root->back != NULL)
		num_children += bsp_tree_count_children(root->back);

	root->num_children = num_children;

	return num_children + 1;
}

struct bsp_tree *
bsp_tree_build_from_attribs(struct list *attrib_list)
{
	struct list_node *p;
	struct bsp_tree *nodes;
	struct list *node_list, *plane_list;
	int nnodes, i;

	node_list = plane_list = NULL;

	for (p = attrib_list->first; p; p = p->next) {
		struct attrib *attr = (struct attrib *)p->data;

		switch (attr->type) {
			case ATTR_NODES:
				if (node_list != NULL)
					yyerror("bsp tree with multiple "
					  "node lists?");
				node_list = (struct list *)attr->ptrval;
				break;

			case ATTR_PLANES:
				if (plane_list != NULL)
					yyerror("bsp tree with multiple "
					  "plane lists?");
				plane_list = (struct list *)attr->ptrval;
				break;

			default:
				assert(0);
				break;
		}
	}

	if (node_list == NULL)
		yyerror("bsp tree missing node list");

	if (plane_list == NULL)
		yyerror("bsp tree missing plane list");

	if (node_list->length != plane_list->length)
		yyerror("lengths of node and plane lists for bsp tree differ");

	nnodes = node_list->length;

	nodes = calloc(nnodes, sizeof *nodes);

	/* initialize planes */
	for (p = plane_list->first, i = 0; p; p = p->next, ++i) {
		float k;
		struct bsp_tree *const node = &nodes[i];
		struct tuple4f *tuple = (struct tuple4f *)p->data;

		node->plane.normal.x = tuple->x;
		node->plane.normal.y = tuple->y;
		node->plane.normal.z = tuple->z;

		k = -tuple->w/vec_length_squared(&node->plane.normal);
		vec_scalar_mul_copy(&node->plane.pt, &node->plane.normal, k);
	}

	/* initialize pointers */
	for (p = node_list->first; p; p = p->next) {
		struct tuple3i *tuple = (struct tuple3i *)p->data;
		struct bsp_tree *const node = &nodes[tuple->a - 1];

		node->front = tuple->b == -1 ? NULL : &nodes[tuple->b - 1];
		node->back = tuple->c == -1 ? NULL : &nodes[tuple->c - 1];
	}

	bsp_tree_count_children(&nodes[0]);

	return &nodes[0];
}

struct sphere *
sphere_build_from_attribs(struct list *attrib_list)
{
	struct sphere *sphere = malloc(sizeof *sphere);
	struct list_node *p;

	for (p = attrib_list->first; p; p = p->next) {
		struct attrib *attr = (struct attrib *)p->data;

		switch (attr->type) {
			case ATTR_CENTER:
				{
					struct tuple3f *tuple =
					  (struct tuple3f *)attr->ptrval;
					sphere->center.x = tuple->x;
					sphere->center.y = tuple->y;
					sphere->center.z = tuple->z;
				}
				break;

			case ATTR_RADIUS:
				sphere->radius = attr->floatval;
				break;

			default:
				break;
		}
	}

	return sphere;
}

struct volume *
volume_build_from_attribs(struct list *attrib_list)
{
	struct list_node *p;
	struct volume *volume;

	volume = calloc(1, sizeof *volume);

	for (p = attrib_list->first; p; p = p->next) {
		struct attrib *attr = (struct attrib *)p->data;

		switch (attr->type) {
			case ATTR_BSP_TREE:
				volume->bsp_tree =
				  (struct bsp_tree *)attr->ptrval;
				break;

			case ATTR_BOUNDING_SPHERE:
				volume->bounding_sphere =
				  (struct sphere *)attr->ptrval;
				break;

			default:
				break;
		}
	}

	return volume;
}

struct mesh *
mesh_build_from_attribs(struct list *attrib_list)
{
	struct list *coords, *normals, *triangles;
	struct list_node *p;
	struct mesh *mesh;
	int i;

	coords = normals = triangles = NULL;

	for (p = attrib_list->first; p; p = p->next) {
		struct attrib *attr = (struct attrib *)p->data;

		switch (attr->type) {
			case ATTR_COORDINATES:
				coords = (struct list *)attr->ptrval;
				break;

			case ATTR_NORMALS:
				normals = (struct list *)attr->ptrval;
				break;

			case ATTR_TRIANGLES:
				triangles = (struct list *)attr->ptrval;
				break;

			default:
				assert(0);
		}
	}

	if (coords->length != normals->length)
		yyerror("lengths of coords list and normals list differ");

	mesh = calloc(1, sizeof *mesh);

	mesh->npoly = triangles->length;
	mesh->nvtx = coords->length;

	mesh->vtx = malloc(mesh->nvtx*sizeof *mesh->vtx);
	mesh->poly = malloc(mesh->npoly*sizeof *mesh->poly);

	/* vertices */
	for (i = 0, p = coords->first; p; ++i, p = p->next) {
		struct tuple3f *tuple = (struct tuple3f *)p->data;

		mesh->vtx[i].pos.x = tuple->x;
		mesh->vtx[i].pos.y = tuple->y;
		mesh->vtx[i].pos.z = tuple->z;
	}

	/* normals */
	for (i = 0, p = normals->first; p; ++i, p = p->next) {
		struct tuple3f *tuple = (struct tuple3f *)p->data;

		mesh->vtx[i].normal.x = tuple->x;
		mesh->vtx[i].normal.y = tuple->y;
		mesh->vtx[i].normal.z = tuple->z;
	}

	/* triangles */
	for (i = 0, p = triangles->first; p; ++i, p = p->next) {
		struct polygon *poly = &mesh->poly[i];
		struct tuple3i *tuple = (struct tuple3i *)p->data;

		poly->nvtx = 3;

		poly->vtx_index[0] = tuple->a;
		poly->vtx_index[1] = tuple->b;
		poly->vtx_index[2] = tuple->c;
	}

	return mesh;
}

struct model *
model_build_from_attribs(struct list *attrib_list)
{
	struct model *model = calloc(1, sizeof *model);
	struct list_node *p;

	for (p = attrib_list->first; p; p = p->next) {
		struct attrib *attrib = (struct attrib *)p->data;

		switch (attrib->type) {
			case ATTR_MESH:
				model->mesh = (struct mesh *)attrib->ptrval;
				break;

			case ATTR_VOLUME:
				model->bsp_tree =
				  ((struct volume *)attrib->ptrval)->bsp_tree;
				model->bounding_sphere =
				  *((struct volume *)attrib->ptrval)->
				    bounding_sphere;
				break;

			default:
				break;
		}
	}

	return model;
}

static struct dict *model_dict;

%}
%union {
	long longval;
	float floatval;
	char *strval;
	struct list *listval;
	struct tuple3i *tuple3i;
	struct tuple3f *tuple3f;
	struct tuple4f *tuple4f;
	struct attrib *attrib;
};

%token <strval> STRINGLIT
%token <floatval> FLOAT
%token <longval> INTEGER

%token MODEL MESH COORDINATES TRIANGLES NORMALS VOLUME
%token BOUNDING_SPHERE CENTER RADIUS
%token BSP_TREE PLANES NODES

%type <longval> scalari
%type <floatval> scalarf
%type <tuple3i> tuple3i
%type <tuple3f> tuple3f
%type <tuple4f> tuple4f
%type <listval> tuple3i_list tuple3i_list_elems
%type <listval> tuple3f_list tuple3f_list_elems
%type <listval> tuple4f_list tuple4f_list_elems
%type <attrib> bsp_tree_attrib planes tree_nodes
%type <attrib> mesh_attrib coordinates triangles normals
%type <attrib> volume_attrib bounding_sphere bsp_tree
%type <attrib> body_attrib mesh volume
%type <attrib> sphere_attrib radius center
%type <listval> bsp_tree_attribs mesh_attribs volume_attribs body_attribs sphere_attribs
%%

input
: models
| /* NOTHING */
;

models
: models model
| model
;

model
: MODEL STRINGLIT '{' body_attribs '}'
	{
		struct model *model = model_build_from_attribs($4);

		dict_put(model_dict, $2, model);

		/* TODO: release memory */
	}
;

body_attribs
: body_attribs body_attrib		{ $$ = list_append($1, $2); }
| body_attrib				{ $$ = list_append(list_make(), $1); }
;

body_attrib
: mesh
| volume
;

mesh
: MESH '{' mesh_attribs '}'
	{
		$$ = attrib_ptrval_make(ATTR_MESH, mesh_build_from_attribs($3));

		list_free($3, (void(*)(void *))attrib_free);
	}
;

mesh_attribs
: mesh_attribs mesh_attrib		{ $$ = list_append($1, $2); }
| mesh_attrib				{ $$ = list_append(list_make(), $1); }
;

mesh_attrib
: coordinates
| triangles
| normals
;

coordinates
: COORDINATES '{' tuple3f_list '}'	{ $$ = attrib_ptrval_make(ATTR_COORDINATES, $3); }
;

triangles
: TRIANGLES '{' tuple3i_list '}'	{ $$ = attrib_ptrval_make(ATTR_TRIANGLES, $3); }
;

normals
: NORMALS '{' tuple3f_list '}'		{ $$ = attrib_ptrval_make(ATTR_NORMALS, $3); }
;

volume
: VOLUME '{' volume_attribs '}'
	{
		$$ = attrib_ptrval_make(ATTR_VOLUME,
		  volume_build_from_attribs($3));
	}
;

volume_attribs
: volume_attribs volume_attrib		{ $$ = list_append($1, $2); }
| volume_attrib				{ $$ = list_append(list_make(), $1); }
;

volume_attrib
: bounding_sphere
| bsp_tree
;

bounding_sphere
: BOUNDING_SPHERE '{' sphere_attribs '}'
	{
		$$ = attrib_ptrval_make(ATTR_BOUNDING_SPHERE,
		  sphere_build_from_attribs($3));

		list_free($3, (void(*)(void *))attrib_free);
	}
;

sphere_attribs
: sphere_attribs sphere_attrib		{ $$ = list_append($1, $2); }
| sphere_attrib				{ $$ = list_append(list_make(), $1); }
;

sphere_attrib
: center
| radius
;

center
: CENTER tuple3f			{ $$ = attrib_ptrval_make(ATTR_CENTER, $2); }
;

radius
: RADIUS scalarf			{ $$ = attrib_floatval_make(ATTR_RADIUS, $2); }
;

bsp_tree
: BSP_TREE '{' bsp_tree_attribs '}'
	{
		$$ = attrib_ptrval_make(ATTR_BSP_TREE,
			bsp_tree_build_from_attribs($3));

		list_free($3, (void(*)(void *))attrib_free);
	}
;

bsp_tree_attribs
: bsp_tree_attribs bsp_tree_attrib	{ $$ = list_append($1, $2); }
| bsp_tree_attrib			{ $$ = list_append(list_make(), $1); }
;

bsp_tree_attrib
: planes
| tree_nodes
;

planes
: PLANES '{' tuple4f_list '}'		{ $$ = attrib_ptrval_make(ATTR_PLANES, $3); }
;

tree_nodes
: NODES '{' tuple3i_list '}'		{ $$ = attrib_ptrval_make(ATTR_NODES, $3); }
;

tuple3f_list
: tuple3f_list_elems ','		{ $$ = $1; }
| tuple3f_list_elems
;

tuple3f_list_elems
: tuple3f_list_elems ',' tuple3f	{ $$ = list_append($1, $3); }
| tuple3f				{ $$ = list_append(list_make(), $1); }
;

tuple4f_list
: tuple4f_list_elems ','		{ $$ = $1; }
| tuple4f_list_elems
;

tuple4f_list_elems
: tuple4f_list_elems ',' tuple4f	{ $$ = list_append($1, $3); }
| tuple4f				{ $$ = list_append(list_make(), $1); }
;

tuple3i_list
: tuple3i_list_elems ','		{ $$ = $1; }
| tuple3i_list_elems
;

tuple3i_list_elems
: tuple3i_list_elems ',' tuple3i	{ $$ = list_append($1, $3); }
| tuple3i				{ $$ = list_append(list_make(), $1); }
;

tuple3f
: scalarf scalarf scalarf		{ $$ = tuple3f_make($1, $2, $3); }
;

tuple4f
: scalarf scalarf scalarf scalarf	{ $$ = tuple4f_make($1, $2, $3, $4); }
;

tuple3i
: scalari scalari scalari		{ $$ = tuple3i_make($1, $2, $3); }
;

scalarf
: FLOAT
| INTEGER				{ $$ = $1; }
;

scalari
: INTEGER
;
%%
#include <stdio.h>

extern FILE *yyin;
extern int lineno;

void
modeldef_parse_file(struct dict *model_dict_, const char *filename)
{
	FILE *in;

	lineno = 1;

	if ((in = fopen(filename, "r")) == NULL)
		panic("could not open `%s': %s", filename, strerror(errno));

	yyin = in;

	model_dict = model_dict_;

	yyparse();

	fclose(in);
}

#if 0
int
main(void)
{
	struct dict *dict;
	struct model *model;

	dict = dict_make();

	modeldef_parse_file(dict, "pawn-low.modeldef");

	model = dict_get(dict, "pawn-low");

	model_dump(stdout, "pawn-low", model);

	return 0;
}
#endif
